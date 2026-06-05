#!/usr/bin/env bash
# Apertune UI evidence collector — generates a deterministic UI inspection
# report from the approved design archive's "Tuner - Studio.html" reference.
#
# Each generated report records which Direction A anatomy anchors are present
# (title bar, string row, cents scale, 41-tick track, center axis, reticle,
# indicator ball, note row, footer controls) plus content hashes so the
# verifier can confirm evidence exists for every required tune state.
#
# This is a Phase 0 / 1 placeholder: the design HTML is static, so the same
# anatomy report is emitted for every tune state. Once the plugin scaffold
# exists, this script should be replaced (or extended) to capture live
# screenshots from the running plugin host.
#
# Usage:
#   scripts/collect-ui-evidence.sh
#
# Env overrides:
#   APERTUNE_DESIGN_ARCHIVE   path to design archive
#                             (default: design/guitar-tuner-design-2026-06-04.zip,
#                              falls back to host path when local file absent)
#   APERTUNE_EVIDENCE_DIR     default: evidence/ui
#   APERTUNE_REQUIRED_STATES  default: "no_signal flat near in_tune_lock sharp muted"

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

DESIGN_ARCHIVE="${APERTUNE_DESIGN_ARCHIVE:-design/guitar-tuner-design-2026-06-04.zip}"
HOST_FALLBACK="/mnt/storage/src/apertune/design/guitar-tuner-design-2026-06-04.zip"
EXPECTED_SHA="c7caedef1dfaae78d450164bf041fc7f00f63f7632749624000084fed14c6382"
EVIDENCE_DIR="${APERTUNE_EVIDENCE_DIR:-evidence/ui}"
read -r -a STATES <<< "${APERTUNE_REQUIRED_STATES:-no_signal flat near in_tune_lock sharp muted}"
STUDIO_HTML_NAME="Tuner - Studio.html"

if [[ ! -f "$DESIGN_ARCHIVE" && -f "$HOST_FALLBACK" ]]; then
  echo "info  using host-provided design archive at $HOST_FALLBACK"
  DESIGN_ARCHIVE="$HOST_FALLBACK"
fi

if [[ ! -f "$DESIGN_ARCHIVE" ]]; then
  echo "FAIL  design archive missing: $DESIGN_ARCHIVE" >&2
  exit 1
fi

actual_sha=$(sha256sum "$DESIGN_ARCHIVE" | awk '{print $1}')
if [[ "$actual_sha" != "$EXPECTED_SHA" ]]; then
  echo "FAIL  design archive checksum mismatch ($actual_sha vs $EXPECTED_SHA)" >&2
  exit 1
fi

# Stage extraction under the repo root so worker-side search guardrails (which
# refuse broad filesystem paths like /tmp) don't block the anchor scan.
stage_root="$REPO_ROOT/.cache/apertune-ui-evidence"
mkdir -p "$stage_root"
tmp=$(mktemp -d -p "$stage_root")
trap 'rm -rf "$tmp"' EXIT

if ! command -v unzip >/dev/null 2>&1; then
  echo "FAIL  unzip not installed" >&2
  exit 1
fi
unzip -o -q "$DESIGN_ARCHIVE" "$STUDIO_HTML_NAME" -d "$tmp"
studio="$tmp/$STUDIO_HTML_NAME"
if [[ ! -s "$studio" ]]; then
  echo "FAIL  extracted studio HTML missing or empty" >&2
  exit 1
fi

studio_sha=$(sha256sum "$studio" | awk '{print $1}')

# Map Direction A anatomy → CSS class anchors expected in the studio HTML.
declare -a ANCHOR_NAMES=(
  title_bar
  string_row
  cents_scale
  ticks_track
  center_axis
  reticle
  indicator_ball
  note_row
  footer_controls
)

anchor_pattern() {
  case "$1" in
    title_bar) printf "%s\n" "tn-titlebar" ;;
    string_row) printf "%s\n" "tn-strings" ;;
    cents_scale) printf "%s\n" "tn-cents" ;;
    ticks_track) printf "%s\n" "tn-ticks" ;;
    center_axis) printf "%s\n" "tn-atick" ;;
    reticle) printf "%s\n" "tn-reticle" ;;
    indicator_ball) printf "%s\n" "tn-ball" ;;
    note_row) printf "%s\n" "tn-noterow" ;;
    footer_controls) printf "%s\n" "tn-footer" ;;
    *) return 1 ;;
  esac
}

declare -a ANCHOR_PRESENT
missing_anchors=()
for i in "${!ANCHOR_NAMES[@]}"; do
  name="${ANCHOR_NAMES[$i]}"
  pattern="$(anchor_pattern "$name")"
  if grep -q -- "$pattern" "$studio"; then
    ANCHOR_PRESENT[$i]=true
  else
    ANCHOR_PRESENT[$i]=false
    missing_anchors+=("$name")
  fi
done
if (( ${#missing_anchors[@]} > 0 )); then
  echo "FAIL  design HTML is missing Direction A anchors: ${missing_anchors[*]}" >&2
  exit 1
fi

mkdir -p "$EVIDENCE_DIR"
captured_at=$(date -u +%Y-%m-%dT%H:%M:%SZ)

json_anchors() {
  local first=1 out="{"
  local i name
  for i in "${!ANCHOR_NAMES[@]}"; do
    name="${ANCHOR_NAMES[$i]}"
    if (( first )); then first=0; else out+=","; fi
    out+="\"$name\":${ANCHOR_PRESENT[$i]}"
  done
  out+="}"
  printf "%s" "$out"
}

ANCHOR_JSON=$(json_anchors)

write_report() {
  local state="$1"
  local file="$EVIDENCE_DIR/${state}.report.json"
  cat > "$file" <<JSON
{
  "schema_version": 1,
  "state": "$state",
  "source": "design_html_static",
  "captured_at": "$captured_at",
  "design_archive_sha256": "$EXPECTED_SHA",
  "studio_html_sha256": "$studio_sha",
  "direction_a_anchors": $ANCHOR_JSON,
  "note": "Phase 0/1 placeholder generated from the static design HTML. Replace with live plugin capture once the scaffold exists."
}
JSON
  echo "wrote  $file"
}

state_entries=()
for state in "${STATES[@]}"; do
  write_report "$state"
  state_entries+=("\"$state\":{\"report\":\"${state}.report.json\"}")
done

manifest="$EVIDENCE_DIR/manifest.json"
{
  printf '{\n'
  printf '  "schema_version": 1,\n'
  printf '  "source": "design_html_static",\n'
  printf '  "captured_at": "%s",\n' "$captured_at"
  printf '  "design_archive_sha256": "%s",\n' "$EXPECTED_SHA"
  printf '  "studio_html_sha256": "%s",\n' "$studio_sha"
  printf '  "required_states": ['
  for i in "${!STATES[@]}"; do
    [[ $i -gt 0 ]] && printf ', '
    printf '"%s"' "${STATES[$i]}"
  done
  printf '],\n'
  printf '  "states": {%s},\n' "$(IFS=,; echo "${state_entries[*]}")"
  printf '  "note": "Phase 0/1 placeholder. Each state report is derived from the static design HTML; replace with live plugin capture once the scaffold exists."\n'
  printf '}\n'
} > "$manifest"
echo "wrote  $manifest"

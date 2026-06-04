#!/usr/bin/env bash
# Apertune product verifier — asserts the MVP Definition of Done.
#
# Single command, exits non-zero if any required check is unmet.
# Build-only success is NEVER reported as Done; UI work is NEVER PASS without
# visible evidence for the key tune states.
#
# Sections (run in order):
#   design            — design archive SHA-256
#   dsp-tests         — DSP / unit tests via the project's test runner
#   build-artifacts   — CLAP / VST3 / AU artifacts under $APERTUNE_BUILD_DIR
#   plugin-validation — clap-validator / pluginval / auval (where available)
#   ui-evidence       — screenshot or deterministic UI inspection report for
#                       each required tune state (default: flat / in_tune / sharp)
#
# Usage:
#   scripts/verify.sh                          # run all sections
#   scripts/verify.sh --section=NAME[,NAME...] # run a subset
#   scripts/verify.sh --list                   # list section names
#   scripts/verify.sh --help                   # this help
#
# Env overrides:
#   APERTUNE_DESIGN_ARCHIVE   default: design/guitar-tuner-design-2026-06-04.zip
#   APERTUNE_BUILD_DIR        default: build
#   APERTUNE_EVIDENCE_DIR     default: evidence/ui
#   APERTUNE_REQUIRED_STATES  default: "flat in_tune sharp"
#   APERTUNE_AU_SPEC          'aufx <type> <sub> <mfr>' for auval (macOS only)

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

DESIGN_ARCHIVE="${APERTUNE_DESIGN_ARCHIVE:-design/guitar-tuner-design-2026-06-04.zip}"
EXPECTED_SHA="c7caedef1dfaae78d450164bf041fc7f00f63f7632749624000084fed14c6382"
BUILD_DIR="${APERTUNE_BUILD_DIR:-build}"
EVIDENCE_DIR="${APERTUNE_EVIDENCE_DIR:-evidence/ui}"
read -r -a REQUIRED_STATES <<< "${APERTUNE_REQUIRED_STATES:-flat in_tune sharp}"

FAILED=0
SKIPPED=0

pass() { printf "PASS  %s\n" "$*"; }
fail() { printf "FAIL  %s\n" "$*" >&2; FAILED=1; }
skip() { printf "SKIP  %s\n" "$*"; SKIPPED=1; }
info() { printf "      %s\n" "$*"; }

# --- design ----------------------------------------------------------------

check_design() {
  local label="design archive SHA-256"
  if [[ ! -f "$DESIGN_ARCHIVE" ]]; then
    fail "$label — missing: $DESIGN_ARCHIVE (provided by host, never committed)"
    return
  fi
  local actual
  actual=$(sha256sum "$DESIGN_ARCHIVE" | awk '{print $1}')
  if [[ "$actual" != "$EXPECTED_SHA" ]]; then
    fail "$label — got $actual, expected $EXPECTED_SHA"
    return
  fi
  pass "$label"
}

# --- dsp / unit tests ------------------------------------------------------

check_dsp_tests() {
  local label="DSP / unit tests"
  if [[ -f CMakeLists.txt ]]; then
    if [[ ! -d "$BUILD_DIR" ]]; then
      fail "$label — CMake project but $BUILD_DIR missing (configure first)"
      return
    fi
    if ! command -v ctest >/dev/null 2>&1; then
      fail "$label — ctest not installed"
      return
    fi
    if (cd "$BUILD_DIR" && ctest --output-on-failure); then
      pass "$label (ctest)"
    else
      fail "$label — ctest reported failures"
    fi
    return
  fi
  if [[ -f package.json ]]; then
    if ! command -v bun >/dev/null 2>&1; then
      fail "$label — bun not installed"
      return
    fi
    if bun test; then
      pass "$label (bun test)"
    else
      fail "$label — bun test failed"
    fi
    return
  fi
  if [[ -f pyproject.toml ]]; then
    if ! command -v uv >/dev/null 2>&1; then
      fail "$label — uv not installed"
      return
    fi
    if uv run pytest; then
      pass "$label (pytest)"
    else
      fail "$label — pytest failed"
    fi
    return
  fi
  fail "$label — plugin scaffold has not been added (no CMakeLists.txt / package.json / pyproject.toml)"
}

# --- build artifacts -------------------------------------------------------

# Print the first artifact matching a name pattern under BUILD_DIR; empty if
# none. Restricted to BUILD_DIR so we never traverse outside the worktree.
find_artifact() {
  # $1 = -name | -iname  (defaults to -name)
  # $2 = pattern (e.g. "*.clap")
  # $3 = type flag, optional (e.g. "-type d")
  local flag="${1:--name}" pattern="$2"
  shift 2
  find "$BUILD_DIR" -maxdepth 8 "$flag" "$pattern" "$@" -print 2>/dev/null | head -n 1
}

check_build_artifacts() {
  local label="plugin build artifacts (CLAP / VST3 / AU)"
  if [[ ! -d "$BUILD_DIR" ]]; then
    fail "$label — build dir missing: $BUILD_DIR"
    return
  fi
  local clap vst3 au missing=()
  clap=$(find_artifact -name "*.clap")
  vst3=$(find_artifact -name "*.vst3")
  au=$(find_artifact -name "*.component" -type d)
  [[ -z "$clap" ]] && missing+=("CLAP (.clap)")
  [[ -z "$vst3" ]] && missing+=("VST3 (.vst3)")
  if [[ "$(uname -s)" == "Darwin" ]]; then
    [[ -z "$au" ]] && missing+=("AU (.component)")
  fi
  if (( ${#missing[@]} > 0 )); then
    fail "$label — missing: ${missing[*]}"
    return
  fi
  pass "$label"
}

# --- plugin validation -----------------------------------------------------

check_plugin_validation() {
  local label="plugin validation"
  if [[ ! -d "$BUILD_DIR" ]]; then
    fail "$label — build dir missing: $BUILD_DIR"
    return
  fi
  local clap vst3
  clap=$(find_artifact -name "*.clap")
  vst3=$(find_artifact -name "*.vst3")

  local ran=0 any_failure=0

  if [[ -n "$clap" ]]; then
    if command -v clap-validator >/dev/null 2>&1; then
      ran=1
      if ! clap-validator validate "$clap"; then any_failure=1; fi
    else
      info "clap-validator not on PATH; skipping CLAP validation"
    fi
  fi

  if [[ -n "$vst3" ]]; then
    if command -v pluginval >/dev/null 2>&1; then
      ran=1
      if ! pluginval --validate "$vst3"; then any_failure=1; fi
    else
      info "pluginval not on PATH; skipping VST3 validation"
    fi
  fi

  if [[ "$(uname -s)" == "Darwin" ]] \
      && command -v auval >/dev/null 2>&1 \
      && [[ -n "${APERTUNE_AU_SPEC:-}" ]]; then
    ran=1
    # shellcheck disable=SC2086
    if ! auval -v $APERTUNE_AU_SPEC; then any_failure=1; fi
  fi

  if (( any_failure )); then
    fail "$label — at least one validator reported failures"
    return
  fi
  if (( ran == 0 )); then
    skip "$label — no validators available on PATH (install clap-validator / pluginval)"
    return
  fi
  pass "$label"
}

# --- ui evidence -----------------------------------------------------------

check_ui_evidence() {
  local label="UI state evidence"
  if [[ ! -d "$EVIDENCE_DIR" ]]; then
    fail "$label — evidence dir missing: $EVIDENCE_DIR (run scripts/collect-ui-evidence.sh)"
    return
  fi
  local manifest="$EVIDENCE_DIR/manifest.json"
  if [[ ! -s "$manifest" ]]; then
    fail "$label — missing or empty manifest: $manifest"
    return
  fi
  local missing_evidence=() missing_manifest=()
  local s png rpt
  for s in "${REQUIRED_STATES[@]}"; do
    png="$EVIDENCE_DIR/${s}.png"
    rpt="$EVIDENCE_DIR/${s}.report.json"
    if [[ ! -s "$png" && ! -s "$rpt" ]]; then
      missing_evidence+=("$s")
    fi
    if ! grep -q "\"$s\"" "$manifest"; then
      missing_manifest+=("$s")
    fi
  done
  if (( ${#missing_evidence[@]} > 0 )); then
    fail "$label — missing screenshot or report for tune state(s): ${missing_evidence[*]}"
    return
  fi
  if (( ${#missing_manifest[@]} > 0 )); then
    fail "$label — manifest does not declare tune state(s): ${missing_manifest[*]}"
    return
  fi
  pass "$label (${REQUIRED_STATES[*]})"
}

# --- dispatch --------------------------------------------------------------

declare -A ALL_SECTIONS=(
  [design]=check_design
  [dsp-tests]=check_dsp_tests
  [build-artifacts]=check_build_artifacts
  [plugin-validation]=check_plugin_validation
  [ui-evidence]=check_ui_evidence
)
SECTION_ORDER=(design dsp-tests build-artifacts plugin-validation ui-evidence)

usage() {
  sed -n '2,28p' "$0" | sed 's/^# \{0,1\}//'
}

SELECTED=()
for arg in "$@"; do
  case "$arg" in
    --help|-h) usage; exit 0 ;;
    --list) printf "%s\n" "${SECTION_ORDER[@]}"; exit 0 ;;
    --section=*) IFS=',' read -r -a SELECTED <<< "${arg#--section=}" ;;
    *) echo "unknown argument: $arg" >&2; usage >&2; exit 2 ;;
  esac
done

if (( ${#SELECTED[@]} == 0 )); then
  SELECTED=("${SECTION_ORDER[@]}")
fi

printf "Apertune product verifier — %s\n\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)"

for s in "${SELECTED[@]}"; do
  fn="${ALL_SECTIONS[$s]:-}"
  if [[ -z "$fn" ]]; then
    echo "unknown section: $s" >&2
    exit 2
  fi
  "$fn"
done

echo
if (( FAILED )); then
  echo "Apertune product verifier FAILED — MVP Definition of Done not met" >&2
  exit 1
fi
if (( SKIPPED )); then
  echo "Apertune product verifier PASSED with SKIPs"
else
  echo "Apertune product verifier PASSED"
fi
exit 0

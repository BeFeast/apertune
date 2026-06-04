#!/usr/bin/env bash
set -euo pipefail

EXPECTED_DESIGN_SHA="c7caedef1dfaae78d450164bf041fc7f00f63f7632749624000084fed14c6382"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DESIGN_ARCHIVE="${APERTUNE_DESIGN_ARCHIVE:-$REPO_ROOT/design/guitar-tuner-design-2026-06-04.zip}"
BUILD_DIR="${APERTUNE_BUILD_DIR:-$REPO_ROOT/build}"

failures=0
warnings=0

section() {
  printf '\n== %s ==\n' "$1"
}

pass() {
  printf 'PASS: %s\n' "$1"
}

warn() {
  warnings=$((warnings + 1))
  printf 'WARN: %s\n' "$1"
}

fail() {
  failures=$((failures + 1))
  printf 'FAIL: %s\n' "$1"
}

run_shell() {
  local label="$1"
  local command_line="$2"

  printf 'RUN: %s\n' "$command_line"
  if (cd "$REPO_ROOT" && bash -lc "$command_line"); then
    pass "$label"
  else
    fail "$label"
  fi
}

first_existing() {
  local candidate
  for candidate in "$@"; do
    if [[ -e "$REPO_ROOT/$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

section "Design Archive"
if [[ ! -f "$DESIGN_ARCHIVE" ]]; then
  fail "design archive is missing; set APERTUNE_DESIGN_ARCHIVE or place the ignored archive under design/"
else
  actual_sha="$(sha256sum "$DESIGN_ARCHIVE" | awk '{print $1}')"
  if [[ "$actual_sha" == "$EXPECTED_DESIGN_SHA" ]]; then
    pass "design archive SHA-256 matches"
  else
    fail "design archive SHA-256 mismatch: expected $EXPECTED_DESIGN_SHA, got $actual_sha"
  fi
fi

section "DSP And Unit Tests"
if [[ -n "${APERTUNE_DSP_TEST_CMD:-}" ]]; then
  run_shell "DSP/unit tests pass" "$APERTUNE_DSP_TEST_CMD"
elif [[ -f "$REPO_ROOT/CMakeLists.txt" && -d "$BUILD_DIR" ]]; then
  run_shell "CTest suite passes" "ctest --test-dir \"$BUILD_DIR\" --output-on-failure"
elif [[ -f "$REPO_ROOT/package.json" && -x "$REPO_ROOT/node_modules/.bin/vitest" ]]; then
  run_shell "Vitest suite passes" "bunx vitest run"
elif [[ -f "$REPO_ROOT/pyproject.toml" ]]; then
  run_shell "Python test suite passes" "uv run pytest"
else
  fail "DSP/unit test command is not defined yet"
fi

section "Plugin Build"
if [[ -n "${APERTUNE_PLUGIN_BUILD_CMD:-}" ]]; then
  run_shell "plugin build command succeeds" "$APERTUNE_PLUGIN_BUILD_CMD"
elif [[ -f "$REPO_ROOT/CMakeLists.txt" && -d "$BUILD_DIR" ]]; then
  run_shell "CMake plugin build succeeds" "cmake --build \"$BUILD_DIR\" --config Release"
else
  fail "plugin build command is not defined yet"
fi

section "Plugin Artifacts"
clap_artifact="${APERTUNE_CLAP_ARTIFACT:-}"
vst3_artifact="${APERTUNE_VST3_ARTIFACT:-}"
au_artifact="${APERTUNE_AU_ARTIFACT:-}"

if [[ -z "$clap_artifact" ]]; then
  clap_artifact="$(first_existing \
    "build/Apertune.clap" \
    "build/Apertune_artefacts/Release/CLAP/Apertune.clap" \
    "build/Apertune_artefacts/Debug/CLAP/Apertune.clap" || true)"
fi

if [[ -z "$vst3_artifact" ]]; then
  vst3_artifact="$(first_existing \
    "build/Apertune.vst3" \
    "build/Apertune_artefacts/Release/VST3/Apertune.vst3" \
    "build/Apertune_artefacts/Debug/VST3/Apertune.vst3" || true)"
fi

if [[ -z "$au_artifact" ]]; then
  au_artifact="$(first_existing \
    "build/Apertune.component" \
    "build/Apertune_artefacts/Release/AU/Apertune.component" \
    "build/Apertune_artefacts/Debug/AU/Apertune.component" || true)"
fi

if [[ -n "$clap_artifact" && -e "$REPO_ROOT/$clap_artifact" ]]; then
  pass "CLAP artifact exists at $clap_artifact"
else
  fail "CLAP artifact is missing"
fi

if [[ -n "$vst3_artifact" && -e "$REPO_ROOT/$vst3_artifact" ]]; then
  pass "VST3 artifact exists at $vst3_artifact"
else
  fail "VST3 artifact is missing"
fi

if [[ -n "$au_artifact" && -e "$REPO_ROOT/$au_artifact" ]]; then
  pass "AU artifact exists at $au_artifact"
else
  fail "AU artifact is missing"
fi

section "Plugin Validation"
if [[ -n "${APERTUNE_PLUGIN_VALIDATION_CMD:-}" ]]; then
  run_shell "plugin validation passes" "$APERTUNE_PLUGIN_VALIDATION_CMD"
else
  if [[ -n "$clap_artifact" && -e "$REPO_ROOT/$clap_artifact" && "$(command -v clap-validator || true)" ]]; then
    run_shell "CLAP validation passes" "clap-validator validate \"$REPO_ROOT/$clap_artifact\""
  elif [[ -n "$vst3_artifact" && -e "$REPO_ROOT/$vst3_artifact" && "$(command -v validator || true)" ]]; then
    run_shell "VST3 validation passes" "validator \"$REPO_ROOT/$vst3_artifact\""
  else
    warn "no plugin validator is available for discovered artifacts"
  fi
fi

section "UI Evidence"
if [[ -n "${APERTUNE_UI_EVIDENCE_CMD:-}" ]]; then
  run_shell "UI evidence command succeeds" "$APERTUNE_UI_EVIDENCE_CMD"
elif [[ -f "$REPO_ROOT/ui-evidence/apertune-ui-state-report.json" ]]; then
  pass "deterministic UI state report exists"
else
  fail "UI evidence is missing; provide screenshots or ui-evidence/apertune-ui-state-report.json for key tune states"
fi

section "Summary"
printf 'Failures: %s\n' "$failures"
printf 'Warnings: %s\n' "$warnings"

if [[ "$failures" -ne 0 ]]; then
  exit 1
fi

pass "Apertune product verifier passed"

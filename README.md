# Apertune

Apertune is a studio-grade chromatic tuner plugin for guitar and bass.

Product target:

- CLAP / VST3 / AU plugin formats.
- Chromatic monophonic pitch detection.
- Guitar and bass tuning presets.
- Rangefinder-style UI where pitch snaps into focus at the center reticle.

This repository is the implementation checkout. Brand, PRD, issue intake, and Maestro
orchestration are owned by a separate private project-manager workspace, not by this repo.

## Source Of Truth

The product source of truth (brand brief, MVP PRD, Maestro issue wave) lives in the private
project-manager workspace. The approved design archive is provided to workers locally and is
not committed here.

Design archive SHA-256: `c7caedef1dfaae78d450164bf041fc7f00f63f7632749624000084fed14c6382`

## Plugin Scaffold

Apertune is scaffolded as a C++17 / JUCE 8 / CMake audio plugin. CMake fetches JUCE and
the CLAP JUCE wrapper during configure.

Repository layout:

- `CMakeLists.txt` defines the JUCE plugin target and CTest entry point.
- `src/` contains the plugin processor, editor, and pitch math.
- `tests/` contains deterministic pitch math coverage.
- `.github/workflows/ci.yml` builds the Linux plugin path and runs tests.

Supported plugin formats:

- Linux CI/local default: CLAP, VST3, and Standalone.
- macOS local default: CLAP, VST3, Standalone, and AU.

The editor is a production JUCE surface, not a web-only prototype. It carries the approved
Direction A anatomy placeholders: title bar, string row, cents scale, 41-tick track, center
axis, reticle, indicator ball, note row, and footer controls.

## Build

Install CMake, a C++17 compiler, Git, and the platform dependencies required by JUCE. On
Ubuntu, the CI workflow documents the required development packages.

Configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Build CLAP, VST3, and tests:

```bash
cmake --build build --config Release --target Apertune_CLAP Apertune_VST3 apertune_pitch_math_tests --parallel
```

Build all configured formats for the current platform:

```bash
cmake --build build --config Release --target Apertune_All --parallel
```

## Test

Run the pitch math tests through CTest:

```bash
ctest --test-dir build --output-on-failure
```

## Verification

Before opening a PR, run:

```bash
git status --short --branch
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Apertune_CLAP Apertune_VST3 apertune_pitch_math_tests --parallel
ctest --test-dir build --output-on-failure
```

The product verifier lives at `scripts/verify.sh`. It is the single command that asserts the
MVP Definition of Done; it exits non-zero whenever any required check fails. Build-only
success is never enough.

Run it locally:

```bash
scripts/verify.sh                       # all sections
scripts/verify.sh --list                # list section names
scripts/verify.sh --section=design,ui-evidence
```

Sections:

| Section             | Checks                                                                |
| ------------------- | --------------------------------------------------------------------- |
| `design`            | SHA-256 of the design archive at `design/guitar-tuner-design-2026-06-04.zip` |
| `dsp-tests`         | Project DSP/unit tests (`ctest`, `bun test`, or `uv run pytest`)      |
| `build-artifacts`   | CLAP, VST3, and (on macOS) AU artifacts in `$APERTUNE_BUILD_DIR`      |
| `plugin-validation` | `clap-validator`, `pluginval`, and (on macOS) `auval` when on `PATH`  |
| `ui-evidence`       | Screenshot or deterministic report per required tune state            |

Required tune states default to `flat`, `in_tune`, `sharp`. Override with
`APERTUNE_REQUIRED_STATES="flat in_tune sharp lock"`.

To generate placeholder UI evidence from the approved design archive (useful for CI smoke
before live plugin capture exists), run:

```bash
scripts/collect-ui-evidence.sh
```

Placeholder evidence is gitignored; collect it on demand. Replace the collector with a live
plugin capture step as the UI work lands.

### CI

GitHub Actions:

```yaml
- name: Apertune product verifier
  run: scripts/verify.sh
```

The verifier reads the design archive from `design/guitar-tuner-design-2026-06-04.zip`.
Provide it via the same secure delivery channel used locally (the archive is never committed)
or override the path with `APERTUNE_DESIGN_ARCHIVE`.

## License

MIT. See `LICENSE`.

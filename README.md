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

Run the product verifier supplied on the execution host as well. If host plugin validators
such as `pluginval` are available, run them against the built plugin bundle and include the
result in PR evidence. Build-only success is not enough for product `Done`; later verifier
work must also prove plugin validation, pitch math, UI state evidence, and the design archive
checksum.

## License

MIT. See `LICENSE`.

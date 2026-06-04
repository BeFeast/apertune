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

## Bootstrap Status

The initial repo exists to anchor GitHub issues, Maestro worktrees, and the product verifier.
The production plugin scaffold is intentionally not invented in this commit.

Expected next work:

1. Choose and implement the plugin scaffold, defaulting to C++ / JUCE 8.x / CMake unless a documented architecture decision changes it.
2. Add deterministic pitch math and tests.
3. Create a real product verifier.
4. Port the approved Direction A UI anatomy.

## Verification

Run the product verifier with:

```bash
scripts/apertune-product-verifier.sh
```

The verifier is intentionally broader than a compile check. It gates the MVP Definition of
Done by checking:

- approved design archive SHA-256;
- DSP/unit tests;
- CLAP, VST3, and AU build artifacts;
- plugin validation when a validator is available;
- UI screenshot evidence or a deterministic UI state inspection report for key tune states.

Until the plugin scaffold exists, the verifier is expected to fail non-zero and report the
missing proof. The design archive is ignored by git; place it under `design/` for local runs
or set `APERTUNE_DESIGN_ARCHIVE` to the read-only archive path supplied by the execution host.

Repo-local CI should run the same script after restoring the design archive and installing the
plugin toolchain. Scaffold-specific jobs may provide:

- `APERTUNE_DSP_TEST_CMD`
- `APERTUNE_PLUGIN_BUILD_CMD`
- `APERTUNE_PLUGIN_VALIDATION_CMD`
- `APERTUNE_UI_EVIDENCE_CMD`

Artifact locations can be overridden with `APERTUNE_CLAP_ARTIFACT`,
`APERTUNE_VST3_ARTIFACT`, and `APERTUNE_AU_ARTIFACT`.

## License

MIT. See `LICENSE`.

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

Until the plugin scaffold exists, the verifier is a bootstrap guard. It is expected to fail
until build/test/validation commands are implemented.

## License

MIT. See `LICENSE`.

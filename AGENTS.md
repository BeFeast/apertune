# AGENTS.md - Apertune Repository

This is the implementation repository for Apertune, a CLAP / VST3 / AU chromatic tuner plugin.

Brand, PRD, issue intake, Maestro orchestration, and verifier policy are owned by a separate
private project-manager workspace. Read that workspace's `AGENTS.md` before changing
orchestration, issue intake, Maestro config, verifier policy, or product scope.

## Execution Surface

- Development, builds, tests, and validation run on the designated execution host, not in a local reference checkout.
- If the implementation checkout and the project-manager workspace disagree, the implementation checkout is authoritative for code and the project-manager workspace is authoritative for orchestration and spec state.
- Do not write personal, infrastructure, execution-host, worktree, or orchestration paths into tracked files, commit messages, or PR text. Refer to those locations abstractly.

Before editing or testing:

    git status --short --branch

## Product Contract

- Apertune is a chromatic tuner plugin for guitar and bass.
- The approved UI metaphor is camera aperture/rangefinder focus lock.
- The design archive is specification, not inspiration.
- Do not create a demo-only tuner or web-only prototype unless the issue explicitly asks for one.
- Preserve CLAP / VST3 / AU targets unless a later architecture note changes the release scope.

## Source Artifacts

The approved design archive is provided to workers from the private workspace and is not
committed to this repo. Expected SHA-256:

    c7caedef1dfaae78d450164bf041fc7f00f63f7632749624000084fed14c6382

Approved visual reference inside the archive: `Tuner - Studio.html`.

## Work Intake

For non-trivial work:

1. Work from a GitHub issue.
2. Keep PRs small and focused.
3. Include acceptance evidence in the PR.
4. Stop after opening the PR.
5. Do not merge unless explicitly asked.

## Tooling Defaults

- Use `uv` for Python commands.
- Use `bun` for Node/frontend commands.
- Use existing repo tooling once scaffolded.
- Do not introduce secrets or local generated artifacts into git.

## Verification

Before opening a PR, run the repo-local build and test path:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target Apertune_CLAP Apertune_VST3 apertune_pitch_math_tests --parallel
    ctest --test-dir build --output-on-failure

The single product verifier is `scripts/verify.sh`; it exits non-zero whenever the MVP
Definition of Done is not met. Build-only success is never enough.

The verifier must eventually prove:

It runs, in order:

1. `design` — SHA-256 of the design archive.
2. `dsp-tests` — DSP / unit tests via the project's test runner.
3. `build-artifacts` — CLAP, VST3, and (on macOS) AU artifacts under `$APERTUNE_BUILD_DIR`.
4. `plugin-validation` — `clap-validator` / `pluginval` / `auval` where available.
5. `ui-evidence` — screenshot or deterministic report per required tune state.

Build-only success is not enough for product `Done`, and UI work is never marked done
without visible evidence for the key tune states (default: `flat`, `in_tune`, `sharp`).

Run subsets with `scripts/verify.sh --section=NAME[,NAME...]`. UI evidence can be (re)generated
from the design archive with `scripts/collect-ui-evidence.sh`; that is a Phase 0/1 placeholder
and should be replaced once the plugin scaffold can render live captures.

See `README.md` for full usage and CI wiring.

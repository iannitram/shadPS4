# Changelog (our pipeline)

Newest first. One line per change of stage, release or decision that the next session needs.

- 2026-11-02 Roadmap (ROADMAP.md, docs/code-map.md): the games consume the core as a submodule pinned to a `gpucore-vX.Y.Z` tag, built with `add_subdirectory(<fork>/gpucore)`. Skeleton committed: `gpucore/`, `tools/pkg-extract/`, the key file parser stub, `tools/fork-delta/`, `docs/fork-delta.md`, and `fork-core.yml` (CI on Linux, Windows and macOS); both skeletons build and pass their tests on Linux.
- 2026-11-02 Spec approved (SPEC.md): this fork becomes the shared PS4 graphics core for Uncharted, Wipeout, Ratchet & Clank and The Last Guardian, plus the package extractor on Linux, Windows and macOS. Stage Parked -> Spec -> Roadmap.
- 2026-10-26 Normalized to pipeline v2 at Parked (paused).
- 2026-10-26 Merged our feat/pkg-extract (Linux --extract-pkg port) into main.

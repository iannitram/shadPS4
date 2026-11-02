# Changelog (our pipeline)

Newest first. One line per change of stage, release or decision that the next session needs.

- 2026-11-02 My rulings: the shared core is optional for the games; 1.0 is a tagged release any PS4 -universal game can build from, proven by Uncharted-Universal (SPEC 1.0 line and M8, ROADMAP, code map, #41). Shadow of the Colossus (SotC-Universal, a PS4 port) joins the list of games that can build from it, as optional.
- 2026-11-02 Dev Loop: 39 issues (#4-#42) cover the 1.0 code map. Stage 2 is five Easy issues with Qwen briefs and locked tests (#4 C1 config check, #5 C2 hook registry, #6 E1 command line, #7 E3a key file, #8 R3 fork-delta); each locked test file was run red against the stubs and green against a throwaway implementation on Linux. Stage Roadmap -> Dev Loop.
- 2026-11-02 Roadmap (ROADMAP.md, docs/code-map.md): the games consume the core as a submodule pinned to a `gpucore-vX.Y.Z` tag, built with `add_subdirectory(<fork>/gpucore)`. Skeleton committed: `gpucore/`, `tools/pkg-extract/`, the key file parser stub, `tools/fork-delta/`, `docs/fork-delta.md`, and `fork-core.yml` (CI on Linux, Windows and macOS); both skeletons build and pass their tests on Linux.
- 2026-11-02 Spec approved (SPEC.md): this fork becomes the shared PS4 graphics core for Uncharted, Wipeout, Ratchet & Clank and The Last Guardian, plus the package extractor on Linux, Windows and macOS. Stage Parked -> Spec -> Roadmap.
- 2026-10-26 Normalized to pipeline v2 at Parked (paused).
- 2026-10-26 Merged our feat/pkg-extract (Linux --extract-pkg port) into main.

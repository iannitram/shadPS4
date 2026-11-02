# Roadmap: our shadPS4 fork to 1.0

1.0 (SPEC.md): a tagged release of `gpucore/` that any PS4 `-universal` game can build from on Mac, PC and Linux, proven by Uncharted-Universal building from it, and `shadps4-pkg` unpacking a self-made package on all three. The other games' switch-overs are optional work in their own repos (my ruling, 2026-11-02). Upstream's own plan is theirs (SPEC.md); this roadmap covers only our delta. Modules and rules: `docs/code-map.md`. Issue ids below are roadmap ids; each issue's title starts with one.

## Features (1.0)

| Id | Feature | Spec | Linux | Windows | macOS |
|---|---|---|---|---|---|
| C | The core as a library a game adds with one `add_subdirectory` | M1 | GCC/Clang | MSVC and MSYS2 clang | Apple clang, MoltenVK |
| C | One versioned C interface and config check | M2 | yes | yes | yes |
| C | Per-game hook table and its call sites; the upscaler hands-off point | M3 | yes | yes | yes |
| C | Host seam: the loader provides memory, faults, logging | M1 | yes | yes | yes |
| T, F | The general fixes from the four games' copies, triaged and brought in once | M4 | yes | yes | yes |
| R | Tagged releases, the fork-delta check, our CI on three systems | M5, M6, M9 | yes | yes | yes |
| E | `shadps4-pkg` and `--extract-pkg` with a run-time key file and generated test packages | M7 | yes | yes | yes |
| G | Each game switches to the pinned core (game repos' own issues) | M8 | yes | yes | yes |

## Code map and skeleton

The code map is `docs/code-map.md`. The skeleton is committed: `gpucore/` (library target, the three public headers, stub implementation, a GoogleTest harness that runs, a C compile check), `tools/pkg-extract/` (CLI parser stub, `shadps4-pkg` binary, harness), `src/core/file_format/pkg_keyfile.*` (key file parser stub), `tools/fork-delta/` (check stub), `docs/fork-delta.md`, and `.github/workflows/fork-core.yml` (builds and tests both skeletons on GitHub-hosted Linux, Windows and macOS, plus the no-game-code grep).

## How the games consume the core (decision)

**A git submodule pinned to a `gpucore-vX.Y.Z` tag, built with `add_subdirectory(third_party/shadPS4/gpucore)`.** Why:

- The four games already use submodules (`.gitmodules` in each), so nothing new to learn and nothing to install.
- A pin is one line in a PR: moving a game to a new core is reviewable and revertable, and each game moves when it is ready.
- The build hubs build offline; FetchContent or a package manager would need the network at configure time, and a prebuilt binary would need per-OS release builds of a library whose ABI is still at 0.x.
- `add_subdirectory` on our own `gpucore/CMakeLists.txt` keeps upstream's top-level CMake out of the games' builds and out of our merge conflicts.

A game whose repo is private can pin this public repo with no extra access. Each game keeps building its current copy until its switch-over issue lands; nothing breaks on our side first.

## Open-source candidates (from the prior art)

| Project | License | Where it fits | How |
|---|---|---|---|
| deadinside28/bloodborne_pc | GPL-2.0 | The bbport template: what a port takes from the core; its `bbgpu` interface is the model for ours | ideas and interface shape; code under GPL-2.0 with credit |
| bloodborne-native/bloodborne-runtime | GPL-2.0 | Its split of loader, PS4 libraries and renderer into one repo is the closest prior art for `gpucore/` and the host seam | learn; code with credit where it fits |
| bloodborne-native/bloodborne-mac | GPL-2.0 | How the Mac side builds MoltenVK and packages on Apple silicon (C5) | learn |
| bloodborne-native/bbcpu | GPL-2.0 | The CPU half on arm64: outside the core (Won't), but the host seam must not block it | learn |
| AzaharPlus/shadPS4Plus | GPL-2.0 | Standalone extractor from shadPS4's old code: direct prior art for E4 | adopt or learn, with credit |
| OpenOrbis/LibOrbisPkg | LGPL-3.0 | PKG/SFO/PFS reference for the test-package builder (E5) | link and learn only, no code copied |
| obhq/obliteration | Apache-2.0 | A PKG and filesystem reader in Rust: edge cases for E5 | ideas only |
| diegolix29/shadPS4, junminlee2004/GR2fork, GetLostIn/CTR-NitroFueled-PC | GPL-2.0 | How other forks keep game fixes apart from core fixes and rebase on upstream (T, F, R) | weekly peer review, `from-peer` issues |
| red-prig/fpPS4 | LGPL-2.1 | A second view of how GPU work splits into parts | ideas only |
| shadps4-emu/shadPS4 | GPL-2.0 | Upstream: merged daily, never posted to | the merge |

## Dev stages

Each stage lists its issues by roadmap id; numbers and `Blocked by` lines are in the issues.

1. **Skeleton and CI** (this commit). `fork-core.yml` builds both skeletons on all three systems.
2. **Interfaces (Easy, Qwen).** C1 ABI version and config check; C2 hook registry; E1 `shadps4-pkg` command line; E3a key file parser; R3 fork-delta compare.
3. **Core build on three systems.** C3 the upstream subset compiles into `shadps4_gpucore` on Linux; C4 on Windows (MSVC and MSYS2 clang); C5 on macOS with MoltenVK; C9 the consumer smoke project in CI.
4. **Core at run time.** C6 host seam; C7 init, resolve, fault, shutdown; C8 hook call sites; C11 render scale and the upscaler hand-off.
5. **Triage and general fixes.** T1-T4 one triage per game; then F1 shader recompiler, F2 caches and page tracking, F3 Vulkan renderer, F4 command processor and common; C10a-d the general shim pieces (diagnostics, settings store, overlay, movie bridge).
6. **Extractor on three systems.** E2 the unpacking loop moves out of `main.cpp`; E3b run-time keys in the reader; E4 standalone `shadps4-pkg` on Linux; E5a/E5b generated test packages and the round trip; E6 Windows; E7 macOS.
7. **Release.** R2 full CI on three systems (Vulkan SDK, MoltenVK); R4 fork-delta check in CI; R5 release tags; R7 our files pass upstream's REUSE check. Cut `gpucore-v0.1.0` when stages 3-4 are merged; Uncharted-Universal's switch-over starts from it, and any other game's optional switch-over can too.
8. **Design pass (last, R04).** Only the in-game overlay (C10c) is UI here; the design pass runs on it, against the one shared design system, and files `design-pass` issues. The extractor is a command-line tool and skips it.

## Cross-repo notes: what the games need (their own issues, not filed here)

Uncharted-Universal's switch-over is needed for 1.0 (SPEC.md M8). For Wipeout-Universal, Ratchet-Clank-Universal, TheLastGuardian-Universal and SotC-Universal it is optional work in their own repos, after 1.0; each may keep and customise its own copy instead (my ruling, 2026-11-02). A game that switches does this:

1. **Triage detail:** T1-T4's triage files here list every changed file and mark it general, per-game or dropped (SPEC.md M4). A per-game line names only the file; what the change does and why (shader patches, title checks, menus) is written in the game repo, since it can carry game-binary analysis and this repo is public.
2. **Move per-game changes behind the hooks** (`shadps4_gpucore_hooks.h`), so the vendored copy holds no per-game edit.
3. **Add the submodule** `third_party/shadPS4` pinned at `gpucore-v0.1.0`; build with `add_subdirectory(third_party/shadPS4/gpucore)` and link `shadps4::gpucore`.
4. **Replace the game's C header** (`bbgpu.h`, `gow3gpu.h`, `tlggpu.h`) with `shadps4_gpucore.h` (a thin compatibility header is fine during the move), and fill `ShadPs4GpuCoreHost` from the game's loader.
5. **Delete the vendored copy** (`gpu/shadps4`, `port/gpu/shadps4` for The Last Guardian) and the shim files the core now provides.
6. **CI on Linux, Windows and macOS** builds against the pin.
7. **Boot check** on each system with my own dump (`needs-owner-device`).
8. **Android and iPhone/iPad builds** keep their current route until a core 1.x adds those targets (SPEC.md Won'ts).

FSR-Vulkan: an issue there to plug into the core's present path (C11) instead of each game's copy.

## Mac / PC / Linux in every part

Every core and extractor issue builds on all three in `fork-core.yml`; platform-only work carries a platform label (C3 Linux, C4 Windows, C5 macOS, E6 Windows, E7 macOS). Device checks with my own packages and games are `needs-owner-device`.

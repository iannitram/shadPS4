# Spec: our shadPS4 fork

**1.0 means:** a tagged release of this fork's shared PS4 graphics core, which any PS4 `-universal` game can build from on Mac, PC (Windows) and Linux, proven by at least one game (Uncharted-Universal) building from it, plus the package extractor (`shadps4-pkg`) unpacking a self-made package on all three. The games that can build from it, each by its own choice: Uncharted, Wipeout, Ratchet & Clank, The Last Guardian and Shadow of the Colossus (iannitram/SotC-Universal, a PS4 port). For every game but Uncharted the switch-over is optional work in its own repo; a game may keep and customise its own copy.

**Approved by me:** 2026-11-02, in the Project Manager session's question tool. Rulings: "Shared PS4 base for the games (Recommended)" (this fork becomes the one shared PS4 graphics core the PS4 `-universal` games build from, so a fix lands once, plus the package extractor); "All games build from it (Recommended)" (1.0 = the four games pull their graphics code from this fork on Mac, PC and Linux, and the extractor works on all three; each game's switch-over is that game repo's own issue); prior art "Approve all 12 (Recommended)". Later the same day I made the shared core optional for the games: "I think each will become custom over time. This doesn't need to be a rule." and "Make it optional (Recommended)" (core 1.0 = a tagged release any game can use). So 1.0 and M8 now need one game (Uncharted-Universal) building from the release, not all four; and on SotC, "Yes, if it's a PS4 port (Recommended)": it is, so it joins the list as optional. Same day, on M4's triage: "Uncharted only for 1.0 (Recommended)": before 1.0 only Uncharted-Universal's copy is triaged (T1); the other games' triage (T2-T4) is optional, done when each game chooses to switch over. These changes are my own rulings, so the spec stays approved. Standing rule: upstream is anti-AI, so I push only to this fork and never open PRs, issues or comments upstream.

A project is Spec'd only when the line above has a date. Change a Must or a Won't after approval and the approval line goes back to "not yet" until I approve again.

## A fork: upstream's roadmap plus our delta (R18)

Upstream ([shadps4-emu/shadPS4](https://github.com/shadps4-emu/shadPS4)) is a PS4 emulator. It publishes no written roadmap: its plan is its [issues](https://github.com/shadps4-emu/shadPS4/issues), [pull requests](https://github.com/shadps4-emu/shadPS4/pulls), [releases](https://github.com/shadps4-emu/shadPS4/releases) and the [game compatibility list](https://github.com/shadps4-compatibility/shadps4-game-compatibility). We take all of that through the daily upstream merge and build none of it ourselves. Our issues and code map cover only the delta below.

Upstream asks for no AI-written contributions, so every row is "keep on our fork": nothing is offered upstream (my standing rule; any exception needs my yes per offer).

| # | Delta | Where | Upstream? |
|---|---|---|---|
| D1 | The shared graphics core: a library target (`shadps4_gpucore`) built from upstream's video core, shader recompiler, common code and the Gnm/VideoOut libraries, for a game's native loader instead of the emulator | `gpucore/` | keep on our fork |
| D2 | A stable, versioned C interface for the games (the generalized `bbgpu_*` interface the bbport-style ports use today) | `gpucore/include/` | keep on our fork |
| D3 | Per-game hooks: the only way a game changes core behaviour (shader patches by hash, submit and present callbacks, settings defaults), so per-game fixes stay in the game repos and general fixes stay here | `gpucore/include/`, call sites in `src/video_core` | keep on our fork |
| D4 | The general fixes now scattered across the games' copies, triaged and brought in once (Uncharted-Universal's for 1.0, the others when each game switches over) | `src/video_core`, `src/shader_recompiler`, `src/common`, `gpucore/src/host` | keep on our fork |
| D5 | Tagged core releases the games pin (`gpucore-vX.Y.Z`) and CI on Linux, Windows and macOS | `.github/workflows/fork-core.yml`, `gpucore/VERSION` | keep on our fork |
| D6 | The package extractor: upstream removed .pkg unpacking after 0.7.0; ours restores it for self-made (fake) packages, as `--extract-pkg` in the emulator and as the standalone `shadps4-pkg` tool, on all three systems | `src/core/file_format/pkg*`, `tools/pkg-extract/` | keep on our fork |

## Musts

Device checks are `needs-owner-device`: they need my machines and my own packages or dumps.

| # | Must | Done looks like | Check |
|---|---|---|---|
| M1 | WHEN a game repo adds this fork as a submodule and calls `add_subdirectory(<fork>/gpucore)`, IT SHALL get the `shadps4::gpucore` static library and its headers, with nothing else from the emulator built | A consumer project with only that line links and runs the core's smoke test | the consumer smoke test (`gpucore/tests/consumer`) in CI on Linux, Windows and macOS |
| M2 | THE CORE SHALL keep one versioned C interface (`shadps4_gpucore.h`): a change that breaks a caller raises the major version, and a game can ask the library which version it was built as | The header's version macro and `shadps4_gpucore_abi_version()` agree; a game built against an older major refuses to start with a plain message | ABI unit tests; the consumer smoke test |
| M3 | THE CORE SHALL hold no per-game code: a game changes core behaviour only through the hook table and the config it passes at start | `git grep -i -E "uncharted|wipeout|ratchet|guardian|CUSA[0-9]"` over `gpucore/`, `src/video_core/` and `src/shader_recompiler/` finds nothing outside comments that cite a source | that grep as a CI step; hook unit tests |
| M4 | THE GENERAL FIXES in Uncharted-Universal's copy SHALL be in the core before 1.0; another game's copy is triaged the same way when that game chooses to switch over (optional, after 1.0; my ruling, 2026-11-02: "Uncharted only for 1.0") | Uncharted-Universal has a triage file listing every file its copy changed against its vendor base, each marked general (brought in here, with the PR), per-game (moves behind a hook, in the game repo) or dropped (upstream already has it) | `docs/gpucore-triage/uncharted.md` is complete and every "general" line links a merged PR; any other game's triage file meets the same bar when it is written |
| M5 | EACH CORE RELEASE SHALL be a tag `gpucore-vX.Y.Z` on `main` whose `gpucore/VERSION` matches, with a line in `CHANGELOG.fork.md` | A game moves to a new core by changing one submodule pin | the release check in CI; the tag list |
| M6 | THE DAILY UPSTREAM MERGE SHALL stay cheap: every upstream file we edit is listed with why | `docs/fork-delta.md` lists each upstream file that differs from `upstream/main`; a merge conflict becomes an `upstream-sync` issue, taken first (R21) | the delta report step in CI fails when an unlisted upstream file differs |
| M7 | WHEN I run `shadps4-pkg <package> --keys <my key file> [--out <dir>] [--threads N]` on Linux, Windows or macOS, IT SHALL unpack the package into `<dir>` (default `./<title id>`), refuse a non-empty output folder, refuse a thread count outside 1-32, and print a plain message for a missing or wrong key file | A self-made package unpacks the same on all three; the same package through the emulator's `--extract-pkg` gives the same files | round-trip tests with packages the tests build from their own generated keys, in CI on all three; one real extraction of my own package per system (`needs-owner-device`) |
| M8 | UNCHARTED-UNIVERSAL SHALL build its graphics from a pinned core release on Mac, PC and Linux, proving the release works for a game; any other PS4 `-universal` game (Wipeout, Ratchet & Clank, The Last Guardian, Shadow of the Colossus) MAY build from it too, as optional work in its own repo (my ruling, 2026-11-02) | Uncharted-Universal's own CI builds against the pinned core on all three; its vendored shadPS4 copy is gone | Uncharted-Universal's switch-over issue (cross-repo notes in `ROADMAP.md`); a boot of the game on each system (`needs-owner-device`) |
| M9 | EVERY MERGE TO `main` SHALL build and test the core and the extractor on Linux, Windows and macOS | `fork-core.yml` green on GitHub-hosted runners (a public repo, R29) | CI green |
| M10 | NO GAME FILES, EVER | Nothing from a dump, a package, firmware or keys is in this repo, a release, a log, a test or an issue; key files are always mine and supplied at run time | review of every PR; the release file list; test packages are generated by the tests |

## Won'ts (1.0)

- Rewriting shadPS4, or any part of it, in Rust. The games' own code may be Rust; the core stays C++ behind its C interface.
- Posting anything upstream: no PRs, issues, comments or reports to shadps4-emu (anti-AI). We take their work through the merge only.
- Game data, keys or firmware in the repo, a release, a log, a test or an issue, ever. That includes keys compiled into a released binary: releases read my key file at run time.
- Per-game code in the core (shader patches, title checks, game menus): those stay in the game repos behind the hooks.
- Upstream's own roadmap work (new HLE libraries, compatibility fixes for other games) as our issues; we get it by merging.
- Changing how upstream's emulator app behaves, beyond the `--extract-pkg` option.
- Android, iPhone and iPad builds of the core, and the x86-64-on-arm64 CPU half (bbcpu): the games keep their own route for those until a core 1.x adds them.
- Making packages, signing packages, or reading retail (non-fake) packages.
- Online play features beyond what upstream's code already does.

## Prior art

Projects doing the same thing, approved by me at Spec ("Approve all 12 (Recommended)", 2026-11-02) and reviewed weekly. Their code comes in only when their license allows it, with credit; otherwise only their ideas.

peer: deadinside28/bloodborne_pc | GPL-2.0 | the bbport template our ports follow; shows exactly what a port takes from the core
peer: bloodborne-native/bloodborne-runtime | GPL-2.0 | loader, PS4 libraries and Vulkan renderer split into their own repo: closest prior art for one shared core
peer: bloodborne-native/bloodborne-mac | GPL-2.0 | native Apple Silicon app on bbport and shadPS4: how the Mac side builds and packages
peer: AzaharPlus/shadPS4Plus | GPL-2.0 | standalone PKG extractor from code once in shadPS4: direct prior art for our extractor
peer: shadps4-emu/shadPS4 | GPL-2.0 | upstream: GPU core, CMake layout and release cadence set our rebase cost (we never post there)
peer: bloodborne-native/bbcpu | GPL-2.0 | x86-64 PS4 code on arm64: the CPU half Apple Silicon ports need beside the GPU core
peer: OpenOrbis/LibOrbisPkg | LGPL-3.0 | PKG/SFO/PFS reference and edge cases (link, don't copy code in)
peer: diegolix29/shadPS4 | GPL-2.0 | the largest active fork: performance and compatibility patches, how it keeps rebasing
peer: junminlee2004/GR2fork | GPL-2.0 | game-focused fork: keeping game-specific fixes apart from core fixes
peer: GetLostIn/CTR-NitroFueled-PC | GPL-2.0 | another bbport-style port: what one game needed changed in the core
peer: red-prig/fpPS4 | LGPL-2.1 | independent PS4 emulator: a second view of how GPU work splits into parts
peer: obhq/obliteration | Apache-2.0 | PS4 emulator in Rust: ideas for a PKG and filesystem reader

## Problem

Each of my four PS4 game ports carries its own copy of shadPS4's graphics code, all taken from the same upstream commit (`b4e7ae7`, 2026-10-02) and then patched separately. Of the files all four copies share, about 107 in the video core, 44 in the shader recompiler, 12 in core and 9 in common now differ between the games. A fix found in one game has to be found again, or copied by hand, in the other three, and each copy is about 280 upstream commits behind.

## Solution

One core, here, that any of the games can use. A game that opts in pins a tagged release of this fork as a submodule and builds only the graphics library from it; a game may instead keep and customise its own copy (my ruling, 2026-11-02). General fixes land once, here; per-game behaviour lives in each game repo and reaches the core only through hooks. The daily upstream merge keeps the core current for every game that pins it. The package extractor ships beside it as a small tool on all three systems.

## User stories

1. As the owner of four ports, I want a graphics fix made once to reach every game, so that I stop fixing the same bug four times.
2. As a game port, I want a stable C interface and a hook table, so that I can move to a new core release by changing one pin.
3. As a person with my own self-made package, I want to unpack it on Linux, Windows or macOS with one command, so that I can prepare a game on whichever machine I am at.

## Implementation decisions

- The core is consumed as a git submodule pinned to a release tag, not FetchContent or a binary package: the games already use submodules, a pin is reviewable in a PR, and the build hubs build offline.
- The core's build lives in its own folder with its own CMake file, pointing at upstream's source tree, so upstream's top-level `CMakeLists.txt` stays untouched and the daily merge does not conflict there.
- The host seam (memory, window and surface, settings, logging, page faults) is one interface the game's loader implements; the core never assumes the emulator's kernel.
- The extractor's keys come from a file I supply at run time; tests build their own packages from keys they generate.

## Testing decisions

- GoogleTest, as upstream's `tests/` uses. The core's tests live in `gpucore/tests/` and build without a GPU (ABI, hooks, config, the consumer smoke project); the extractor's in `tools/pkg-extract/tests/`.
- Test packages are generated by the tests (their own RSA and AES keys), never committed and never real.

## Further notes

- Platforms: Linux, Windows and macOS (Apple silicon, through MoltenVK) for every part.
- FSR-Vulkan becomes the games' shared upscaler; the core exposes the hook it plugs into but does not vendor it.

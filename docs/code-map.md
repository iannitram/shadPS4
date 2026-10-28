# shadPS4 code map

- **Repo:** iannitram/shadPS4 (public fork). **Kind:** C++ (CMake, Ninja) PlayStation 4 emulator with SDL3, ImGui and a Vulkan renderer; targets Windows, macOS and Linux upstream.
- **Upstream:** shadps4-emu/shadPS4 (`upstream/main`). **Integration branch:** `main`. Upstream asks for no AI-assisted contributions: push only to this fork, never post upstream.
- **Describes commit:** `c43fc34b` (2026-10-27).

shadPS4 loads PS4 ELF/SELF modules, links them against its own high-level implementations of the PS4 system libraries (`src/core/libraries`), emulates the kernel, memory and threading model, recompiles GCN shaders to SPIR-V, and renders through Vulkan. Upstream owns almost everything here. Our one code change is a Linux `--extract-pkg` command, which restores the .pkg unpacking that upstream removed after 0.7.0.

## Layout

| Path | What it owns |
|---|---|
| `src/main.cpp`, `src/emulator.*` | Entry point and CLI (CLI11), emulator startup; `src/sdl_window.*` (SDL window, `.mm` variant for Apple) |
| `src/core/` | Emulation core: `linker`, `module`, `memory`, `address_space`, `tls`, `signals`, `cpu_patches`, settings (`emulator_settings.*`, `user_settings.*`, `user_manager.*`), `debugger`, `devtools/`, `ipc/`, `net/`, `aerolib/` (symbol stubs), `loader/` (ELF/SELF), `file_sys/`, `file_format/` (psf, pfs, trp, rif, npbind, playgo, and our pkg files) |
| `src/core/libraries/` | HLE of the PS4 system libraries, one folder per library: kernel, libc_internal, gnmdriver, videoout, audio and audio3d, ngs2, np, net, network, save_data, pad, usbd, avplayer, videodec, font, and about 50 others |
| `src/video_core/`, `src/shader_recompiler/` | GPU: command processor, AMDGPU register state, buffer and texture caches, Vulkan renderer; GCN to SPIR-V shader recompiler (frontend, IR, backend) |
| `src/common/`, `src/input/`, `src/imgui/`, `src/shadnet/`, `src/dist/`, `src/resources/` | Utilities and logging; input mapping; in-game and devtools UI; shadNet client (protobuf) and server probe; distribution files; embedded resources |
| `externals/` | Third-party libraries as submodules or in-tree (about 50: fmt, spdlog, sdl3, vulkan headers and loader, vma, glslang, sirit, zydis, libressl, protobuf, ffmpeg-core, openal-soft, ImGui, and others) |
| `tests/` | GoogleTest unit tests (`ENABLE_TESTS`): `gcn/` (shader translator), `network/`, `stubs/`, emulator settings, NGS2 |
| `cmake/`, `CMakeLists.txt`, `CMakePresets.json`, `CMakeSettings.json` | Build system: Find modules, per-OS presets, the single source list |
| `.github/workflows/build.yml` | CI (see below); `documents/` holds build guides for Linux, macOS, Windows and Docker, shader patching, debugging, the changelog |
| `scripts/`, `flake.nix`, `flake.lock`, `REUSE.toml`, `LICENSES/` | Symbol-name tooling (`ps4_names2stubs.py`, `aerolib.inl`) and file-format notes; Nix flake; REUSE licensing data |
| `README.md`, `CONTRIBUTING.md`, `AGENTS.md`, `LICENSE` | Upstream's project files (GPL-2.0-or-later) |
| `AGENTS.fork.md`, `CHANGELOG.fork.md`, `PIPELINE.md` | Our rules for this fork, change history, stage record |

## Where our changes live

`git log upstream/main..HEAD` is three commits (upstream has three newer commits we have not merged). Against the merge-base the delta is ten files, about 1460 added lines, nothing removed from upstream code:

| File | Change |
|---|---|
| `src/core/file_format/pkg.cpp`, `pkg.h`, `pkg_type.cpp`, `pkg_type.h` | New: the 0.7.0 package reader brought back for fake (self-made) packages. OpenSSL/LibreSSL replaces CryptoPP (AES-128-CBC and XTS, HMAC-SHA256, SHA-256, RSA), sizes and offsets are 64-bit, bad keys or blocks are errors. The key sets are not in the repo: `pkg_keys.h` is git-ignored, and without it the build still compiles and the command reports the file missing |
| `src/main.cpp` | New CLI options `--extract-pkg <pkg>`, `--extract-dir <dir>` (must be empty or absent; default `./<title id>`), `--extract-threads N` (1 to 32, default 4), and `RunPkgExtract`, which unpacks with worker threads and exits before the emulator starts |
| `CMakeLists.txt` | Adds the four pkg files to the `CORE` source list |
| `.gitignore` | Ignores `src/core/file_format/pkg_keys.h` |

This is the port that lived on branch `feat/pkg-extract`. The branch was merged into `main` on 2026-10-26 and is no longer on this fork's remote. The other three files are our pipeline files.

## Build and test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release      # add -DENABLE_TESTS=ON for the unit tests
cmake --build build --parallel
ctest --test-dir build                                        # when built with tests
./build/shadps4 --extract-pkg game.pkg --extract-dir out/     # our option
```

- Per-platform dependency lists and preset use are in `documents/building-linux.md`, `building-macos.md`, `building-windows.md`. Upstream's Linux guide uses clang 23.
- CI (`.github/workflows/build.yml`, name "Build and Release"): REUSE and clang-format checks, C++ tests on a Linux, macOS and Windows matrix (Debug, `-DENABLE_TESTS=ON`), and release builds `windows-sdl`, `macos-sdl`, `linux-sdl`, `linux-sdl-gcc`.

## Mac / PC / Linux

- Upstream builds and releases for Windows, macOS and Linux and tests on all three in CI. Our `--extract-pkg` port is covered only on Linux (PIPELINE.md: platforms linux); the code is plain C++ with OpenSSL and `std::filesystem`, but it has not been tried elsewhere.
- Per R04, which machine can build what is not recorded for this repo; a Mac session can read and edit the files and may configure the project only with the toolchain from the macOS guide.

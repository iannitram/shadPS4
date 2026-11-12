# shadPS4 fork: code map

- **Repo:** iannitram/shadPS4 (public fork). **Kind:** C++ (CMake, Ninja) PlayStation 4 emulator with SDL3, ImGui and a Vulkan renderer; upstream builds Windows, macOS and Linux.
- **Upstream:** shadps4-emu/shadPS4 (`upstream/main`). **Integration branch:** `main`. Upstream asks for no AI-assisted contributions: push only to this fork, never post upstream.
- **Our job here (SPEC.md):** the shared PS4 graphics core the four PS4 games build from (`gpucore/`), and the package extractor (`--extract-pkg`, `tools/pkg-extract/`). Upstream owns everything else; we take it through the daily merge. Plan and stages: `ROADMAP.md`.
- **Describes commit:** the roadmap commit of 2026-11-02 (upstream merged up to `ed8a479e`).

Status per module: **present** (built and tested), **skeleton** (files and stubs, tests run), **planned** (not started). A PR that builds a module moves its status in the same PR.

## Upstream's layout (theirs, unchanged)

| Path | What it owns |
|---|---|
| `src/main.cpp`, `src/emulator.*` | Entry point and CLI (CLI11), emulator startup; `src/sdl_window.*` (SDL window, `.mm` variant for Apple) |
| `src/core/` | Emulation core: `linker`, `module`, `memory`, `address_space`, `tls`, `signals`, `cpu_patches`, settings, `debugger`, `devtools/`, `ipc/`, `net/`, `aerolib/` (symbol stubs), `loader/` (ELF/SELF), `file_sys/`, `file_format/` (psf, pfs, trp, rif, npbind, playgo, and our pkg files) |
| `src/core/libraries/` | HLE of the PS4 system libraries, one folder per library: kernel, gnmdriver, videoout, audio, ngs2, np, net, save_data, pad, avplayer, videodec, font and about 50 others |
| `src/video_core/` | GPU: `amdgpu/` (Liverpool command processor, register state), `buffer_cache/`, `texture_cache/`, `page_manager.*`, `renderer_vulkan/` (instance, scheduler, presenter, pipeline cache, rasterizer) |
| `src/shader_recompiler/` | GCN to SPIR-V: `frontend/` (decoder, translator), `ir/` (passes), `backend/spirv/` |
| `src/common/`, `src/input/`, `src/imgui/`, `src/shadnet/` | Utilities and logging; input mapping; in-game and devtools UI; shadNet client |
| `externals/` | Third-party libraries as submodules or in-tree (sirit, glslang, vma, vulkan headers, sdl3, fmt, zydis, libressl, ffmpeg-core, ImGui and others) |
| `tests/` | Upstream's GoogleTest unit tests (`ENABLE_TESTS`): `gcn/`, `network/`, `stubs/`, settings, NGS2 |
| `CMakeLists.txt`, `CMakePresets.json`, `cmake/` | Upstream's build: one source list, per-OS presets |
| `.github/workflows/build.yml` | Upstream's CI: REUSE, clang-format, tests and release builds on all three systems |

## Our modules

### gpucore: the shared graphics core (D1-D5)

The library the games link: `add_subdirectory(<fork>/gpucore)`, then `target_link_libraries(<game> PRIVATE shadps4::gpucore)`. It compiles a subset of upstream's tree (the same subset the games vendor today: `src/video_core`, `src/shader_recompiler`, `src/common`, `src/core/libraries/{gnmdriver,videoout}`, the kernel event queue and error headers, `externals/{sirit,gcn,half}`) and never upstream's emulator app. Upstream's top-level `CMakeLists.txt` is not touched.

| Path | Owns | Status |
|---|---|---|
| `gpucore/CMakeLists.txt` | The `shadps4_gpucore` static library (`shadps4::gpucore`), version from `gpucore/VERSION`, tests when built on its own | skeleton (stubs only; the upstream sources join in the core-sources issues) |
| `gpucore/VERSION` | The core release, `X.Y.Z`; a tag `gpucore-vX.Y.Z` must match it | present (`0.0.0`) |
| `gpucore/include/shadps4_gpucore.h` | The C interface: ABI version, config, init, resolve, fault, shutdown | skeleton |
| `gpucore/include/shadps4_gpucore_host.h` | What the game's loader provides (log, memory protection, guest-to-host): the host seam | skeleton (grows with the host-seam issue) |
| `gpucore/include/shadps4_gpucore_hooks.h` | The per-game hook table | skeleton |
| `gpucore/src/gpucore.cpp` | The C interface's implementation | ABI and config functions present; the rest skeleton |
| `gpucore/src/hooks.cpp` | The hook registry | present |
| `gpucore/src/host/` | General pieces the bbport shims carry today: diagnostics (write log, wait trace, NaN guard), settings store, in-game overlay, movie playback bridge | planned |
| `gpucore/tests/` | GoogleTest, no GPU needed: every `test_*.cpp` joins `shadps4_gpucore_test`; `abi_c_compile.c` keeps the headers plain C | skeleton (harness runs) |
| `gpucore/tests/consumer/` | A tiny project that consumes the core exactly as a game does | planned |
| `docs/gpucore-triage/uncharted.md` | M4 triage of Uncharted-Universal's shadPS4 copy: every changed file marked general, per-game or dropped (#17) | present |

**ABI version and config check** (`shadps4_gpucore_abi_version`, `shadps4_gpucore_version_string`, `shadps4_gpucore_check_config`):

- `shadps4_gpucore_abi_version()` returns `SHADPS4_GPUCORE_ABI_VERSION` (`MAJOR << 16 | MINOR`, today `0x00000001`).
- `shadps4_gpucore_version_string()` returns the text of `gpucore/VERSION` (the compile definition `SHADPS4_GPUCORE_VERSION_STRING`), never NULL.
- `shadps4_gpucore_check_config(config)` checks in this order and returns the first failure:
  1. `config` is NULL: `SHADPS4_GPUCORE_E_INVALID_ARGUMENT`.
  2. `config->struct_size` is less than `sizeof(ShadPs4GpuCoreConfig)`: `E_INVALID_ARGUMENT`.
  3. `config->abi_version >> 16` is not `SHADPS4_GPUCORE_ABI_MAJOR`: `SHADPS4_GPUCORE_E_ABI_MISMATCH` (the minor half is not compared).
  4. `title` is NULL or `""`: `E_INVALID_ARGUMENT`.
  5. `serial` is NULL, or not exactly 9 characters of which the first 4 are `A`-`Z` and the last 5 are `0`-`9`: `E_INVALID_ARGUMENT`.
  6. `user_dir` is NULL or `""`: `E_INVALID_ARGUMENT`.
  7. `width` or `height` is below 1 or above 16384: `E_INVALID_ARGUMENT`.
  8. Otherwise `SHADPS4_GPUCORE_OK`. `sdk_version` and `psf_attributes` are not checked.

**Hook table** (`shadps4_gpucore_set_hooks`, `shadps4_gpucore_hooks`):

1. `set_hooks(NULL)` clears the table and returns `SHADPS4_GPUCORE_OK`; `hooks()` then returns NULL.
2. A table whose `struct_size` is below `SHADPS4_GPUCORE_HOOKS_V1_SIZE` returns `SHADPS4_GPUCORE_E_INVALID_ARGUMENT` and leaves the current table as it was.
3. Otherwise the core copies the first `min(struct_size, sizeof(ShadPs4GpuCoreHooks))` bytes into its own table, zeroes the rest, sets its copy's `struct_size` to `sizeof(ShadPs4GpuCoreHooks)` and returns `SHADPS4_GPUCORE_OK`. `hooks()` returns a pointer to the core's copy, never the caller's pointer.
4. A table with every callback NULL is still a set table: `hooks()` returns non-NULL.
5. Before the core-init issue lands there is no running core; once it does, `set_hooks` after `shadps4_gpucore_init` succeeded returns `SHADPS4_GPUCORE_E_ALREADY_INITIALIZED`.

Hook call sites (planned): `patch_shader` where the pipeline cache compiles a guest shader (`src/video_core/renderer_vulkan/vk_pipeline_cache.cpp`), `before_submit` in the scheduler's submit (`vk_scheduler.cpp`), `after_present` in the presenter (`vk_presenter.cpp`). Each is a one-line `// fork:` call into `gpucore/src/hooks.cpp`, listed in `docs/fork-delta.md`.

**Host seam** (planned): the emulator-kernel calls the subset makes today (memory tracking and protection in `page_manager`, guest address translation, logging, settings, fault handling) go through `ShadPs4GpuCoreHost`. In the games this was each port's `shim/` folder; the general parts of those shims move to `gpucore/src/host/`, the per-game parts stay in the game.

### pkg-extract: the package extractor (D6)

| Path | Owns | Status |
|---|---|---|
| `src/core/file_format/pkg.{h,cpp}`, `pkg_type.{h,cpp}` | The package reader restored from upstream 0.7.0 for fake (self-made) packages: OpenSSL/LibreSSL crypto, 64-bit sizes, bad keys or blocks are errors. The keys are the `PkgKeyFile::Keys` passed in from `--extract-keys` at run time; no key is compiled in | present (Linux) |
| `src/core/file_format/pkg_keyfile.{h,cpp}` | Parser for the key file read at run time | present |
| `src/core/file_format/pkg_extract.{h,cpp}` | The unpacking loop (worker threads, progress) behind `--extract-pkg`; `shadps4-pkg` does not use it yet (its unpacking is not wired) | present |
| `src/main.cpp` | Upstream's CLI plus our `--extract-pkg <pkg>`, `--extract-dir <dir>`, `--extract-keys <file>`, `--extract-threads N` | present (Linux) |
| `tools/pkg-extract/cli.{h,cpp}` | `shadps4-pkg`'s argument parser | present |
| `tools/pkg-extract/main.cpp`, `CMakeLists.txt` | The standalone `shadps4-pkg` binary | skeleton (parses; unpacking not wired) |
| `tools/pkg-extract/tests/` | GoogleTest: every `test_*.cpp` joins `shadps4_pkg_test`; `pkg_builder.{h,cpp}` makes fake test packages (header, entry table, param.sfo) from payloads the test generates | present (builder: header, entries and param.sfo; PFS image and encryption later) |

**Command line** (`PkgExtract::ParseArgs(args)`, `args` = the arguments after the program name):

1. If any argument is exactly `-h` or `--help`, return options with `help = true` and every other field at its default, whatever else is given.
2. Otherwise scan left to right. `--keys`, `--out` and `--threads` take the next argument as their value, whatever it is; with no next argument the error is `missing value for --keys` (or `--out`, `--threads`). A repeated option: the last one wins.
3. `--threads` takes only 1 to 3 ASCII digits whose value is 1 to 32 (`07` is 7); anything else is the error `--threads must be a number from 1 to 32`.
4. Any other argument that starts with `-` and is longer than one character is the error `unknown option: <argument>`.
5. The first other argument is the package; a second one is the error `unexpected argument: <argument>`.
6. After the scan: no package is the error `missing package path`; then no `--keys` is the error `missing --keys <file>`.
7. On an error, `options` is empty and `error` holds exactly the message above. On success `error` is empty, `threads` defaults to 4 and `out_dir` stays empty unless `--out` was given.

**Key file** (`PkgKeyFile::Parse(text)`), the user's own file, never shipped:

1. Split `text` on `\n`; drop one trailing `\r` from each line; trim spaces and tabs from both ends. Skip empty lines and lines starting with `#`. Lines are numbered from 1, counting skipped lines.
2. Every other line is `name = value` with exactly one `=`; name and value are trimmed. A line with no `=` or more than one is the error `line N: expected name = hex`.
3. The names are `dk3.modulus`, `dk3.private_exponent`, `fake.modulus`, `fake.private_exponent`; any other is `line N: unknown key name: <name>`, where `<name>` is shown only if it is 1-32 ASCII letters, digits, `_` or `.`; otherwise it is `<unprintable key name>`. A name given twice is `line N: <name> given twice` (N is the second line).
4. Each value is exactly 512 hex digits (`0-9`, `a-f`, `A-F`), read as 256 bytes, first byte first; anything else is `line N: <name> must be 512 hex digits`.
5. At the end, the first missing name in the order of rule 3 is the error `missing key: <name>`.
6. On an error, `keys` is empty and `error` holds exactly that message; on success `error` is empty.

### fork-delta: the upstream-delta check (M6)

| Path | Owns | Status |
|---|---|---|
| `docs/fork-delta.md` | The list of upstream files we edit, with why | present |
| `tools/fork-delta/fork_delta.py` | `parse_listed(markdown)`, `compare(listed, changed)` and the CI entry point | present |

`parse_listed` returns the set of paths in the first column of the table under the heading `## Upstream files we edit` (the text inside the first pair of backticks of each row; the header and `|---|` rows are skipped; the table ends at the first line that does not start with `|`). `compare(listed, changed)` drops from `changed` every path that starts with one of `OURS_PREFIXES`, then returns `(unlisted, stale)`: the sorted paths in `changed` but not `listed`, and the sorted paths in `listed` but not `changed`.

### CI and releases

| Path | Owns | Status |
|---|---|---|
| `.github/workflows/fork-core.yml` | Our CI on GitHub-hosted Linux, Windows and macOS: the no-game-code grep (M3), core and extractor build and tests | skeleton (builds the skeletons) |
| tags `gpucore-vX.Y.Z` | Core releases the games pin | planned |

## Where our changes live

Our edits to upstream's files are listed, with why, in `docs/fork-delta.md`. Everything else of ours is in our own files: `gpucore/`, `tools/pkg-extract/`, `tools/fork-delta/`, `src/core/file_format/pkg*`, `docs/code-map.md`, `docs/fork-delta.md`, `.github/workflows/fork-core.yml`, `SPEC.md`, `ROADMAP.md`, `PIPELINE.md`, `AGENTS.fork.md`, `CHANGELOG.fork.md`.

## Interlinks

- **The four games** (Uncharted-Universal, Wipeout-Universal, Ratchet-Clank-Universal, TheLastGuardian-Universal; private): each vendors a copy of this subset today (`gpu/shadps4`, The Last Guardian under `port/gpu`), all from upstream `b4e7ae7`, plus its own `shim/` and a C header (`bbgpu.h`, `gow3gpu.h` or `tlggpu.h`). At 1.0 Uncharted-Universal pins this fork as a submodule at a `gpucore-v*` tag and links `shadps4::gpucore`, its per-game changes behind the hooks; the others (and SotC-Universal, also a PS4 port) may do the same as optional work, or keep and customise their own copy (my ruling, 2026-11-02). Switch-over issues live in each game repo (`ROADMAP.md`, cross-repo notes).
- **FSR-Vulkan** (public): the games' shared upscaler. It plugs into the core's present path through the hooks; the core does not vendor it.
- **save-relay**: not needed here (saves belong to the games' runtimes).

## Build and test

```sh
# The core and the extractor (ours), each on its own:
cmake -S gpucore -B build/gpucore -DCMAKE_BUILD_TYPE=Release && cmake --build build/gpucore --config Release
ctest --test-dir build/gpucore -C Release --output-on-failure
cmake -S tools/pkg-extract -B build/pkg-extract -DCMAKE_BUILD_TYPE=Release && cmake --build build/pkg-extract --config Release
ctest --test-dir build/pkg-extract -C Release --output-on-failure
python3 -m pytest tools/fork-delta                     # the fork-delta check's tests, once they exist

# Upstream's emulator, with our --extract-pkg:
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # add -DENABLE_TESTS=ON for upstream's tests
cmake --build build --parallel
./build/shadps4 --extract-pkg game.pkg --extract-keys my-keys.txt --extract-dir out/
```

Per-platform dependencies for upstream's build: `documents/building-linux.md`, `building-macos.md`, `building-windows.md`. The skeletons need only CMake 3.24+, a C++23 compiler and network access for GoogleTest.

## Mac / PC / Linux

- **Linux:** the reference platform: GCC or Clang, Vulkan loader from the distribution. Our `--extract-pkg` was first built here.
- **Windows:** MSVC (Visual Studio 2022+) or clang-cl; the games build with MSYS2 clang today, so the core must build with both. Paths are wide (`std::filesystem`), long paths need care in the extractor.
- **macOS:** Apple silicon, Vulkan through MoltenVK (upstream's macOS guide). The core's GPU code builds natively for arm64; the PS4's x86-64 game code needs a CPU translator (bbcpu-style), which is the games' business, not the core's.
- CI covers all three on every merge (`fork-core.yml`); device checks with my own packages and games are `needs-owner-device`.

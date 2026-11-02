# Upstream files this fork edits

Every upstream file that differs from upstream's main, with why, so the daily upstream merge stays cheap (SPEC.md M6). Our own files (`gpucore/`, `tools/pkg-extract/`, `tools/fork-delta/`, `docs/`, `src/core/file_format/pkg*`, our pipeline files and `.github/workflows/fork-core.yml`) need no line here. Keep each edit small and marked `// fork:` in the code. CI checks this list (`tools/fork-delta/fork_delta.py`, once the fork-delta issues land).

## Upstream files we edit

| Path | Why |
|---|---|
| `src/main.cpp` | The `--extract-pkg`, `--extract-dir` and `--extract-threads` options and the unpacking call |
| `CMakeLists.txt` | Adds our package reader files to the `CORE` source list |
| `.gitignore` | Ignores `src/core/file_format/pkg_keys.h` (key sets never enter the repo) |

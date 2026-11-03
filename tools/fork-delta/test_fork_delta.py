# SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Locked tests: the fork-delta check (docs/code-map.md, fork-delta)."""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import fork_delta  # noqa: E402
from fork_delta import OURS_PREFIXES, compare, parse_listed  # noqa: E402

DOC = """# Fork delta

Some text with `not/a/row.cpp` in it.

## Upstream files we edit

| File | Why |
|---|---|
| `src/main.cpp` | our `--extract-pkg` option |
| `CMakeLists.txt` | builds `pkg_keyfile.cpp` |
|`.gitignore`|keys stay out|
Text right after the table ends it.
| `after/table.cpp` | not in the table any more |

## Another heading

| `other/table.cpp` | a different table |
"""


def test_parse_listed_reads_first_backticks_of_each_row():
    assert parse_listed(DOC) == {"src/main.cpp", "CMakeLists.txt", ".gitignore"}


def test_parse_listed_skips_header_and_rule_rows():
    doc = "## Upstream files we edit\n\n| File | Why |\n|---|---|\n| `a.cpp` | x |\n"
    assert parse_listed(doc) == {"a.cpp"}


def test_parse_listed_table_ends_at_first_non_pipe_line():
    doc = "## Upstream files we edit\n| `a.cpp` | x |\n\n| `b.cpp` | y |\n"
    assert parse_listed(doc) == {"a.cpp"}


def test_parse_listed_ignores_other_headings():
    doc = "## Something else\n| `x.cpp` | y |\n"
    assert parse_listed(doc) == set()


def test_parse_listed_real_doc_has_the_three_files():
    real = (Path(__file__).resolve().parents[2] / "docs" / "fork-delta.md").read_text("utf-8")
    assert {"src/main.cpp", "CMakeLists.txt", ".gitignore"} <= parse_listed(real)


def test_compare_drops_our_own_paths():
    changed = {"gpucore/src/gpucore.cpp", "tools/pkg-extract/cli.cpp", "SPEC.md",
               "src/core/file_format/pkg_keyfile.cpp", ".github/workflows/fork-core.yml"}
    assert compare(set(), changed) == ([], [])


def test_compare_every_ours_prefix_is_dropped():
    changed = {p + ("x" if p.endswith("/") else "") for p in OURS_PREFIXES}
    assert compare(set(), changed) == ([], [])


def test_compare_unlisted_and_stale_are_sorted():
    listed = {"src/main.cpp", "z.txt", "a.txt"}
    changed = {"src/main.cpp", "src/video_core/b.cpp", "src/video_core/a.cpp"}
    assert compare(listed, changed) == (
        ["src/video_core/a.cpp", "src/video_core/b.cpp"],
        ["a.txt", "z.txt"],
    )


def test_compare_clean_when_lists_match():
    listed = {"src/main.cpp", "CMakeLists.txt"}
    changed = {"src/main.cpp", "CMakeLists.txt", "gpucore/VERSION"}
    assert compare(listed, changed) == ([], [])


def test_compare_does_not_change_its_inputs():
    listed = {"a"}
    changed = {"b", "gpucore/c"}
    compare(listed, changed)
    assert listed == {"a"} and changed == {"b", "gpucore/c"}


def test_main_without_argument_prints_usage_and_returns_2(capsys):
    assert fork_delta.main(["fork_delta.py"]) == 2
    assert "Usage" in capsys.readouterr().err

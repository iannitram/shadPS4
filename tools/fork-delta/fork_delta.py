# SPDX-FileCopyrightText: Copyright 2026 shadPS4 fork contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Checks that every upstream file this fork edits is listed in docs/fork-delta.md (SPEC.md M6).

Usage (CI, after fetching upstream's main):
    python3 tools/fork-delta/fork_delta.py <merge-base commit>

Rules for parse_listed and compare: docs/code-map.md, fork-delta.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

# Paths that are wholly ours: never need a line in docs/fork-delta.md.
OURS_PREFIXES = (
    "gpucore/",
    "tools/pkg-extract/",
    "tools/fork-delta/",
    "docs/",
    ".github/workflows/fork-core.yml",
    "src/core/file_format/pkg",
    "AGENTS.fork.md",
    "CHANGELOG.fork.md",
    "PIPELINE.md",
    "SPEC.md",
    "ROADMAP.md",
)


def parse_listed(markdown: str) -> set[str]:
    """The paths listed in the '## Upstream files we edit' table of docs/fork-delta.md."""
    raise NotImplementedError("the fork-delta check issue")


def compare(listed: set[str], changed: set[str]) -> tuple[list[str], list[str]]:
    """(unlisted, stale): changed upstream files missing from the list, listed files not changed."""
    raise NotImplementedError("the fork-delta check issue")


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    root = Path(__file__).resolve().parents[2]
    changed_text = subprocess.run(
        ["git", "diff", "--name-only", argv[1], "HEAD"],
        cwd=root, check=True, capture_output=True, text=True,
    ).stdout
    changed = {line for line in changed_text.splitlines() if line}
    listed = parse_listed((root / "docs" / "fork-delta.md").read_text(encoding="utf-8"))
    unlisted, stale = compare(listed, changed)
    for path in unlisted:
        print(f"unlisted: {path} (add it to docs/fork-delta.md with why)")
    for path in stale:
        print(f"stale: {path} (no longer differs from upstream; remove its line)")
    return 1 if unlisted or stale else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

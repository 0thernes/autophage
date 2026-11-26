#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Alexander Stephen Donahue
"""Verify that README.md reproduces the verbatim Lean 4.35.0 `#print axioms` output
from `results/lean4-audit.txt` with zero drift, and that zero forbidden axioms exist."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
AXIOMS_PATH = ROOT / "results" / "lean4-audit.txt"
README_PATH = ROOT / "README.md"
ALLOWED = {"propext", "Classical.choice", "Quot.sound"}
FORBIDDEN = ("sorryAx", "Lean.ofReduceBool", "Lean.trustCompiler", "_native", "bv_decide", "native_decide")


def main() -> None:
    if not AXIOMS_PATH.exists():
        print(f"FAIL: missing {AXIOMS_PATH}", file=sys.stderr)
        sys.exit(1)
    axioms_txt = AXIOMS_PATH.read_text(encoding="utf-8").replace("\r\n", "\n").strip()
    lines = [ln.strip() for ln in axioms_txt.splitlines() if ln.strip()]
    if len(lines) != 48:
        print(f"FAIL: expected 48 axiom lines in {AXIOMS_PATH}, got {len(lines)}", file=sys.stderr)
        sys.exit(1)
    for tok in FORBIDDEN:
        if tok in axioms_txt:
            print(f"FAIL: forbidden axiom token {tok!r} found in {AXIOMS_PATH}", file=sys.stderr)
            sys.exit(1)
    for ln in lines:
        if re.match(r"^'[^']+' does not depend on any axioms$", ln):
            continue
        m = re.match(r"^'([^']+)' depends on axioms: \[([^\]]*)\]$", ln)
        if not m:
            print(f"FAIL: unexpected axiom line: {ln}", file=sys.stderr)
            sys.exit(1)
        axs = [a.strip() for a in m.group(2).split(",") if a.strip()]
        bad = [a for a in axs if a not in ALLOWED]
        if bad:
            print(f"FAIL: disallowed axioms {bad} on {m.group(1)}", file=sys.stderr)
            sys.exit(1)

    readme = README_PATH.read_text(encoding="utf-8").replace("\r\n", "\n")
    marker_start = "<!-- LEAN_AXIOMS_START -->"
    marker_end = "<!-- LEAN_AXIOMS_END -->"
    if marker_start not in readme or marker_end not in readme:
        print("FAIL: README.md missing LEAN_AXIOMS_START / LEAN_AXIOMS_END markers", file=sys.stderr)
        sys.exit(1)
    block = readme.split(marker_start, 1)[1].split(marker_end, 1)[0]
    m = re.search(r"```text\n(.*?)\n```", block, flags=re.S)
    if not m:
        print("FAIL: README.md axioms block missing ```text fence", file=sys.stderr)
        sys.exit(1)
    readme_axioms = m.group(1).strip()
    expected_block = "\n".join(lines)
    if readme_axioms != expected_block:
        print("FAIL: README.md verbatim Lean axioms block drifted from results/lean4-audit.txt!", file=sys.stderr)
        sys.exit(1)
    print(f"PASS: All {len(lines)} Lean 4.35.0 theorem axioms verified strictly within {sorted(ALLOWED)} and README.md verbatim block matches 100%.")


if __name__ == "__main__":
    main()

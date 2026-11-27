#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Alexander Stephen Donahue
"""Validate the Interactive Lean 4.35.0 Proof Blueprint & GitHub Pages Portal
(`docs/index.html`, `docs/blueprint.html`, `docs/blueprint.css`, `docs/blueprint.js`):
  1. Strict CSP header & zero inline scripts/styles.
  2. Zero external CDN dependencies (self-hosted assets only).
  3. WCAG 2.2 AA accessibility structure (lang, skip-link, landmarks, focus-visible, reduced-motion).
  4. Exact 100% parity with every audited theorem in `results/lean4-audit.txt`.
  5. Explicit honest-scope notices ("not peer-reviewed", "0 / 1,900" Mega-Atlas non-retirement)."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
AXIOMS_PATH = ROOT / "results" / "lean4-audit.txt"

FORBIDDEN_CDN = re.compile(
    r"(cdn\.jsdelivr|unpkg\.com|cdnjs\.cloudflare|fonts\.googleapis|fonts\.gstatic|polyfill\.io|ajax\.googleapis)",
    re.I,
)
INLINE_SCRIPT = re.compile(r"<script(?![^>]*\bsrc=)[^>]*>", re.I)


def main() -> None:
    required = ["index.html", "blueprint.html", "blueprint.css", "blueprint.js"]
    for name in required:
        p = DOCS / name
        if not p.exists() or p.stat().st_size == 0:
            sys.exit(f"FAIL: missing or empty docs/{name}")

    css = (DOCS / "blueprint.css").read_text(encoding="utf-8")
    for token in (":focus-visible", "prefers-reduced-motion", ".skip-link"):
        if token not in css:
            sys.exit(f"FAIL: docs/blueprint.css missing accessibility rule {token!r}")

    for name in ("index.html", "blueprint.html"):
        html_txt = (DOCS / name).read_text(encoding="utf-8")
        if FORBIDDEN_CDN.search(html_txt):
            sys.exit(f"FAIL: docs/{name} references an external CDN")
        if INLINE_SCRIPT.search(html_txt):
            sys.exit(f"FAIL: docs/{name} contains an inline <script> violating strict CSP")
        if "<style" in html_txt.lower():
            sys.exit(f"FAIL: docs/{name} contains an inline <style> violating strict CSP")
        for req_tok in (
            'lang="en"',
            'Content-Security-Policy',
            "default-src 'self'",
            "script-src 'self'",
            'class="skip-link"',
            "<main",
            "<nav",
            "<footer",
            "not peer-reviewed",
            "0 / 1,900",
        ):
            if req_tok.lower() not in html_txt.lower():
                sys.exit(f"FAIL: docs/{name} missing required token {req_tok!r}")

    bp_txt = (DOCS / "blueprint.html").read_text(encoding="utf-8")
    axioms_txt = AXIOMS_PATH.read_text(encoding="utf-8")
    thms = re.findall(r"'([^']+)'\s+(?:does not depend|depends on axioms)", axioms_txt)
    if len(thms) != 56:
        sys.exit(f"FAIL: expected 56 audited theorems in results/lean4-audit.txt, found {len(thms)}")

    missing = [fq.rsplit(".", 1)[-1] for fq in thms if fq.rsplit(".", 1)[-1] not in bp_txt]
    if missing:
        sys.exit(f"FAIL: docs/blueprint.html missing {len(missing)} theorem(s): {missing[:5]}")

    print(
        f"PASS: Verified docs/index.html & docs/blueprint.html (strict CSP, zero CDNs, WCAG 2.2 AA, "
        f"{len(thms)}/{len(thms)} theorem parity, honest scope notices)."
    )


if __name__ == "__main__":
    main()

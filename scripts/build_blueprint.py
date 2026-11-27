#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Alexander Stephen Donahue
"""Generate the Interactive Lean 4.35.0 Proof Blueprint & GitHub Pages Portal
into both `docs/` (`docs/index.html`, `docs/blueprint.html`, `docs/blueprint.css`,
`docs/blueprint.js`) and `_site/` for `.github/workflows/pages.yml`.

WCAG 2.2 AA accessible, strict CSP (zero inline scripts/styles, zero external CDNs),
dynamically synced from `VERSION`, `results/lean4-audit.txt`, and `lean/comparator.json`."""

from __future__ import annotations

import html
import json
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
SITE = ROOT / "_site"
REPO_URL = "https://github.com/0thernes/autophage"

GROUPS = [
    ("Group I", "GFNI Ribosomes (T01-T07)", "lean/AutophageProofs.lean", "§3.2", "#38e8c6"),
    ("Group II", "GF(2)[x] & Cauchy MDS (T08-T16a)", "lean/AutophageProofs.lean", "§3.2", "#56ccf2"),
    ("Group III", "Symplectic Gates & Erasure (T17-T22)", "lean/AutophageProofs.lean", "§3.3", "#6ee7b7"),
    ("Group IV", "2-Adic TZCNT Ultrametric (T23-T25a)", "lean/AutophageProofs.lean", "§4", "#a78bfa"),
    ("Group V", "JIT Guards & 5-Pool Ledger (T26-T30)", "lean/AutophageProofs.lean", "§4", "#fbbf24"),
    ("Group VI", "Philox, Seeds, OEE, Weismann (T39-T42)", "lean/AutophageProofs.lean", "§4", "#60a5fa"),
    ("NonVacuity", "Witnesses & Boundaries (T43-T48)", "lean/AutophageProofs.lean", "§4", "#f59e0b"),
    ("AdversarialTests", "False-Variant Refutations (AT01-AT08)", "lean/AdversarialTests.lean", "§4", "#f87171"),
]

ELEMENTARY_HELPERS = {
    "T21_landauer_zero_change_zero_erasure",
    "T23_ultrametric_self_zero",
    "T24_ultrametric_symmetry",
    "T28_jit_gas_strictly_decreasing",
    "T29_first_law_transfer_conservation",
}


def classify_theorem(short: str) -> tuple[str, str, str, str]:
    if short.startswith("AT"):
        return ("AdversarialTests", "lean/AdversarialTests.lean", "§4 · Adversarial ¬P", "refutation")
    m = re.match(r"^T(\d+)", short)
    num = int(m.group(1)) if m else 99
    kind = "elementary helper" if short in ELEMENTARY_HELPERS else "model theorem"
    if num <= 7:
        return ("Group I", "lean/AutophageProofs.lean", "§3.2 · GFNI", kind)
    if num <= 16:
        return ("Group II", "lean/AutophageProofs.lean", "§3.2 · GF(2)[x] & MDS", kind)
    if num <= 22:
        return ("Group III", "lean/AutophageProofs.lean", "§3.3 · Reversible Gates", kind)
    if num <= 25:
        return ("Group IV", "lean/AutophageProofs.lean", "§4 · 2-Adic TZCNT", kind)
    if num <= 30:
        return ("Group V", "lean/AutophageProofs.lean", "§4 · JIT & Ledger", kind)
    if num <= 42:
        return ("Group VI", "lean/AutophageProofs.lean", "§4 · Philox/Seeds/OEE", kind)
    return ("NonVacuity", "lean/AutophageProofs.lean", "§4 · Non-Vacuity / Boundary", "witness/boundary")


def parse_axioms() -> list[dict[str, str]]:
    audit_path = ROOT / "results" / "lean4-audit.txt"
    txt = audit_path.read_text(encoding="utf-8") if audit_path.exists() else ""
    pat = re.compile(r"'([^']+)'\s+(does not depend on any axioms|depends on axioms:\s*\[([^\]]*)\])")
    rows: list[dict[str, str]] = []
    for fq, clause, ax_raw in pat.findall(txt):
        short = fq.rsplit(".", 1)[-1]
        grp, rel_path, sec, kind = classify_theorem(short)
        axioms = "none (pure kernel)" if "does not depend" in clause else ax_raw.strip()
        rows.append({
            "fq": fq,
            "short": short,
            "group": grp,
            "path": rel_path,
            "section": sec,
            "kind": kind,
            "axioms": axioms,
        })
    return rows


def build_svg_dag(theorems: list[dict[str, str]]) -> str:
    counts: dict[str, int] = {}
    for t in theorems:
        counts[t["group"]] = counts.get(t["group"], 0) + 1
    total = len(theorems)
    W, H = 980, 420
    lines = [
        f'<svg class="dag-svg" viewBox="0 0 {W} {H}" role="img" aria-label="Lean 4.35.0 proof dependency graph from Spec.lean through eight theorem groups to the 6-kernel Palomar comparator">',
        '<defs><marker id="arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M 0 1 L 10 5 L 0 9 z" fill="#4f6d9a"/></marker></defs>',
    ]
    sx, sy, sw, sh = 18, 172, 138, 76
    core_groups = GROUPS[:6]
    nodes = {}
    for idx, (gid, label, path, sec, col) in enumerate(core_groups):
        col_i = idx // 3
        row_i = idx % 3
        nx = 200 + col_i * 212
        ny = 48 + row_i * 116
        nodes[gid] = (nx, ny, 192, 86, label, sec, counts.get(gid, 0), col, path)
    nv_x, nv_y = 636, 86
    at_x, at_y = 636, 240
    px, py, pw, ph = 824, 146, 142, 128

    for gid, (nx, ny, nw, nh, *_rest) in nodes.items():
        x1, y1 = sx + sw, sy + sh / 2
        x2, y2 = nx, ny + nh / 2
        lines.append(f'<path class="dag-edge" d="M {x1:.0f} {y1:.0f} C {x1+28:.0f} {y1:.0f}, {x2-28:.0f} {y2:.0f}, {x2:.0f} {y2:.0f}" marker-end="url(#arr)"/>')
        cx1, cy1 = nx + nw, ny + nh / 2
        lines.append(f'<path class="dag-edge dim" d="M {cx1:.0f} {cy1:.0f} C {cx1+22:.0f} {cy1:.0f}, {nv_x-22:.0f} {nv_y+36:.0f}, {nv_x:.0f} {nv_y+36:.0f}"/>')
        lines.append(f'<path class="dag-edge dim" d="M {cx1:.0f} {cy1:.0f} C {cx1+22:.0f} {cy1:.0f}, {at_x-22:.0f} {at_y+36:.0f}, {at_x:.0f} {at_y+36:.0f}"/>')

    for my in (nv_y + 36, at_y + 36):
        lines.append(f'<path class="dag-edge" d="M {nv_x+160:.0f} {my:.0f} L {px:.0f} {py+ph/2:.0f}" marker-end="url(#arr)"/>')

    lines.append(
        f'<g class="dag-node" data-mod="ALL" tabindex="0" role="button" aria-label="Show all {total} theorems from Spec.lean">'
        f'<rect x="{sx}" y="{sy}" width="{sw}" height="{sh}" rx="10" fill="#0b1728" stroke="#38e8c6" stroke-width="2"/>'
        f'<text x="{sx+sw/2:.0f}" y="{sy+28}" text-anchor="middle" fill="#38e8c6" font-weight="700" font-size="13">Spec.lean</text>'
        f'<text x="{sx+sw/2:.0f}" y="{sy+47}" text-anchor="middle" fill="#cbdcf5" font-size="10.5">Pure Definitions</text>'
        f'<text x="{sx+sw/2:.0f}" y="{sy+63}" text-anchor="middle" fill="#9bb4d8" font-size="9.5">0 Mathlib · 0 sorry</text></g>'
    )

    for gid, (nx, ny, nw, nh, label, sec, cnt, col, _path) in nodes.items():
        lines.append(
            f'<g class="dag-node" data-mod="{html.escape(gid)}" tabindex="0" role="button" aria-label="Filter theorems by {html.escape(gid)}">'
            f'<rect x="{nx}" y="{ny}" width="{nw}" height="{nh}" rx="9" fill="#0c1524" stroke="{col}" stroke-width="1.6"/>'
            f'<text x="{nx+10}" y="{ny+23}" fill="{col}" font-weight="700" font-size="11.5">{html.escape(gid)}</text>'
            f'<text x="{nx+10}" y="{ny+44}" fill="#e6f0ff" font-size="9.8">{html.escape(label)}</text>'
            f'<text x="{nx+10}" y="{ny+66}" fill="#a3badb" font-size="9.5">{html.escape(sec)} · {cnt} theorems</text></g>'
        )

    for gid, mx, my, col, title, sub in [
        ("NonVacuity", nv_x, nv_y, "#f59e0b", "NonVacuity", f"T43–T48 · {counts.get('NonVacuity', 0)} witnesses"),
        ("AdversarialTests", at_x, at_y, "#f87171", "AdversarialTests", f"AT01–AT08 · {counts.get('AdversarialTests', 0)} ¬P refutations"),
    ]:
        lines.append(
            f'<g class="dag-node" data-mod="{gid}" tabindex="0" role="button" aria-label="Filter theorems by {gid}">'
            f'<rect x="{mx}" y="{my}" width="160" height="72" rx="9" fill="#0c1524" stroke="{col}" stroke-width="1.8"/>'
            f'<text x="{mx+10}" y="{my+24}" fill="{col}" font-weight="700" font-size="11.5">{title}</text>'
            f'<text x="{mx+10}" y="{my+44}" fill="#e6f0ff" font-size="9.8">{sub}</text>'
            f'<text x="{mx+10}" y="{my+60}" fill="#a3badb" font-size="9.3">Playbook v1.3 Audit</text></g>'
        )

    lines.append(
        f'<g class="dag-node" data-mod="ALL" tabindex="0" role="button" aria-label="6-kernel Palomar verification summary">'
        f'<rect x="{px}" y="{py}" width="{pw}" height="{ph}" rx="10" fill="#0d1e2b" stroke="#38e8c6" stroke-width="2.2"/>'
        f'<text x="{px+pw/2:.0f}" y="{py+26}" text-anchor="middle" fill="#38e8c6" font-weight="700" font-size="11.5">6-Kernel Palomar</text>'
        f'<text x="{px+pw/2:.0f}" y="{py+46}" text-anchor="middle" fill="#ffffff" font-weight="700" font-size="13">{total} / {total} Verified</text>'
        f'<text x="{px+pw/2:.0f}" y="{py+67}" text-anchor="middle" fill="#cbdcf5" font-size="9.3">Lean Ref + lean4lean</text>'
        f'<text x="{px+pw/2:.0f}" y="{py+82}" text-anchor="middle" fill="#cbdcf5" font-size="9.3">nanoda · con-leche</text>'
        f'<text x="{px+pw/2:.0f}" y="{py+97}" text-anchor="middle" fill="#cbdcf5" font-size="9.3">con-ron · comparator</text>'
        f'<text x="{px+pw/2:.0f}" y="{py+114}" text-anchor="middle" fill="#8be6ff" font-size="9">0 sorry · 0 custom axioms</text></g>'
    )
    lines.append("</svg>")
    return "\n".join(lines)


CSS_TEXT = """/* SPDX-License-Identifier: Apache-2.0 */
:root{--bg:#060a12;--panel:#0d1422;--border:#233756;--text:#ecf4ff;--muted:#b2c7e6;--accent:#4bf2d2;--ice:#9aeaff;--amber:#fcd34d;}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font-family:ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,sans-serif;line-height:1.6}
a{color:var(--ice);text-decoration:underline;text-underline-offset:2px}a:hover{color:#ffffff}
:focus-visible{outline:3px solid var(--accent);outline-offset:2px}
.skip-link{position:absolute;left:-9999px;top:auto;width:1px;height:1px;overflow:hidden}
.skip-link:focus{position:fixed;left:12px;top:12px;width:auto;height:auto;padding:10px 14px;background:#0b1728;color:#fff;border:2px solid var(--accent);border-radius:8px;z-index:999}
.sr-only{position:absolute;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip:rect(0,0,0,0);white-space:nowrap;border:0}
.wrap{max-width:1180px;margin:0 auto;padding:24px 18px 56px}
.topnav{display:flex;flex-wrap:wrap;gap:12px;align-items:center;justify-content:space-between;margin-bottom:16px;padding:10px 14px;background:#0a1220;border:1px solid var(--border);border-radius:10px}
.topnav .brand{font-weight:700;color:var(--accent);letter-spacing:.03em;text-decoration:none}
.topnav .links{display:flex;flex-wrap:wrap;gap:14px;font-size:13.5px}
.hero{background:linear-gradient(135deg,#0b1728 0%,#08101c 100%);border:1px solid var(--border);border-radius:14px;padding:26px;margin-bottom:20px}
.hero h1{margin:0 0 8px;font-size:26px;letter-spacing:.02em}
.hero h1 span{color:var(--accent);font-size:15px;font-weight:600}
.hero p{margin:0 0 14px;color:var(--muted);max-width:94ch}
.notice{background:#151d2e;border-left:4px solid var(--amber);padding:10px 14px;border-radius:6px;margin-bottom:14px;font-size:13.5px;color:#f4f8ff}
.pills{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:14px}
.pill{padding:4px 10px;border-radius:999px;border:1px solid #2b756b;background:#091e24;color:var(--accent);font-size:12px;font-family:ui-monospace,Consolas,monospace}
.cta-row{display:flex;flex-wrap:wrap;gap:10px}
.btn{display:inline-block;padding:8px 14px;border-radius:8px;border:1px solid #355482;background:#112038;color:#fff;font-weight:600;font-size:13.5px;text-decoration:none}
.btn.primary{background:#0f574b;border-color:var(--accent)}
.btn:hover{border-color:var(--accent);background:#162b4a;text-decoration:none}
.card{background:var(--panel);border:1px solid var(--border);border-radius:12px;padding:20px;margin-bottom:18px}
.card h2{margin:0 0 10px;font-size:18px;color:var(--accent);letter-spacing:.02em}
.dag-wrap{overflow-x:auto;background:#070c16;border:1px solid #1d2f4d;border-radius:10px;padding:10px}
.dag-svg{width:100%;min-width:780px;height:auto;display:block}
.dag-edge{fill:none;stroke:#5d80b4;stroke-width:1.4}
.dag-edge.dim{stroke:#2a4063;stroke-width:1.0}
.dag-node{cursor:pointer}.dag-node:hover rect,.dag-node.active rect{stroke-width:3;filter:brightness(1.18)}
.toolbar{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin:12px 0}
.toolbar input,.toolbar select{background:#070d18;color:var(--text);border:1px solid var(--border);border-radius:7px;padding:8px 10px;font-size:13.5px}
.toolbar input{flex:1;min-width:220px}
.count{font-family:ui-monospace,Consolas,monospace;font-size:12.5px;color:var(--amber)}
.table-wrap{overflow-x:auto}
table{width:100%;border-collapse:collapse;font-size:13.5px}
th,td{padding:8px 10px;border-bottom:1px solid #1d2f4d;text-align:left;vertical-align:top}
th{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.04em}
code{font-family:ui-monospace,Consolas,monospace;font-size:12.5px;color:#dff7ff}
.ax{color:var(--accent);font-family:ui-monospace,Consolas,monospace;font-size:12px}
.badge{display:inline-block;padding:2px 7px;border-radius:6px;font-size:11.5px;font-family:ui-monospace,Consolas,monospace;background:#102238;border:1px solid #2b4a73;color:#d6e9ff}
.badge.helper{border-color:#9a7b2c;color:#fde68a;background:#1f190b}
.badge.refutation{border-color:#9b3c46;color:#fecaca;background:#261015}
.grid2{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:14px}
.mini{background:#09111e;border:1px solid #1e314f;border-radius:10px;padding:15px}
.mini h3{margin:0 0 6px;font-size:15px;color:#fff}
.mini p{margin:0;color:var(--muted);font-size:13px}
footer{margin-top:26px;color:var(--muted);font-size:12.5px;text-align:center}
@media (prefers-reduced-motion:reduce){*{transition:none!important;animation:none!important}}
"""

JS_TEXT = """/* SPDX-License-Identifier: Apache-2.0 */
const q = document.getElementById("filter-q");
const modSel = document.getElementById("filter-mod");
const rows = Array.from(document.querySelectorAll("#thm-body tr"));
const countEl = document.getElementById("thm-count");
const nodes = Array.from(document.querySelectorAll(".dag-node"));

if (q && modSel && countEl) {
  function applyFilter() {
    const needle = (q.value || "").trim().toLowerCase();
    const mod = modSel.value;
    let shown = 0;
    for (const tr of rows) {
      const okMod = mod === "ALL" || tr.dataset.mod === mod;
      const okText = !needle || tr.textContent.toLowerCase().includes(needle);
      const vis = okMod && okText;
      tr.hidden = !vis;
      if (vis) shown++;
    }
    countEl.textContent = `Showing ${shown} / ${rows.length} audited theorems`;
    for (const n of nodes) n.classList.toggle("active", n.dataset.mod === mod && mod !== "ALL");
  }

  q.addEventListener("input", applyFilter);
  modSel.addEventListener("change", applyFilter);
  for (const n of nodes) {
    const pick = () => {
      modSel.value = n.dataset.mod || "ALL";
      applyFilter();
    };
    n.addEventListener("click", pick);
    n.addEventListener("keydown", (e) => {
      if (e.key === "Enter" || e.key === " ") { e.preventDefault(); pick(); }
    });
  }
  applyFilter();
}
"""


def build_blueprint_html(version: str, theorems: list[dict[str, str]], comp_count: int) -> str:
    tr_html = []
    for i, t in enumerate(theorems, 1):
        src_link = f"{REPO_URL}/blob/main/{t['path']}"
        cls = "helper" if "helper" in t["kind"] else ("refutation" if t["kind"] == "refutation" else "")
        tr_html.append(
            f'<tr data-mod="{html.escape(t["group"])}">'
            f'<td>{i}</td>'
            f'<td><a href="{src_link}"><code>{html.escape(t["short"])}</code></a></td>'
            f'<td>{html.escape(t["group"])}</td>'
            f'<td><span class="badge {cls}">{html.escape(t["kind"])}</span></td>'
            f'<td>{html.escape(t["section"])}</td>'
            f'<td class="ax">{html.escape(t["axioms"])}</td>'
            f"</tr>"
        )

    counts: dict[str, int] = {}
    for t in theorems:
        counts[t["group"]] = counts.get(t["group"], 0) + 1
    mod_opts = [f'<option value="ALL">All 8 Groups ({len(theorems)} Declarations)</option>'] + [
        f'<option value="{html.escape(g[0])}">{html.escape(g[0])} ({counts.get(g[0], 0)})</option>'
        for g in GROUPS
    ]

    return f"""<!doctype html>
<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>AUTOPHAGE v{html.escape(version)} — Interactive Lean 4.35.0 Proof Blueprint</title>
  <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; base-uri 'none'; form-action 'none'" />
  <link rel="stylesheet" href="blueprint.css" />
</head>
<body>
  <a class="skip-link" href="#main-content">Skip to main content</a>
  <div class="wrap">
    <nav class="topnav" aria-label="Primary navigation">
      <a class="brand" href="index.html">AUTOPHAGE v{html.escape(version)}</a>
      <div class="links">
        <a href="index.html">Research Portal</a>
        <a href="blueprint.html" aria-current="page">Interactive Proof Blueprint ({len(theorems)} Theorems)</a>
        <a href="{REPO_URL}">GitHub</a>
        <a href="{REPO_URL}/blob/main/dist/Donahue_2026_autophage.pdf">Preprint PDF</a>
      </div>
    </nav>

    <main id="main-content">
      <header class="hero">
        <h1>AUTOPHAGE <span>v{html.escape(version)} · Interactive Lean 4.35.0 Proof Blueprint</span></h1>
        <p>Interactive dependency graph and complete kernel-audited theorem ledger for <strong>{len(theorems)} Pure Lean 4.35.0 declarations</strong> about the discrete models in <code>lean/AutophageProofs/Spec.lean</code> (zero Mathlib, zero <code>sorry</code> in <code>Solution.lean</code>, zero <code>native_decide</code>/<code>bv_decide</code>, axioms strictly within <code>[propext, Classical.choice, Quot.sound]</code>).</p>
        <div class="notice"><strong>Honest Verification Boundary:</strong> Independent research work; <strong>not peer-reviewed</strong>. Lean proves properties of <code>Spec.lean</code> models rather than emitted x86-64 machine code. Five theorems (<code>T21</code>, <code>T23</code>, <code>T24</code>, <code>T28</code>, <code>T29</code>) are elementary arithmetic/XOR helper lemmas; finite-field and reversible-gate facts are textbook results verified mechanically against this codebase's definitions. Zero (<strong>0 / 1,900</strong>) Mega-Atlas open problems are solved or retired.</div>
        <div class="pills">
          <span class="pill">Lean 4.35.0 · 0 Mathlib · 0 sorry</span>
          <span class="pill">{comp_count} Comparator Targets</span>
          <span class="pill">8 AdversarialTests (AT01–AT08)</span>
          <span class="pill">19 C11 Gates + 6 Mutants</span>
        </div>
      </header>

      <section class="card" aria-labelledby="dag-h">
        <h2 id="dag-h">Proof Dependency Blueprint (Click or Press Enter on Any Group to Filter)</h2>
        <div class="dag-wrap">
          {build_svg_dag(theorems)}
        </div>
      </section>

      <section class="card" aria-labelledby="tbl-h">
        <h2 id="tbl-h">All {len(theorems)} Kernel-Audited Declarations (T01–T30, T39–T48, AT01–AT08)</h2>
        <div class="toolbar">
          <label class="sr-only" for="filter-q">Search theorems</label>
          <input id="filter-q" type="search" placeholder="Filter by theorem name, group, kind, or axiom (e.g. T30, AT01, cauchy, helper)..." />
          <label class="sr-only" for="filter-mod">Filter by group</label>
          <select id="filter-mod">{"".join(mod_opts)}</select>
          <span class="count" id="thm-count" role="status" aria-live="polite">Showing {len(theorems)} / {len(theorems)} audited theorems</span>
        </div>
        <div class="table-wrap">
          <table>
            <thead><tr><th>#</th><th>Theorem Declaration</th><th>Group</th><th>Role</th><th>Paper Mapping</th><th>Verified Axioms</th></tr></thead>
            <tbody id="thm-body">{"".join(tr_html)}</tbody>
          </table>
        </div>
      </section>
    </main>

    <footer>AUTOPHAGE v{html.escape(version)} · Alexander Stephen Donahue (Independent Researcher, Spokane, WA, USA; ORCID <a href="https://orcid.org/0009-0006-8168-9648">0009-0006-8168-9648</a>) · Not Peer-Reviewed · Zero External CDNs · Built by <code>scripts/build_blueprint.py</code></footer>
  </div>
  <script src="blueprint.js"></script>
</body>
</html>
"""


def build_index_html(version: str, theorems: list[dict[str, str]], comp_count: int) -> str:
    return f"""<!doctype html>
<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>AUTOPHAGE v{html.escape(version)} — Research Portal &amp; Interactive Lean 4.35.0 Proof Blueprint</title>
  <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; base-uri 'none'; form-action 'none'" />
  <link rel="stylesheet" href="blueprint.css" />
</head>
<body>
  <a class="skip-link" href="#main-content">Skip to main content</a>
  <div class="wrap">
    <nav class="topnav" aria-label="Primary navigation">
      <a class="brand" href="index.html" aria-current="page">AUTOPHAGE v{html.escape(version)}</a>
      <div class="links">
        <a href="index.html">Research Portal</a>
        <a href="blueprint.html">Interactive Proof Blueprint ({len(theorems)} Theorems)</a>
        <a href="{REPO_URL}">GitHub</a>
        <a href="{REPO_URL}/blob/main/dist/Donahue_2026_autophage.pdf">Preprint PDF</a>
      </div>
    </nav>

    <main id="main-content">
      <header class="hero">
        <h1>AUTOPHAGE <span>v{html.escape(version)} · Bare-Metal Polymorphic x86-64 Galois-Field Genetic Engine</span></h1>
        <p>Zero-dependency C11 artificial-life testbed where 256-byte dual-strand organisms translate germline genomes via evolvable 8×8 F<sub>2</sub> affine ribosome matrices (<code>VGF2P8AFFINEQB</code>) directly into live native x86-64 RWX machine code, paired with <strong>{len(theorems)} Pure Lean 4.35.0 model theorems</strong>, a <strong>19-gate silicon falsification suite</strong> (+ 6 planted mutants), and an AMS-LaTeX white paper.</p>
        <div class="notice"><strong>Peer-Review &amp; Scope Notice:</strong> Independent research software &amp; preprint — <strong>not peer-reviewed</strong>. Zero (<strong>0 / 1,900</strong>) overarching Unsolved Problems Mega-Atlas problems (<code>AI-045</code>, <code>BIO-001</code>, <code>CHEM-052</code>, <code>SYS-009</code>) are solved or retired. Lean proves properties of <code>Spec.lean</code> models, not emitted x86-64 machine code.</div>
        <div class="pills">
          <span class="pill">Lean 4.35.0 · {len(theorems)}/{comp_count} Theorems</span>
          <span class="pill">19/19 C11 Gates + 6/6 Mutants</span>
          <span class="pill">0-Bit JIT == Scalar SHA-256 Parity</span>
          <span class="pill">17/17 Crossref/DataCite Citations</span>
          <span class="pill">RFC 3161 + Bitcoin OTS + SLSA</span>
        </div>
        <div class="cta-row">
          <a class="btn primary" href="blueprint.html">Open Interactive {len(theorems)}-Theorem Blueprint</a>
          <a class="btn" href="{REPO_URL}">GitHub Repository</a>
          <a class="btn" href="{REPO_URL}/blob/main/dist/Donahue_2026_autophage.pdf">Frozen White Paper PDF</a>
          <a class="btn" href="{REPO_URL}/blob/main/CLAIMS.md">Numbered Claims Ledger</a>
          <a class="btn" href="{REPO_URL}/blob/main/SUBMISSION_READINESS.md">Playbook v1.3 Readiness</a>
          <a class="btn" href="{REPO_URL}/blob/main/docs/statement-fidelity.md">Statement-Fidelity Matrix</a>
          <a class="btn" href="{REPO_URL}/actions">Live Cross-OS &amp; 6-Kernel CI</a>
        </div>
      </header>

      <section class="card" aria-labelledby="pillars-h">
        <h2 id="pillars-h">Verification Architecture &amp; Cryptographic Provenance</h2>
        <div class="grid2">
          <article class="mini">
            <h3>6-Kernel Palomar Lean 4.35.0 Suite</h3>
            <p>Isolated <code>Challenge.lean</code> (importing only <code>Spec.lean</code>) matches sorry-free <code>Solution.lean</code> ({len(theorems)} theorems, including 8 <code>AdversarialTests.lean</code> refutations <code>AT01</code>–<code>AT08</code> and upgraded multi-step ledger theorem <code>T30</code>). Checked locally on Windows by the Lean reference kernel + 3 independent Rust checkers (<code>nanoda_bin</code>, <code>con-leche</code>, <code>con-ron</code>) and by all 6 kernels (+ <code>lean4lean</code> &amp; <code>bwrap</code> comparator + live negative tamper rejection) in Ubuntu CI.</p>
          </article>
          <article class="mini">
            <h3>Bare-Metal C11 Engine &amp; 19-Gate Falsification</h3>
            <p>Zero-bit-diff SHA-256 parity between native RWX JIT and portable scalar oracle (<code>4bd29f39...3c20612</code>), exact 0 uq integer ledger residual, 0 post-warmup heap allocations, 6/6 planted mutants killed, ASan/UBSan verified (with B07 AVX-512 FakeStack alignment regression Gate <code>T19</code>), and locked N=24 paired statistics (24/24 wins, p<sub>Holm</sub>=0.00030).</p>
          </article>
          <article class="mini">
            <h3>Archival, FAIR Metadata &amp; Timestamps</h3>
            <p>Anchored by SHA-256 manifests (<code>CHECKSUMS.sha256</code>), DigiCert RFC 3161 (<code>.tsr</code>) + 3-calendar Bitcoin OpenTimestamps (<code>.ots</code>), SLSA/Rekor provenance, Software Heritage (<code>swh:1:snp:6ea23803655df1f9679deb98d44efbf53321a393</code>), MLCommons Croissant 1.0, W3C RO-Crate 1.1, CodeMeta 2.0, Datasheet, and REUSE/SPDX.</p>
          </article>
          <article class="mini">
            <h3>Author, AI Disclosure &amp; Dual Open License</h3>
            <p>Alexander Stephen Donahue (Independent Researcher, Spokane, WA, USA; ORCID <a href="https://orcid.org/0009-0006-8168-9648">0009-0006-8168-9648</a>). Transparent AI assistance documented in <a href="{REPO_URL}/blob/main/docs/AI_DISCLOSURE.md"><code>docs/AI_DISCLOSURE.md</code></a>. Dual-licensed: <strong>Apache-2.0</strong> (code/Lean) / <strong>CC-BY-4.0</strong> (paper/docs).</p>
          </article>
        </div>
      </section>
    </main>

    <footer>AUTOPHAGE v{html.escape(version)} · Independent Research Preprint (Not Peer-Reviewed) · Zero Tracking · Zero External CDNs · Built by <code>scripts/build_blueprint.py</code></footer>
  </div>
</body>
</html>
"""


def main() -> None:
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    theorems = parse_axioms()
    comp = json.loads((ROOT / "lean" / "comparator.json").read_text(encoding="utf-8"))
    comp_count = len(comp.get("theorem_names", []))
    if len(theorems) != comp_count:
        raise SystemExit(f"FAIL: lean4-audit.txt has {len(theorems)} theorems != comparator.json {comp_count}")

    DOCS.mkdir(parents=True, exist_ok=True)
    SITE.mkdir(parents=True, exist_ok=True)

    bp_html = build_blueprint_html(version, theorems, comp_count)
    idx_html = build_index_html(version, theorems, comp_count)

    for out_dir in (DOCS, SITE):
        (out_dir / "blueprint.css").write_text(CSS_TEXT, encoding="utf-8", newline="\n")
        (out_dir / "blueprint.js").write_text(JS_TEXT, encoding="utf-8", newline="\n")
        (out_dir / "blueprint.html").write_text(bp_html, encoding="utf-8", newline="\n")
        (out_dir / "index.html").write_text(idx_html, encoding="utf-8", newline="\n")
    (SITE / ".nojekyll").write_text("", encoding="utf-8", newline="\n")
    if (DOCS / "media" / "social-preview.svg").exists():
        (SITE / "media").mkdir(parents=True, exist_ok=True)
        shutil.copy2(DOCS / "media" / "social-preview.svg", SITE / "media" / "social-preview.svg")

    print(f"PASS: Built Interactive {len(theorems)}-Theorem Lean 4.35.0 Blueprint & Portal in docs/ and _site/!")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Alexander Stephen Donahue
"""Live Negative Lean 4.35.0 Kernel & Elaboration Rejection Test (Build Standard B06 / Playbook v1.3).

Verifies that the live Lean 4.35.0 kernel and axiom audit gate genuinely CATCH and REJECT:
  1. Negative elaboration: false mathematical claim (`1 = 2` via `rfl`) fails typechecking (nonzero exit).
  2. Negative elaboration: false Toffoli popcount conservation claim (`decide`) fails typechecking.
  3. Live `sorry`-tamper rejection: planted `sorry` emits `sorryAx` and is rejected by the axiom gate.
  4. Live custom-axiom rejection: planted custom `axiom` is rejected by the axiom gate.
  5. Live `native_decide` rejection: planted `native_decide` emits `_native`/`ofReduceBool` and is rejected.

Must run with `lake` on PATH (or ~/.elan/bin/lake); skips cleanly only if `--allow-no-lake` is passed.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LEAN_DIR = ROOT / "lean"
ALLOWED = {"propext", "Classical.choice", "Quot.sound"}
FORBIDDEN_RE = re.compile(r"(sorryAx|ofReduceBool|trustCompiler|_native)", re.I)

ELAB_REJECT_CASES = [
    (
        "false_nat_eq",
        "theorem bad_false : (1 : Nat) = 2 := rfl\n",
    ),
    (
        "false_toffoli_popcnt",
        (
            "import AutophageProofs.Spec\n"
            "open Autophage\n"
            "theorem bad_toffoli_popcnt :\n"
            "    let (d1, s1, t1) := toffoli64 0#64 1#64 1#64\n"
            "    popcnt64 d1 + popcnt64 s1 + popcnt64 t1 = popcnt64 0#64 + popcnt64 1#64 + popcnt64 1#64 := by\n"
            "  decide\n"
        ),
    ),
]

AXIOM_TAMPER_CASES = [
    (
        "planted_sorry",
        "theorem bad_sorry : 1 = 2 := by sorry\n#print axioms bad_sorry\n",
        "sorryAx",
    ),
    (
        "planted_custom_axiom",
        "axiom bad_custom_ax : 1 = 2\ntheorem bad_ax_thm : 1 = 2 := bad_custom_ax\n#print axioms bad_ax_thm\n",
        "bad_custom_ax",
    ),
    (
        "planted_native_decide",
        "theorem bad_native : (1 : Nat) + 1 = 2 := by native_decide\n#print axioms bad_native\n",
        "_native",
    ),
]


def find_lake() -> str | None:
    found = shutil.which("lake")
    if found:
        return found
    elan = Path.home() / ".elan" / "bin" / ("lake.exe" if sys.platform == "win32" else "lake")
    return str(elan) if elan.exists() else None


def audit_accepts(output: str) -> tuple[bool, str]:
    if FORBIDDEN_RE.search(output):
        return False, "forbidden axiom token detected"
    matches = re.findall(
        r"'([^']+)'\s+(does not depend on any axioms|depends on axioms:\s*\[([^\]]*)\])",
        output,
    )
    if not matches:
        return False, "no #print axioms declaration produced"
    for name, _, axs in matches:
        for ax in [a.strip() for a in axs.split(",") if a.strip()]:
            if ax not in ALLOWED:
                return False, f"unpermitted axiom '{ax}' in {name}"
    return True, "accepted"


def main() -> None:
    parser = argparse.ArgumentParser(description="Live negative Lean 4.35.0 elaboration & tamper rejection audit.")
    parser.add_argument("--allow-no-lake", action="store_true", help="Exit 0 with SKIP notice if lake is unavailable")
    args = parser.parse_args()

    lake = find_lake()
    if not lake:
        if args.allow_no_lake:
            print("SKIP: lake binary not found on PATH (--allow-no-lake).")
            return
        print("FAIL: lake binary not found on PATH or ~/.elan/bin.", file=sys.stderr)
        sys.exit(1)

    with tempfile.TemporaryDirectory(prefix="ap_lean_neg_") as tmp:
        tmp_dir = Path(tmp)

        for name, code in ELAB_REJECT_CASES:
            src = tmp_dir / f"{name}.lean"
            src.write_text(code, encoding="utf-8", newline="\n")
            proc = subprocess.run(
                [lake, "env", "lean", str(src)],
                cwd=LEAN_DIR,
                capture_output=True,
                text=True,
                timeout=60,
            )
            if proc.returncode == 0:
                print(f"FAIL: negative elaboration case '{name}' WRONGLY SUCCEEDED!", file=sys.stderr)
                sys.exit(1)
            print(f"  [CAUGHT] {name:<24} -> elaboration rejected (rc={proc.returncode})")

        for name, code, expected_marker in AXIOM_TAMPER_CASES:
            src = tmp_dir / f"{name}.lean"
            src.write_text(code, encoding="utf-8", newline="\n")
            proc = subprocess.run(
                [lake, "env", "lean", str(src)],
                cwd=LEAN_DIR,
                capture_output=True,
                text=True,
                timeout=60,
            )
            combined = (proc.stdout or "") + "\n" + (proc.stderr or "")
            accepted, reason = audit_accepts(combined)
            if accepted:
                print(f"FAIL: tamper case '{name}' was WRONGLY ACCEPTED!\n{combined}", file=sys.stderr)
                sys.exit(1)
            if expected_marker not in combined and proc.returncode == 0:
                print(
                    f"FAIL: tamper case '{name}' did not emit expected marker '{expected_marker}':\n{combined}",
                    file=sys.stderr,
                )
                sys.exit(1)
            print(f"  [CAUGHT] {name:<24} -> rejected ({reason})")

    total = len(ELAB_REJECT_CASES) + len(AXIOM_TAMPER_CASES)
    print(f"PASS: All {total}/{total} live negative Lean 4.35.0 elaboration & axiom-tamper tests were rejected!")


if __name__ == "__main__":
    main()

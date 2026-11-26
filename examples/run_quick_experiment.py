#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Alexander Stephen Donahue
"""Minimal runnable example exercising the compiled AUTOPHAGE v1.4.0 binary:
1. Hardware silicon capability probe (`probe`)
2. Dual-kernel bit-exact JIT vs. Scalar SHA-256 benchmark (`bench --epochs 40 --size 16`)
3. Short deterministic zero-seed abiogenesis trace (`abiogenesis --epochs 60 --seed 5000 --size 16`)
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "bin" / ("autophage.exe" if sys.platform == "win32" else "autophage_linux")


def run_step(args: list[str]) -> str:
    res = subprocess.run([str(BIN), *args], capture_output=True, text=True, check=False)
    if res.returncode != 0:
        print(res.stdout)
        print(res.stderr, file=sys.stderr)
        sys.exit(res.returncode)
    return res.stdout


def main() -> None:
    if not BIN.exists():
        print(f"Binary not found at {BIN}. Build first via build.ps1 or build_linux.sh.", file=sys.stderr)
        sys.exit(1)

    print("=== 1. Silicon Probe ===")
    print(run_step(["probe"]).strip())

    print("\n=== 2. Dual-Kernel Bit-Exact Parity Check (16x16, 40 epochs) ===")
    bench_out = run_step(["bench", "--epochs", "40", "--size", "16"])
    print(bench_out.strip())
    assert "BIT-EXACT PARITY  : PASS (0-BIT DIFF!)" in bench_out

    print("\n=== 3. Deterministic Zero-Seed Abiogenesis Sample (16x16, 60 epochs, seed 5000) ===")
    abio_out = run_step(["abiogenesis", "--epochs", "60", "--seed", "5000", "--size", "16"])
    for ln in abio_out.splitlines()[:10]:
        print(ln)
    print("\nPASS: Example completed cleanly.")


if __name__ == "__main__":
    main()

# AUTOPHAGE v1.5.0 — Independent Clean-Clone Replication Protocol

Anyone with an x86-64 Windows or Linux machine can reproduce every theorem, falsification gate, mutant kill, negative Lean test, dual-kernel SHA-256 hash, empirical benchmark, interactive blueprint, cryptographic timestamp check, and white-paper PDF from a clean clone.

## 1. Prerequisites

- **C11 Toolchain:** Clang 18+ (`clang`, `lld`) or GCC 12+ (`gcc`), plus `node` (v20+, optional for Web Observatory) and `python3` (3.10+, zero external pip packages required).
- **Lean 4 Toolchain:** `elan` with pinned `leanprover/lean4:v4.35.0` (automatically read from `lean/lean-toolchain`). On Ubuntu Linux, install `bubblewrap` (`sudo apt-get install -y bubblewrap`) for `lake check --paranoid` and `lake comparator --paranoid`.
- **White Paper (Optional):** `tectonic` (`https://tectonic-typesetting.github.io`) for zero-install TeX compilation.
- **Timestamps (Optional):** `openssl` for RFC 3161 `.tsr` verification (`scripts/stamp_timestamps.py --verify`).

## 2. Step-by-Step Replication Commands

### A. Linux / WSL2 (Full 6-Kernel Lean + C11 + ASan/UBSan)
```bash
git clone https://github.com/0thernes/autophage.git
cd autophage

# 1. Native C11 build (-Wall -Wextra -Werror) + 19/19 gates + 6/6 mutants + 56 Lean proofs
#    (Add AUTOPHAGE_CI_PORTABLE=1 if CPU lacks hardware GFNI/VPCLMULQDQ)
bash build_linux.sh

# 2. AddressSanitizer + UndefinedBehaviorSanitizer run (incl. B07 AVX-512 alignment Gate T19)
AUTOPHAGE_SANITIZE=1 AUTOPHAGE_CI_PORTABLE=1 bash build_linux.sh

# 3. Verify 0-bit diff JIT vs Scalar canonical SHA-256 benchmark
./bin/autophage_linux bench --epochs 200 --size 32
# Expected SHA-256 (both JIT and SCALAR):
# 4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612

# 4. Run full 6-kernel Palomar verification + Challenge/Solution comparator + live negative tests
cd lean
lake build
lake build Challenge
lake env lean VerifyAxioms.lean > ../results/lean4-audit.txt
lake env leanchecker --fresh Solution
lake env leanchecker-paranoid --fresh Solution
lake check --paranoid
lake comparator --config comparator.json --paranoid
cd ..
python3 scripts/test_lean_negative.py

# 5. Independent Python property checks, axiom/blueprint/timestamp/citation & paper verification
python3 scripts/prop_check.py
python3 scripts/check_readme_axioms.py
python3 scripts/check_blueprint.py
python3 scripts/stamp_timestamps.py --verify
python3 scripts/check_citations.py
python3 scripts/build_paper.py
bash scripts/audit-repo.sh .
```

### B. Windows 11 x64 (PowerShell 7)
```powershell
git clone https://github.com/0thernes/autophage.git
Set-Location autophage

# 1. Build Win64 PE + DLL + Static Lib + 19/19 Gates + 6/6 Mutants + 56 Lean Proofs
pwsh -NoProfile -ExecutionPolicy Bypass -File build.ps1

# 2. Replay the Lean reference kernel (lean4, leanchecker, leanchecker-paranoid) + 3 independent Rust checkers (nanoda_bin, con-leche, con-ron) + negative tests
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/verify_lean_multikernel.ps1
python scripts/test_lean_negative.py

# 3. Verify dual-kernel SHA-256 parity & locked N=24 paired stats
.\bin\autophage.exe bench --epochs 200 --size 32
.\bin\autophage.exe stats --pairs 24 --epochs 100

# 4. Property checks, axiom/blueprint/timestamp parity, live citations, Tectonic white paper, Build Standard audit
python scripts/prop_check.py
python scripts/check_readme_axioms.py
python scripts/check_blueprint.py
python scripts/stamp_timestamps.py --verify
python scripts/check_citations.py
python scripts/build_paper.py
pwsh -NoProfile -File scripts/Audit-Repo.ps1
```

## 3. Replication Log

External independent replications are welcomed via GitHub Issues (label `replication`).

| Date | Replicator | Platform / CPU / Compilers | Scope Verified | Outcome & Canonical `bench` SHA-256 |
|---|---|---|---|---|
| 2026-11-26 | Alexander Stephen Donahue (Author, first-party) | Windows 11 x64 + WSL2 Ubuntu · Intel Core Ultra 9 275HX · Clang 23.1.0 + MinGW GCC 16.1.0 + Linux GCC 13.3.0 ASan/UBSan · Lean 4.35.0 · Tectonic | `v1.5.0`: 19/19 gates (incl. B07 Gate `T19`), 6/6 mutants, `prop_check`, 56/56 Lean proofs (reference kernel + 3 Rust checkers) + 5/5 negative tests, 17/17 Crossref/DataCite citations, interactive Blueprint, DigiCert `.tsr` + Bitcoin `.ots`, Tectonic PDF + clean-room arXiv tarball, Build Standard v1.2.0 (`100.0%`) | PASS · `4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612` (`0`-bit JIT==Scalar diff) |
| 2026-11-26 | GitHub Actions CI (`ubuntu-latest`, `windows-latest`) | Cloud x86-64 runners · GCC `-Werror` + ASan/UBSan · Lean 4.35.0 + `bubblewrap` (6/6 checkers + negative tests) · Tectonic · GitHub Pages | Automated CI push-to-`main` verification (`AUTOPHAGE_CI_PORTABLE=1`, ASan/UBSan Gate `T19`, 6-kernel `lake check` & `lake comparator`, `test_lean_negative.py`, `check_blueprint.py`, `stamp_timestamps.py --verify`, PDF & arXiv tarball, Pages deployment) | Automated on every push to `main` (see `.github/workflows/ci.yml` & `pages.yml`) |
| *Awaiting external* | *Independent external replicator* | *Any x86-64 Linux/Windows host* | *Full clean-clone reproduction above* | *Open — submit via GitHub Issue (`replication`)* |

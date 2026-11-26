# AUTOPHAGE v1.4.0 — Independent Clean-Clone Replication Protocol

Anyone with an x86-64 Windows or Linux machine can reproduce every theorem, falsification gate, mutant kill, dual-kernel SHA-256 hash, empirical benchmark, and white-paper PDF from a clean clone.

## 1. Prerequisites

- **C11 Toolchain:** Clang 18+ (`clang`, `lld`) or GCC 12+ (`gcc`), plus `node` (v20+, optional for Web Observatory) and `python3` (3.10+, zero external pip packages required).
- **Lean 4 Toolchain:** `elan` with pinned `leanprover/lean4:v4.35.0` (automatically read from `lean/lean-toolchain`). On Ubuntu Linux, install `bubblewrap` (`sudo apt-get install -y bubblewrap`) for `lake check --paranoid` and `lake comparator --paranoid`.
- **White Paper (Optional):** `tectonic` (`https://tectonic-typesetting.github.io`) for zero-install TeX compilation.

## 2. Step-by-Step Replication Commands

### A. Linux / WSL2 (Full 6-Kernel Lean + C11 + ASan/UBSan)
```bash
git clone https://github.com/0thernes/autophage.git
cd autophage

# 1. Native C11 build (-Wall -Wextra -Werror) + 18/18 gates + 6/6 mutants + 48 Lean proofs
#    (Add AUTOPHAGE_CI_PORTABLE=1 if CPU lacks hardware GFNI/VPCLMULQDQ)
bash build_linux.sh

# 2. AddressSanitizer + UndefinedBehaviorSanitizer run
AUTOPHAGE_SANITIZE=1 AUTOPHAGE_CI_PORTABLE=1 bash build_linux.sh

# 3. Verify 0-bit diff JIT vs Scalar canonical SHA-256 benchmark
./bin/autophage_linux bench --epochs 200 --size 32
# Expected SHA-256 (both JIT and SCALAR):
# 4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612

# 4. Run full 6-kernel Palomar verification + Challenge/Solution comparator
cd lean
lake build
lake build Challenge
lake env lean VerifyAxioms.lean > ../results/lean4-audit.txt
lake env leanchecker --fresh Solution
lake env leanchecker-paranoid --fresh Solution
lake check --paranoid
lake comparator --config comparator.json --paranoid
cd ..

# 5. Independent Python property checks, README axiom parity, citation & paper verification
python3 scripts/prop_check.py
python3 scripts/check_readme_axioms.py
python3 scripts/check_citations.py
python3 scripts/build_paper.py
bash scripts/audit-repo.sh .
```

### B. Windows 11 x64 (PowerShell 7)
```powershell
git clone https://github.com/0thernes/autophage.git
Set-Location autophage

# 1. Build Win64 PE + DLL + Static Lib + 18/18 Gates + 6/6 Mutants + 48 Lean Proofs
pwsh -NoProfile -ExecutionPolicy Bypass -File build.ps1

# 2. Run 5 Windows-capable independent Lean 4.35.0 kernels (lean, leanchecker, leanchecker-paranoid, con-leche, con-ron, nanoda_bin)
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/verify_lean_multikernel.ps1

# 3. Verify dual-kernel SHA-256 parity & locked N=24 paired stats
.\bin\autophage.exe bench --epochs 200 --size 32
.\bin\autophage.exe stats --pairs 24 --epochs 100

# 4. Property checks, README axiom parity, live citations, Tectonic white paper, Build Standard audit
python scripts/prop_check.py
python scripts/check_readme_axioms.py
python scripts/check_citations.py
python scripts/build_paper.py
pwsh -NoProfile -File scripts/Audit-Repo.ps1
```

## 3. Replication Log

External independent replications are welcomed via GitHub Issues (label `replication`).

| Date | Replicator | Platform / CPU / Compilers | Scope Verified | Outcome & Canonical `bench` SHA-256 |
|---|---|---|---|---|
| 2026-11-25 | Alexander Stephen Donahue (Author, first-party) | Windows 11 x64 · Intel Core Ultra 9 275HX · Clang 23.1.0 + lld · Lean 4.35.0 · Tectonic | 18/18 gates, 6/6 mutants, `prop_check`, 48/48 Lean proofs (5 Windows kernels), 17/17 live citations, Tectonic PDF + clean-room arXiv tarball, Build Standard v1.2.0 (`100.0%`) | PASS · `4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612` (`0`-bit JIT==Scalar diff) |
| 2026-11-25 | GitHub Actions CI (`ubuntu-latest`, `windows-latest`) | Cloud x86-64 runners · GCC `-Werror` + ASan/UBSan · Lean 4.35.0 + `bubblewrap` (6/6 kernels) · Tectonic | Automated CI push-to-`main` verification (`AUTOPHAGE_CI_PORTABLE=1`, 6-kernel `lake check` & `lake comparator`, PDF & arXiv tarball) | Automated on every push to `main` (see `.github/workflows/ci.yml`) |
| *Awaiting external* | *Independent external replicator* | *Any x86-64 Linux/Windows host* | *Full clean-clone reproduction above* | *Open — submit via GitHub Issue (`replication`)* |

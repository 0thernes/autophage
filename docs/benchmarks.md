# AUTOPHAGE v1.4.0 — Honest Benchmarks & Performance Budgets

All figures below were measured live on real hardware (**Intel Core Ultra 9 275HX**, AVX2 + AVX-VNNI + GFNI + PCLMULQDQ + VPCLMULQDQ + BMI1/2 + POPCNT + RDSEED + RDTSCP, Windows 11 x64 PE via Clang 23.1.0 + lld + ThinLTO + `-ffp-contract=off`, and Linux ELF64 via GCC `-O3 -Werror` on WSL2). Zero fabricated or mocked figures.

## 1. End-to-End 200-Epoch Dual-Kernel Benchmark (`32×32`, `173,484` Execs, Seed `5055`)

| Platform / Compiler / ABI | Kernel | Wall Time | Throughput | Latency / Cycles | Cache Hit Rate | Canonical SHA-256 |
|---|---|---|---|---|---|---|
| **Win64 PE64** (`clang 23.1.0`, ThinLTO, `v1.4.0`, `results/02_bench_win64.txt`) | Native RWX JIT | `0.7356 s` | `0.236 M execs/s` | `4239.9 ns/exec` (`1545.0 cyc`) | `27.35%` (`126,031` compiles) | `4bd29f39...3c20612` |
| **Win64 PE64** (`clang 23.1.0`, ThinLTO, `v1.4.0`, `results/02_bench_win64.txt`) | Scalar Reference | `0.7686 s` | `0.226 M execs/s` | `4430.5 ns/exec` | N/A | `4bd29f39...3c20612` |
| **Win64 PE64** (unloaded run, `v1.2.0`/`v1.3.0` baseline) | Native RWX JIT | `0.5124 s` | `0.339 M execs/s` | `2953.4 ns/exec` (`1161.4 cyc`) | `27.35%` (`126,031` compiles) | `4bd29f39...3c20612` |
| **Win64 PE64** (unloaded run, `v1.2.0`/`v1.3.0` baseline) | Scalar Reference | `0.5905 s` | `0.294 M execs/s` | `3403.5 ns/exec` | N/A | `4bd29f39...3c20612` |
| **Linux ELF64** (`gcc -O3`, WSL2, `results/02_bench_linux.txt`) | Native RWX JIT | `0.9306 s` | `0.186 M execs/s` | `5364.4 ns/exec` (`1476.4 cyc`) | `27.35%` (`126,031` compiles) | `4bd29f39...3c20612` |
| **Linux ELF64** (`gcc -O3`, WSL2, `results/02_bench_linux.txt`) | Scalar Reference | `1.2067 s` | `0.144 M execs/s` | `6955.9 ns/exec` | N/A | `4bd29f39...3c20612` |

### Honest Speedup Tradeoff Disclosure
End-to-end world speedup of `NATIVE_JIT` over `SCALAR_REF` is modest (`1.04×–1.15×` on Win64; `1.30×` on WSL2 Linux) because rapid somatic/germline mutation produces a **27.35% JIT cache hit rate** (`126,031` single-pass code emissions across `173,484` executions), and non-JIT ecological bookkeeping (heat/detritus torus diffusion, Galois mating, OVBS metrology) accounts for the majority of epoch time. By contrast, when executing cached organisms in Gate `T05` (`2,000` compiled genomes × `500` runs = **`1,000,000` live native RWX calls**), wall time is **`55–65 ms` (`~15.4–18.2 M execs/s`)** with `0` faults and `0` canary violations.

## 2. 18-Gate Falsification Suite Timings (`v1.4.0`, Win64 Clang 23.1.0 ThinLTO)

| Gate | Description | Measured Wall Time (`v1.4.0`) | Budget | Status |
|---|---|---|---|---|
| `T01`–`T03` | CPUID, RWX, SHA-256, GFNI 500×256, 30 polys + PCLMUL 1,000× | `0.000s`, `0.003s`, `0.000s` | `< 0.100s` | PASS |
| `T04` | JIT vs. Scalar Oracle bit-exactness (5,000 random genomes) | `0.041s` | `< 0.500s` | PASS |
| `T05` | 1,000,000 live RWX fuzz executions + 64B OOB canaries | `0.065s` (`15.4 M/s`) | `< 0.500s` | PASS |
| `T06`–`T08` | Landauer, 1st/2nd Law + Mutants 1&2, Philox + Mutant 6 | `0.000s`, `0.060s`, `0.019s` | `< 0.500s` | PASS |
| `T09` | Full-world 60-epoch dual-kernel SHA-256 parity | `0.494s` | `< 2.000s` | PASS |
| `T10`–`T12` | Observer independence + Mutant 5, `.apbin` tamper, 0 allocs | `0.188s`, `0.084s`, `0.065s` | `< 1.000s` | PASS |
| `T13` | Spontaneous zero-seed abiogenesis phase transition (160 ep) | `0.512s` | `< 2.500s` | PASS |
| `T14`–`T18` | Anti-gaming + Mutants 3&4, Planarian ECC, Weismann, Backlash, 112-trait | `0.001s`, `0.001s`, `0.147s`, `0.352s`, `0.454s` | `< 2.500s` | PASS |
| **Total 18-Gate Suite** | **All 18 gates + 6 planted mutants** | **`2.486 s`** | **`< 10.0 s`** | **18/18 PASS** |

## 3. Formal Verification Performance Budgets (Lean 4.35.0)

| Check | Measured Local Time (`v1.4.0`, Win64) | Budget | Status |
|---|---|---|---|
| `lake build` (`Spec` + `AutophageProofs` + `Solution`, 48 theorems) | `~38.4 s` | `< 120 s` | PASS |
| `lake build Challenge` | `~1.2 s` | `< 15 s` | PASS |
| `leanchecker --fresh` + `leanchecker-paranoid --fresh` + `leanexport` (23.22 MB) + `con-leche` + `con-ron` + `nanoda_bin` | `~170 s` total | `< 600 s` | PASS |

## 4. Explicit Performance Budgets (`A37`) & Tracking (`B27`)

- **Memory Allocation Budget (`P10`):** Strictly **`0` heap allocations** during `autophage_step_epochs()` (enforced continuously by `g_ap_in_step_guard` in Gate `T12`).
- **JIT Arena Budget:** Fixed `16 MiB` (`4,096` slots × `4 KiB`), zero dynamic resizing.
- **Thermodynamic Conservation Budget:** `int_ledger_residual_uq == 0` and `relative_drift < 1e-12` (measured `<= 2.02e-16` across all 30 ablation conditions).
- **CI Performance Tracking (`B27`):** `.github/workflows/ci.yml` executes `./bin/autophage_linux bench --epochs 200 --size 32` on every push to `main` (report-only) to track cross-commit throughput and verify canonical SHA-256 invariance.

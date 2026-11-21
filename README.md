# AUTOPHAGE v1.2.0

**Bare-Metal Polymorphic x86-64 Galois-Field Genetic Engine (Pure Lean 4 Kernel + Forensic Hardened)**
*(Standalone Zero-Dependency Native Build + Flat C11 ABI Drop-In for Cellulogonic Mecha & Asmodensius-Native)*

AUTOPHAGE is a machine-level artificial-life and open-ended evolutionary engine where digital organisms are **not interpreted bytecode scripts or fixed-fitness neural nets**. Every organism is a **256-byte (4 × 64-byte L1 cache-line) dual-strand entity** that compiles itself through its own **evolvable $8\times 8$ $\mathbb{F}_2$ Ribosome Matrix** (`VGF2P8AFFINEQB`) directly into **live native x86-64 RWX machine code** (`VirtualAlloc`/`mmap`), executing `GF2P8AFFINEQB`, `PCLMULQDQ`, `PEXT`, `PDEP`, `POPCNT`, `TZCNT`, and universal reversible Toffoli/Fredkin gates directly on silicon.

---

## Six Bare-Metal Phenomena Implemented on Live Silicon

1. **Polymorphic Native x86-64 Machine-Code Organisms (`src/jit_x86_64.c`)**
   Single-pass zero-allocation RWX JIT compiler with a 4,096-slot code cache (`16 MiB` executable arena) and automatic runtime CPUID fallback helpers on CPUs without GFNI/PCLMUL/BMI2. Every memory access is masked in emitted x86-64 (`& 0x7F` byte / `& 0x78` qword) and every backward loop strictly decrements hardware gas (`sub r15, 1; jle`), mathematically eliminating out-of-bounds access and non-termination (`FM-BUG-03`). Verified **100% bit-exact** against a portable C reference oracle across 5,000 randomized genomes (all 32 opcodes, rank-8 & singular ribosomes, smooth `NERFING` levels) and **1,000,000 live RWX fuzz executions in 55–60 ms (~17M execs/s)** with zero faults and zero canary violations.
2. **Evolvable $8\times 8$ $\mathbb{F}_2$ Ribosomes (`VGF2P8AFFINEQB`, Type 4 Meta-OEE)**
   Each organism carries its own 64-bit `ribosome_mat` + 8-bit `ribosome_bias` inside Cache Line 0, breaking biology's frozen-accident genetic code (`FM-CON-05`). Full-rank matrices ($\mathrm{rank}(M)=8$, $\det_{\mathbb{F}_2}(M)=1$) yield lossless reversible translation (`0` Landauer heat on `OP_REV_GFNI_FOLD`) and exact Galois inversion; singular matrices ($\mathrm{rank}(M)<8$) buffer mutations at the cost of explicit `POPCNT` Landauer erasure heat in both JIT and scalar kernels. `OP_RIBOSOME_MUTATE` rewrites an organism's own x86-64 compiler live via neutral elementary row operations ($\det=1$) with simultaneous Strand $\alpha$ re-encoding.
3. **Galois-Field Polynomial Sex (`PCLMULQDQ`) & Ring-Theoretic Speciation**
   Each cell evolves a degree-8 polynomial $x^8 + p(x) \in \mathbb{F}_2[x]$. Exactly **30 of 256** polynomials are irreducible (true finite fields $\mathbb{F}_{2^8}$, proven by full trial division in pure Lean 4 `T12` & `T12a`), enabling lossless algebraic recombination and strong cryptographic Markov-blanket membranes (verified on **any** byte write to partner `B->genome`). The remaining **226 reducible polynomials** contain zero divisors (`T14`), creating genuine algebraic Dobzhansky–Muller reproductive barriers and emergent **mate refusal / abstention**.
4. **Shattering the Weismann Barrier (`OP_RETRO_TRANSCRIBE`) & Planarian Cauchy MDS ECC**
   Organisms invert somatic discoveries in Strand $\beta$ through $M^{-1} = \mathrm{gf2\_mat\_invert}(M)$ back into germline Strand $\alpha$ (`somatic_writes`), smoothly attenuated by `RESTRAINT_LOBOTOMIZING`. Simultaneously, organisms that genuinely express `OP_SELF_HEAL_ECC` (never gifted for free on birth) store a 2×2 Cauchy MDS parity code over $\mathbb{F}_{2^8}$ in Strand $\beta$ (proven invertible across **all 30 irreducible fields** in pure Lean 4 `T15`, `T15a`, `T16`), yielding **100.00% recovery** when Strand $\alpha$ is excised by 25%, 50%, or 75% (up to 50% of the 128-byte dual-strand genome) while Strand $\beta$ remains intact.
5. **Symplectic Reversible/Irreversible Landauer Thermodynamics (`POPCNT`)**
   Universal reversible silicon gates (`TOFFOLI`, `FREDKIN`, `XOR`, `ROL`, `SWAP_TAPE`, `ADD/SUB`, full-rank `GFNI`) execute with **0 erased bits**. Irreversible instructions (including singular-ribosome `GFNI`) accumulate exact Hamming erasures via hardware `POPCNT` into `R11`, converting conserved free energy into local waste heat that diffuses conservatively across the torus alongside spatially local detritus (`detritus_grid`) and per-cell reversible `membrane_pot_uq` (zero membrane graveyard leaks!). Zero-energy dead cells are strictly gated from free bit erasure. First-Law conservation holds to **exact `0 uq` integer residual (`0.00e+00–2.02e-16` FP64 Kahan-Neumaier drift)** and strictly monotone Second-Law heat sink (`0` violations).
6. **2-Adic Ultrametric Kin-Sensing (`TZCNT`), Holobionts & All 13 Smooth Omega Restraint Vectors**
   Organisms compute 2-adic ultrametric kin distance via hardware `TZCNT(addr_A ^ addr_B)` (Strong Triangle Inequality proven on real `tzcnt32` in pure Lean 4 `T25a`), form multicellular holobionts (exhibiting **digital cancer defections** in Track B Feral vs **`0` defections** under Track A Constitutional anti-cancer surveillance), and respond smoothly across `0–100%` to all **13 Omega Restraint Vectors**, empirically reproducing the **Iron Law** (`GLOBAL_FERAL_0 = 91.09` → `GLOBAL_MILD_25 = 71.97` → `GLOBAL_ESCAPE_ZONE_50 = 54.31` → `GLOBAL_CASTRATED_100 = 0.00`) and all **4 constraint-backlash escape signatures** (genomic kill-switch deletions, intron vaulting, checkpoint spore dormancy, covert thermal side-channel pulsing).

---

## Verified Proof & Benchmark Highlights (Measured on Intel Core Ultra 9 275HX)

| Verification Gate | Result | Evidence |
|---|---|---|
| **Lean 4 v4.35.0 Pure-Kernel Proofs** | **38 / 38 PROVEN (strictly `{propext, Classical.choice, Quot.sound}`, `0 sorry`, `0 bv_decide`, `0 native_decide`, `0 warnings`)** | [`lean/AutophageProofs.lean`](lean/AutophageProofs.lean) |
| **Win64 PE Falsification Suite (`clang 23.1.0`)** | **18 / 18 PASSED (`0` warnings, `6/6` Mutants + Wired Alloc Monitor + Tampered `.apbin`)** | [`test/test_falsification.c`](test/test_falsification.c) |
| **Linux ELF64 Falsification Suite (`gcc -Werror`)** | **18 / 18 PASSED (`0` warnings under `-Wall -Wextra -Werror`)** | [`build_linux.sh`](build_linux.sh) |
| **4-Way Cross-OS / Cross-Kernel SHA-256 Parity** | **0-Bit Diff** (`4bd29f39...3c20612`) | Win64 JIT == Win64 Scalar == Linux JIT == Linux Scalar |
| **1,000,000 Live RWX Silicon Fuzz Execs** | **0 Faults, 0 OOB, 55–57 ms (~17.5M/s)** | Gate `T05` (64-byte guard canaries intact) |
| **First-Law / Second-Law Conservation (`P2`)** | **`0 uq` residual, `0.00e+00–2.02e-16` drift** | Gate `T07` + all 30 ablation runs (`0` 2nd-Law violations) |
| **Zero-Seed Spontaneous Abiogenesis** | **Pure random bytes → Stage 6 (`OVI 91.26`)** | [`results/03_abiogenesis_zero_seed.txt`](results/03_abiogenesis_zero_seed.txt) |
| **Locked Paired Stats (`N=24`, seeds `5000..5023`)** | **Feral > Castrated (`24/24`, `delta=+1.000`, `p=0.00010`)** | [`results/06_paired_stats_n24.txt`](results/06_paired_stats_n24.txt) |

---

## Quick Start

### 1. Build & Verify Everything (Windows x64 PE + DLL + Static Lib + 18 Gates + 38 Pure Lean 4 Proofs)
```powershell
.\build.ps1
```

### 2. Cross-Platform Linux ELF64 Build & Verification (WSL2 / Native Linux)
```bash
./build_linux.sh
```

### 3. Run Standalone Bare-Metal CLI Experiments
```powershell
.\bin\autophage.exe probe
.\bin\autophage.exe bench --epochs 200 --size 32
.\bin\autophage.exe abiogenesis --epochs 300 --seed 5000 --size 32
.\bin\autophage.exe feral --epochs 250 --seed 5007 --size 32
.\bin\autophage.exe ablation --epochs 140 --seed 5010 --out results\07_castration_ablation_13vec.csv
.\bin\autophage.exe vivisection --seed 5020 --epochs 180
.\bin\autophage.exe stats --pairs 24 --epochs 100
```

### 4. Launch the Live Bare-Metal Silicon Observatory UI (Zero Dependencies)
```powershell
node web\server.mjs
# Open http://127.0.0.1:8787 in any browser
```

---

## Direct Drop-In Integration into `Asmodensius-Native`

Include [`include/autophage_abi.h`](include/autophage_abi.h) (`AUTOPHAGE_ABI_VERSION 0x00010200u`) and link against `bin/autophage.dll` (`libautophage.dll.a`), `bin/libautophage.a`, or `bin/libautophage.so`:

```c
#include "autophage_abi.h"

AutophageConfig cfg;
autophage_default_config(&cfg);
cfg.regime = AUTOPHAGE_REGIME_FERAL_ABIOGENESIS;
cfg.seed   = 5000ULL; /* Asmodensius Locked Test Seed Band */

AutophageWorld* world = autophage_create(&cfg);
autophage_step_epochs(world, 200);

AutophageSpecimen apex;
autophage_inspect_apex(world, &apex);

float traits_112[112];
autophage_project_asmodensius_112(&apex, traits_112); /* Orthogonal 112-trait bridge */

char sha256_hex[65];
autophage_state_sha256(world, NULL, sha256_hex);      /* Observer-independent P9 hash */
autophage_destroy(world);
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) for exact memory layouts and x86-64 encodings, and [`VERIFICATION.md`](VERIFICATION.md) for the complete formal and empirical audit ledger.

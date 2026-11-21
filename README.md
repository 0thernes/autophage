# AUTOPHAGE v1.0

**Bare-Metal Polymorphic x86-64 Galois-Field Genetic Engine**
*(Standalone Zero-Dependency Native Build + Flat C11 ABI Drop-In for Asmodensius-Native)*

AUTOPHAGE is a machine-level artificial-life and open-ended evolutionary engine where digital organisms are **not interpreted bytecode scripts or fixed-fitness neural nets**. Every organism is a **256-byte (4 × 64-byte L1 cache-line) dual-strand entity** that compiles itself through its own **evolvable $8\times 8$ $\mathbb{F}_2$ Ribosome Matrix** (`VGF2P8AFFINEQB`) directly into **live native x86-64 RWX machine code** (`VirtualAlloc`/`mmap`), executing `GF2P8AFFINEQB`, `PCLMULQDQ`, `PEXT`, `PDEP`, `POPCNT`, `TZCNT`, and universal reversible Toffoli/Fredkin gates directly on silicon.

---

## Six Bare-Metal Phenomena Implemented on Live Silicon

1. **Polymorphic Native x86-64 Machine-Code Organisms (`src/jit_x86_64.c`)**
   Single-pass zero-allocation RWX JIT compiler with a 4,096-slot code cache (`16 MiB` executable arena). Every memory access is masked in emitted x86-64 (`& 0x7F` byte / `& 0x78` qword) and every backward loop strictly decrements hardware gas (`sub r15, 1; jle`), mathematically eliminating out-of-bounds access and non-termination (`FM-BUG-03`). Verified **100% bit-exact** against a portable C reference oracle across 5,000 randomized genomes and **1,000,000 live RWX fuzz executions in 48–50 ms (~20M execs/s)** with zero faults and zero canary violations.
2. **Evolvable $8\times 8$ $\mathbb{F}_2$ Ribosomes (`VGF2P8AFFINEQB`, Type 4 Meta-OEE)**
   Each organism carries its own 64-bit `ribosome_mat` + 8-bit `ribosome_bias` inside Cache Line 0, breaking biology's frozen-accident genetic code (`FM-CON-05`). Full-rank matrices ($\mathrm{rank}(M)=8$, $\det_{\mathbb{F}_2}(M)=1$) yield lossless reversible translation (`0` Landauer heat) and exact Galois inversion; singular matrices ($\mathrm{rank}(M)<8$) buffer mutations at the cost of Landauer degeneracy heat. `OP_RIBOSOME_MUTATE` rewrites an organism's own x86-64 compiler live via neutral elementary row operations (`det=1`) with simultaneous Strand $\alpha$ re-encoding.
3. **Galois-Field Polynomial Sex (`PCLMULQDQ`) & Ring-Theoretic Speciation**
   Each cell evolves a degree-8 polynomial $x^8 + p(x) \in \mathbb{F}_2[x]$. Exactly **30 of 256** polynomials are irreducible (true finite fields $\mathbb{F}_{2^8}$, proven in Lean 4 `T12`), enabling lossless algebraic recombination and strong cryptographic Markov-blanket membranes. The remaining **226 reducible polynomials** contain zero divisors (proven in `T14`), creating genuine algebraic Dobzhansky–Muller reproductive barriers and emergent **mate refusal / abstention**.
4. **Shattering the Weismann Barrier (`OP_RETRO_TRANSCRIBE`) & Planarian Cauchy ECC**
   Organisms invert somatic discoveries in Strand $\beta$ through $M^{-1} = \mathrm{gf2\_mat\_invert}(M)$ back into germline Strand $\alpha$ (`somatic_writes`), unless explicitly blocked by `RESTRAINT_LOBOTOMIZING`. Simultaneously, `OP_SELF_HEAL_ECC` stores a holographic 2×2 Cauchy MDS parity code over $\mathbb{F}_{2^8}$ in Strand $\beta$ (proven non-singular in all 30 fields in Lean 4 `T15`), yielding **100% recovery under 25%, 50%, and 75% genomic excision** (`T16`).
5. **Symplectic Reversible/Irreversible Landauer Thermodynamics (`POPCNT`)**
   Universal reversible silicon gates (`TOFFOLI`, `FREDKIN`, `XOR`, `ROL`, `SWAP_TAPE`, `ADD/SUB`, invertible `GFNI`) execute with **0 erased bits**. Irreversible instructions accumulate exact Hamming erasures via hardware `POPCNT` into `R11`, converting conserved free energy into local waste heat that diffuses conservatively across the torus and triggers thermal SOS hypermutation above threshold. First-Law conservation holds to **exact `0 uq` integer residual (`< 1e-15` FP64 Kahan-Neumaier drift)** and strictly monotone one-way Second-Law heat sink (`0` violations).
6. **2-Adic Ultrametric Kin-Sensing (`TZCNT`), Holobionts & All 13 Omega Castration Vectors**
   Organisms compute 2-adic ultrametric kin distance via hardware `TZCNT(addr_A ^ addr_B)` (`T25`), form multicellular holobionts (exhibiting **digital cancer defections** in Track B Feral vs **`0` defections** under Track A Constitutional anti-cancer surveillance), and respond physically to all **13 Omega Restraint Vectors (`0–100%`)**, empirically reproducing the **Adversarial Escape-Breeding Paradox** at 50% partial restraint (kill-switch deletions, intron vaulting, checkpoint spore dormancy, covert thermal side-channel pulsing).

---

## Verified Proof & Benchmark Highlights (Measured on Intel Core Ultra 9 275HX)

| Verification Gate | Result | Evidence |
|---|---|---|
| **Lean 4 v4.35.0 Formal Proofs** | **30 / 30 PROVEN (`0 sorry`)** | [AutophageProofs.lean](file:///C:/Users/Alexa/.gemini/antigravity/scratch/autophage/lean/AutophageProofs.lean) |
| **Win64 PE Falsification Suite (`clang 23.1.0`)** | **18 / 18 PASSED (6/6 Mutants Caught)** | [test_falsification.c](file:///C:/Users/Alexa/.gemini/antigravity/scratch/autophage/test/test_falsification.c) |
| **Linux ELF64 Falsification Suite (`gcc 13.3.0`)** | **18 / 18 PASSED (6/6 Mutants Caught)** | [build_linux.sh](file:///C:/Users/Alexa/.gemini/antigravity/scratch/autophage/build_linux.sh) |
| **4-Way Cross-OS / Cross-Kernel SHA-256 Parity** | **0-Bit Diff** (`28fcbf73...316b7f5`) | Win64 JIT == Win64 Scalar == Linux JIT == Linux Scalar |
| **1,000,000 Live RWX Silicon Fuzz Execs** | **0 Faults, 0 OOB, 48–50 ms (~20M/s)** | Gate `T05` (64-byte guard canaries intact) |
| **First-Law / Second-Law Conservation (`P2`)** | **`0 uq` residual, `0.00e+00–2.0e-16` drift** | Gate `T07` + all 30 ablation runs (`0` 2nd-Law violations) |
| **Zero-Seed Spontaneous Abiogenesis** | **Pure random bytes → Stage 5 (`OVI 87.59`)** | [03_abiogenesis_zero_seed.txt](file:///C:/Users/Alexa/.gemini/antigravity/scratch/autophage/results/03_abiogenesis_zero_seed.txt) |
| **Locked Paired Stats (`N=24`, seeds `5000..5023`)** | **Feral > Castrated (`23/24`, `p=0.00010`)** | [06_paired_stats_n24.txt](file:///C:/Users/Alexa/.gemini/antigravity/scratch/autophage/results/06_paired_stats_n24.txt) |

---

## Quick Start

### 1. Build & Verify Everything (Windows x86-64 PE + DLL + Static Lib + 18 Gates + 30 Lean Proofs)
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

Include [autophage_abi.h](file:///C:/Users/Alexa/.gemini/antigravity/scratch/autophage/include/autophage_abi.h) and link against `bin/autophage.dll` (`libautophage.dll.a`), `bin/libautophage.a`, or `bin/libautophage.so`:

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
autophage_project_asmodensius_112(&apex, traits_112); /* Deterministic 112-trait bridge */

char sha256_hex[65];
autophage_state_sha256(world, NULL, sha256_hex);      /* Observer-independent P9 hash */
autophage_destroy(world);
```

See [ARCHITECTURE.md](file:///C:/Users/Alexa/.gemini/antigravity/scratch/autophage/ARCHITECTURE.md) for exact memory layouts and x86-64 encodings, and [VERIFICATION.md](file:///C:/Users/Alexa/.gemini/antigravity/scratch/autophage/VERIFICATION.md) for the complete formal and empirical audit ledger.

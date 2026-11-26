# AUTOPHAGE v1.4.0 — Limitations, Threats to Validity & Real-vs-Metaphor Boundary

This document establishes what AUTOPHAGE **does** and **does not** prove, separating bare-metal software engineering and finite algebraic theorems from biological or physical claims.

## 1. Real vs. Metaphor Boundary

AUTOPHAGE uses biologically inspired nomenclature ("ribosome", "Strand α / Strand β", "Weismann barrier", "centromere", "holobiont", "digital cancer", "Planarian regeneration", "abiogenesis"). These terms denote **exact deterministic data structures and x86-64 operations**, NOT biochemical identity:

| Term in AUTOPHAGE | Exact Computational Implementation | What It Is **NOT** |
|---|---|---|
| **Evolvable Ribosome (`M`, `bias`)** | 64-bit $\mathbb{F}_2^{8\times 8}$ affine matrix executed by `VGF2P8AFFINEQB` translating 64 germline bytes into 64 expressed opcodes | Not a 3D ribonucleoprotein complex or wet-lab aminoacyl-tRNA synthetase |
| **Zero-Seed Abiogenesis** | 1,024 slots of `Philox4x32-10` random bytes running in a pre-engineered 32-opcode x86-64 physics with an explicit conserved energy ledger and copy opcodes | Not prebiotic wet chemistry (`BIO-001`, `CHEM-052`); the opcode ISA and replication trap semantics are designed, not emergent from quantum mechanics |
| **Landauer Thermodynamics (`POPCNT`)** | Exact integer micro-quantum (`uq`) ledger charging `popcnt(old ^ new)` erased bits on irreversible instructions and `0` on reversible gates (`TOFFOLI`/`FREDKIN`/rank-8 `GFNI`) | Not physical transistor-level calorimetry ($k_B T \ln 2$); physical CMOS CPUs dissipate orders of magnitude more heat even on logically reversible instructions |
| **Weismann Barrier Shattering** | Applying $M^{-1}$ (`gf2_mat_invert`) to map Strand $\beta$ somatic bytes back into germline Strand $\alpha$ | Not biological reverse transcriptase or Lamarckian epigenetics in wet-lab organisms |
| **Planarian 50% Regeneration** | 2×2 Cauchy MDS parity over $\mathbb{F}_{2^8}$ stored in Strand $\beta$ reconstructing excised bytes of Strand $\alpha$ | Not cellular blastema morphogenesis or bioelectric anatomical remodeling |
| **Digital Cancer & Constitutional Immunity** | Holobiont-bonded cells exploiting shared resources vs. rule-based immune severance in `AUTOPHAGE_REGIME_CONSTITUTIONAL` | Not oncology or clinical immunology |

## 2. Formal Verification Scope Boundaries (Lean 4.35.0)

Our 48 Pure Lean 4.35.0 theorems (`lean/AutophageProofs.lean`) verify the **mathematical specifications** of GFNI affine maps, `PCLMULQDQ` carry-less products, 30 irreducible degree-8 polynomials, 2×2 Cauchy MDS decoding, 64-bit Toffoli/Fredkin involutions and `popcnt64` conservation, 32-bit `tzcnt32` ultrametricity, JIT mask bounds, gas decrement, 5-pool integer transfer conservation, Philox 8-tuple disjointness, 3-tier seed bands, OEE archive growth, and Weismann soma isolation.

They do **NOT** formally verify inside Lean:
1. **Raw x86-64 machine-code emission (`src/jit_x86_64.c`)** — verified empirically and differentially against `jit_reference_oracle` across 5,000 randomized genomes (`T04`), full 60-epoch worlds (`T09`), `1,000,000` canary-guarded fuzz runs (`T05`), and libFuzzer (`test/fuzz_jit_libfuzzer.c`), not via a formally verified compiler backend like CompCert.
2. **IEEE-754 floating-point Kahan-Neumaier summation (`k_inflow_kj`, `k_export_kj`, `k_heat_radiated_kj`)** — Lean proves exact `Int` micro-quantum closure (`T29`, `T29a`, `T45`); FP64 relative drift (`<= 2.02e-16`) is verified empirically (`T07`) and stabilized with `-ffp-contract=off`.
3. **Full C11 memory safety of `soup_engine.c` / `metrology_ovbs.c`** — verified via `-Wall -Wextra -Werror`, ASan/UBSan (`-fsanitize=address,undefined` in CI), and `T05`/`T11`/`T12`, not via formal C verification (Frama-C/VST).
4. **Windows `lean4lean` / `bubblewrap` gap** — on Windows hosts, `lean4lean.exe` cannot parse Windows `.olean` path headers and `lake check` / `lake comparator` require Linux `bubblewrap` (`bwrap`). Five independent checkers run natively on Windows, while all 6/6 kernels + `lake comparator --paranoid` execute on Ubuntu Linux in `.github/workflows/ci.yml`.

## 3. Empirical & Performance Limitations

- **JIT Speedup Is Workload-Dependent (Honest Tradeoff):** In the 200-epoch `32×32` evolutionary benchmark (`results/02_bench_win64.txt`), rapid genome mutation yields a **27.35% JIT cache hit rate** (`126,031` single-pass compiles out of `173,484` executions). Consequently, end-to-end world speedup of `NATIVE_JIT` over `SCALAR_REF` is modest (`1.04×–1.15×` on Win64; `1.30×` on WSL2 Linux) because JIT compilation overhead and soup-engine ecology dominate wall time. Pure cached execution (`T05`, `1,000,000` calls across 2,000 cached genomes) reaches `~15.4–18.2 M execs/s` (`55–65 ms`).
- **Finite Grid & Horizon:** Default experiments run on `32×32` (`1,024` cells) or `16×16` (`256` cells) for `100–300` epochs. Demonstrating true unbounded open-ended evolution (`AI-045`) over millions of epochs and unbounded memory spaces remains open.
- **Single-Threaded Canonical Execution:** `threads = 1` is enforced to preserve exact cross-OS SHA-256 bit-determinism (`0`-bit diff across Win64 PE and Linux ELF64).
- **Single-Hardware Primary Silicon Profile:** Hardware `VGF2P8AFFINEQB` + `VPCLMULQDQ` timings were measured on an Intel Core Ultra 9 275HX; older CPUs without GFNI fall back transparently to bit-exact scalar trampolines at lower throughput.
- **No External Peer Review:** This software, proof suite, and white paper are independent research works (`self-assessed`) and have not undergone journal or conference peer review.

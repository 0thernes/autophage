# AUTOPHAGE v1.5.0 — Formal Verification & Empirical Proof Ledger

All measurements below were produced directly by compiling and executing **AUTOPHAGE v1.5.0** (`ABI 0x00010500`) on live bare-metal hardware (**Intel Core Ultra 9 275HX**, AVX2 + AVX-VNNI + GFNI + VPCLMULQDQ + PCLMULQDQ + BMI1/2 + POPCNT + RDSEED + RDTSCP; **Windows 11 x64 PE** via Clang 23.1.0 + lld + ThinLTO + `-ffp-contract=off` and MinGW GCC 16.1.0 `-Werror`, **Linux ELF64** via GCC `-Wall -Wextra -Werror` + `-fsanitize=address,undefined`, and **Lean 4.35.0**). Zero fabricated, cached, or mocked figures.

> **v1.5.0 Deep Hardening Summary:** Fixed live Ubuntu CI ASan/UBSan AVX-512 FakeStack `SIGSEGV` (`#45904159100`, GCC PR101000) via `AP_ALIGN64` (`_Alignas(64)`) + `-march=x86-64` / `-mno-avx512f` + permanent B07 regression **Gate `T19`** (`19 / 19` gates + `6 / 6` planted mutants). Replaced trivial `T30` with constructive multi-step `T30_ledger_heat_sink_monotone_and_zero_weight_gate` over `Spec.lean`'s `LedgerState`/`applyTransfers`, added `lean/AdversarialTests.lean` (`AT01`–`AT08` → **`56 / 56` Pure Lean 4.35.0 declarations**), and added live negative elaboration & tamper rejection (`scripts/test_lean_negative.py`). Checked locally on Windows by the Lean reference kernel (`lean4`, `leanchecker`, `leanchecker-paranoid`) + 3 independent Rust checkers (`nanoda_bin`, `con-leche`, `con-ron`) and in Ubuntu CI by all 6 Palomar checkers (`+ lean4lean` + `bubblewrap` `lake comparator --paranoid`). Closed all 5 frozen white-paper errata (`ERRATA.md`), upgraded citation audit to live Crossref/DataCite title/author/year/retraction verification (`17 / 17` in `results/citation-audit.json`), published interactive Lean Blueprint & GitHub Pages (`https://0thernes.github.io/autophage/`), and anchored `CHECKSUMS.sha256` with DigiCert RFC 3161 (`.tsr`) + 3-calendar Bitcoin OTS (`.ots`).

---

## 1. Pure-Kernel Lean 4.35.0 Mathematical Verification (`56 / 56` Declarations, Zero External Axioms)

Sources: [`lean/AutophageProofs/Spec.lean`](lean/AutophageProofs/Spec.lean), [`lean/AutophageProofs.lean`](lean/AutophageProofs.lean), [`lean/AdversarialTests.lean`](lean/AdversarialTests.lean), [`lean/Challenge.lean`](lean/Challenge.lean), [`lean/Solution.lean`](lean/Solution.lean) · Audit Logs: [`results/lean4-audit.txt`](results/lean4-audit.txt) · [`results/lean4-multi-kernel.txt`](results/lean4-multi-kernel.txt) — `0` errors, `0` warnings in `AutophageProofs`/`AdversarialTests`/`Solution`, `0` `sorry` in `Solution`, `0` `admit`, `0` `bv_decide`, `0` `native_decide`, `0` custom axioms. Every `#print axioms` output lies strictly within `{propext, Classical.choice, Quot.sound}`. Scope: all 56 declarations prove properties of the discrete models in `Spec.lean` (not emitted x86-64 machine code); five (`T21`, `T23`, `T24`, `T28`, `T29`) are explicitly tagged elementary arithmetic/XOR helper lemmas (`docs/statement-fidelity.md`).

| Group | Declarations | Count | Pure-Kernel Mathematical Properties Machine-Proven (`Spec.lean` Models) |
|---|---|---|---|
| **I. $\mathbb{F}_2^{8\times 8}$ Ribosome (`GFNI`)** | `T01`–`T07` (`T03a`, `T05a`) | 9 | Exact Intel `GF2P8AFFINEQB` identity (`0x0102040810204080`), bias involution, universal `gfniApply` XOR-linearity (`T03a`), zero fixed point, general affine inverse (`T05a`), asymmetric Weismann $M^{-1}$ round-trip (`T05`), and universal `(i j : Fin 8)` elementary row-swap (`T06`) and row-shear (`T07`) involutions. |
| **II. $\mathbb{F}_2[x]$ Speciation & Cauchy MDS ECC** | `T08`–`T16a` (`T12a`, `T15a`) | 12 | `clmul8` commutativity, zero/one laws, XOR distributivity, exact count of 30 irreducible / 226 reducible degree-8 polynomials (`T12`) matching full trial division `d = 2..31` and literal C11 `GF2_IRRED_POLY8[30]` (`T12a`), AES `0x1B` irreducibility (`T13`), Dobzhansky–Muller zero-divisor (`0x03 * 0xFF = 0` mod `0x01`, `T14`), Cauchy determinant invertibility (`T15`, `T15a`), and universal 2×2 Cauchy MDS recovery across all 30 irreducible fields (`T16`) + AES corollary (`T16a`). |
| **III. Symplectic Gates & Bit-Erasure** | `T17`–`T22` (`T20a`) | 7 | 64-bit `TOFFOLI` and `FREDKIN` exact self-involution (`T17`, `T18`), Fredkin-only XOR, AND, and exact `popcnt64` Hamming-weight conservation (`T19`, `T20`, `T20a`), elementary helper `T21` (`x ^^^ x = 0`), and exact `AND` erased bit-set (`T22`). |
| **IV. 2-Adic Ultrametric (`TZCNT`)** | `T23`–`T25a` | 4 | Elementary helpers `T23` (`a ^^^ a = 0`) & `T24` (`a ^^^ b = b ^^^ a`), mask ultrametricity (`T25`), and exact 2-adic Strong Triangle Inequality on real `tzcnt32` (`T25a`). |
| **V. JIT Mask/Gas Arithmetic & 5-Pool Ledger** | `T26`–`T30` (`T29a`) | 6 | Masked byte access `< 128` (`T26`), masked qword access `off + 8 <= 128` (`T27`), elementary helpers `T28` (`g - 1 < g`) & `T29` (`(a-t)+(b+t)=a+b`), arbitrary multi-step 5-pool ledger conservation (`T29a`), and constructive multi-step radiated-heat sink monotonicity + residual conservation + `oviGate 0 raw_ovi = 0` over `LedgerState`/`applyTransfers` (`T30`). |
| **VI. Philox, Seed Firewall, OEE & Weismann** | `T39`–`T42` | 4 | Distinct Philox domain tags yield strictly distinct 8-tuple counter keys (`T39`); pairwise disjointness of Practice (`100..999`), Tuning (`2000..2999`), and Locked (`>= 5000`) seed tiers (`T40`); OEE lineage archive strict size growth and monotone cumulative novelty (`T41`); Weismann soma-only perturbation preserving germline `BitVec 64` identically (`T42`). |
| **VII. Non-Vacuity & Sharp Boundaries** | `T43`–`T48` | 6 | Concrete non-involutive asymmetric ribosome witness (`T43`), irreducible Cauchy vs. reducible sentinel witness (`T44`), 8-op multi-step ledger + 1-uq leak detection witness (`T45`), 3-tier seed & Philox witness (`T46`), refutation of reducible quotient integral domains (`T47`), and refutation of unmasked / `& 0x7F` qword tape heads (`T48`). |
| **VIII. Adversarial False-Variant Refutations** | `AT01`–`AT08` (`AdversarialTests.lean`) | 8 | Concrete `¬P` kernel refutations: Toffoli popcount failure (`AT01`), singular rank-7 ribosome non-invertibility (`AT02`), reducible `0x01` divisor & Cauchy failure (`AT03`), unguarded ledger transfer & negative heat radiation (`AT04`), nonzero `AND` erasure (`AT05`), wrong-mask `PEXT`/`PDEP` bound (`AT06`), unconstrained Philox/seed collisions (`AT07`), and germline write breaking isolation (`AT08`). |

---

## 2. 19-Gate Bare-Metal Silicon Falsification Suite (+ 6 Planted Negative Mutants)

Source: [`test/test_falsification.c`](test/test_falsification.c) — **19/19 PASSED on Win64 PE (`clang 23.1.0` ThinLTO & MinGW GCC 16.1.0 `-Werror`) and Linux ELF64 (`gcc -Wall -Wextra -Werror` normal + `-fsanitize=address,undefined`) with `0` warnings!**

| Gate | Falsification Property Tested | Win64 PE (`v1.5.0`) | Linux ELF64 + ASan/UBSan |
|---|---|---|---|
| `Gate T01` | CPUID silicon flags, live RWX page exec, NIST SHA-256 `"abc"` vector | PASS | PASS |
| `Gate T02` | `VGF2P8AFFINEQB` vs scalar (500 matrices × 256 bytes) + exact $M^{-1}$ inverse | PASS | PASS |
| `Gate T03` | Exact 30 irreducible deg-8 polys + `PCLMULQDQ` vs scalar (1,000 pairs) | PASS | PASS |
| `Gate T04` | Native x86-64 JIT vs Scalar Oracle bit-exactness (32 opcodes, 5,000 genomes, rank-8 & singular, smooth `NERFING`) | PASS | PASS |
| `Gate T05` | 1,000,000 fuzz-mutated genomes live on RWX silicon + 64B OOB canaries | PASS | PASS |
| `Gate T06` | Reversible gates + rank-8 GFNI erase `0` bits; singular GFNI erases `64`; `NERF=100` true NOP; `AND_MASK` erases `32` | PASS | PASS |
| `Gate T07` | Closed-box & open-flux conservation (`<1e-12`), per-cell `membrane_pot_uq` & `detritus_grid` sync + **Mutants 1 & 2 caught** | PASS | PASS |
| `Gate T08` | Domain-isolated `Philox4x32-10` + `T39`/`T40` checks + `traits112[75]` + dead-world `OVI=0.0` + **Mutant 6 caught** | PASS | PASS |
| `Gate T09` | Full-world 60-epoch dual-kernel SHA-256 parity (`NATIVE_JIT == SCALAR_REF`) | PASS | PASS |
| `Gate T10` | Zero-Observer-Effect (`P9`) + **Planted Mutant 5 (1-bit observer write) caught** | PASS | PASS |
| `Gate T11` | `.apbin` v2 snapshot save (JIT), cross-kernel resume (Scalar) 0-bit diff, and tampered 1-byte `.apbin` rejected (`NULL`) | PASS | PASS |
| `Gate T12` | Zero post-warmup heap allocations (`HW02`, wired `g_ap_alloc_counter` + live negative-control guard trip test) | PASS | PASS |
| `Gate T13` | Spontaneous zero-seed abiogenesis phase transition + live `execs_per_second` | PASS | PASS |
| `Gate T14` | Anti-gaming Gates 1 & 2 + **Mutants 3a (intron bloat), 3b (dead scratch ops), & 4 (white noise) caught** | PASS | PASS |
| `Gate T15` | Planarian 50% Strand-$\alpha$ excision regeneration (`>=99%` ECC vs `<15%` unprotected control) | PASS | PASS |
| `Gate T16` | Weismann barrier shattering (open `>10` vs lobotomized `==0`) + `T41`/`T42` checks | PASS | PASS |
| `Gate T17` | Adversarial constraint-backlash escape signatures (all 4 vectors: Kill-switch, Checkpoint Spores, Intron Vaults, Side-channel) | PASS | PASS |
| `Gate T18` | Asmodensius orthogonal 112-trait projection (incl. holobiont root `[68]`) + Galois sex & mate refusals | PASS | PASS |
| `Gate T19` | **B07 Regression (CI `#45904159100`):** `metrology_excision_regen_cell` 64B stack alignment & zero-mutation under ASan FakeStack across 6 excision levels × 8 stack depths | PASS | PASS |

---

## 3. Four-Way Cross-OS / Cross-ABI / Cross-Kernel Bit-Exact SHA-256 Proof

Benchmark (`32×32 = 1024 cells`, `200 epochs`, `173,484` full organism executions, `CacheHit = 27.35%` / `126,031` compiles, seed `5055`; [`results/02_bench_win64.txt`](results/02_bench_win64.txt) & [`results/02_bench_linux.txt`](results/02_bench_linux.txt)):

| Platform / Compiler / ABI | Kernel | Measured Throughput | Canonical State SHA-256 |
|---|---|---|---|
| Windows 11 PE64 (`clang 23.1.0` ThinLTO) | Native RWX JIT | `0.236 M execs/s` (`1545.0 cyc/exec`, `1.04×`; unloaded `0.339 M/s`, `1.15×`) | `4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612` |
| Windows 11 PE64 (`clang 23.1.0` ThinLTO) | Scalar Reference | `0.226 M execs/s` (unloaded `0.294 M/s`) | `4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612` |
| Linux ELF64 (`gcc`, SysV AMD64, WSL2) | Native RWX JIT | `0.186 M execs/s` (`1476.4 cyc/exec`, `1.30×`) | `4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612` |
| Linux ELF64 (`gcc`, SysV AMD64, WSL2) | Scalar Reference | `0.144 M execs/s` | `4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612` |

---

## 4. Key Empirical Results (`results/`)

- **Zero-Seed Abiogenesis ([`results/03_abiogenesis_zero_seed.txt`](results/03_abiogenesis_zero_seed.txt)):** Pure random bytes (`t=0: alive=0, OVI=0.00`) → `t=30: alive=981, rep=688, OVI=86.56 (S5)` → `t=300: alive=1023, rep=1018, lin=286, ribo=188 (inv=189), irred=469, holo=842 (max=803), OVI=91.26 (S6), A=22.0, L_eff=27.9, R50=0.53, drift=0.00e+00, 2ndV=0, sha=28ecb9064a00`.
- **Track B Feral vs. Track A Constitutional ([`results/04_feral_vs_constitutional.txt`](results/04_feral_vs_constitutional.txt)):** Feral `OVI=87.55 (S5), BedauPackard_Anew=62.266, CancerDefections=25,151, drift=1.18e-16, sha=5096fa0a9187` vs. Constitutional `OVI=89.79 (S5), BedauPackard_Anew=70.258, CancerDefections=0, drift=1.18e-16, sha=b0fc34caaed7`.
- **13-Vector Castration Ablation ([`results/07_castration_ablation_13vec.csv`](results/07_castration_ablation_13vec.csv)):** Strictly monotone Iron Law curve (`GLOBAL_FERAL_0 = 91.09` → `GLOBAL_MILD_25 = 71.97` → `GLOBAL_ESCAPE_ZONE_50 = 54.31` → `GLOBAL_CASTRATED_100 = 0.00`); all 13/13 individual restraints at `100%` score strictly `< 91.09`; all 30 runs maintain `RelDrift <= 2.02e-16` and `0` observed heat-sink decreases.
- **Vivisection, 90% Mass Extinction & Cross-Substrate Transplant ([`results/05_vivisection_shock_transplant.txt`](results/05_vivisection_shock_transplant.txt)):** Non-ECC apex Slot #214 `R50=9.38%` vs. top evolved ECC Slot #788 `R25=R50=R75=Live50%=100.00%` (`PopMeanR50=43.4%`); `100.0%` transplant survival (32 organisms); `90%` extinction (`916` killed, `0 uq` residual) rebounds to `1,021 / 1,024` (`OVI=82.80 (S5)`).
- **Locked Paired Statistics (`N=24`, Seeds `5000..5023`, [`results/06_paired_stats_n24.txt`](results/06_paired_stats_n24.txt)):**

| Hypothesis (`N = 24` Locked Pairs) | Mean A | Mean B | Mean Diff | Pair Wins | $\delta_{\text{pair}}$ | $p_{\text{signflip}}$ | $p_{\text{Holm}}$ |
|---|---|---|---|---|---|---|---|
| **OVI: Feral vs Castrated** | `87.998` | `22.686` | `+65.312` | `24 / 24` | `+1.000` | `0.00010` | `0.00030` |
| **Weismann Retro: Open vs Lobotomized** | `9920.042` | `95.750` | `+9824.292` | `24 / 24` | `+1.000` | `0.00010` | `0.00030` |
| **Distinct Ribosomes: Feral vs Delimited** | `132.958` | `2.625` | `+130.333` | `24 / 24` | `+1.000` | `0.00010` | `0.00030` |

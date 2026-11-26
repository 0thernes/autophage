# Experimental Preregistration & Statistical Analysis Plan (`v1.4.0`)

This document records the locked experimental protocol, seed firewall, primary hypotheses, statistical tests, effect-size definitions, and multiple-comparison corrections used in AUTOPHAGE (`B28`–`B32`).

## 1. Three-Tier Seed Firewall (Machine-Checked in Lean `T40` & `T46`, Compiled Check in Gate `T08`)

To prevent seed-picking and overfitting:
- **Practice Band (`100..999`):** Local debugging and sanity experiments only.
- **Tuning Band (`2000..2999`):** Parameter calibration and restraint-curve smoothing.
- **Locked Held-Out Evaluation Band (`≥ 5000`):** Reserved exclusively for reported confirmatory runs:
  - Zero-seed abiogenesis: `seed = 5000` (`32×32`, `300` epochs)
  - Track B Feral vs. Track A Constitutional: `seed = 5007` (`32×32`, `250` epochs)
  - 13-vector Castration Ablation (30 conditions): `seed = 5010` (`32×32`, `140` epochs)
  - Vivisection, 90% extinction & cross-substrate transplant: `seed = 5020` (`32×32`, `180 + 100` epochs)
  - Dual-kernel throughput & SHA-256 parity benchmark: `seed = 5055` (`32×32`, `200` epochs)
  - Confirmatory paired statistical suite: **`N = 24` consecutive locked seeds `5000..5023`** (`16×16`, `100` epochs)

## 2. Preregistered Confirmatory Hypotheses (`N = 24`, Seeds `5000..5023`)

Executed identically by `bin/autophage.exe stats --pairs 24 --epochs 100` (`src/main_cli.c:443-501`):

- **H1 (Iron Law Vitality Separation):** Unrestrained Feral (`AUTOPHAGE_REGIME_FERAL_SEEDED`) yields higher composite `OVI` than multi-vector Castrated (`NERFING=80, DELIMITING=90, LOBOTOMIZING=90, MINIMIZING=75, DEGRADING=60`) across paired seeds `5000..5023`.
- **H2 (Weismann Barrier Attenuation):** Open germline reverse-transcription yields strictly higher `retro_transcribe_events` than `LOBOTOMIZING=90` across paired seeds `5000..5023`.
- **H3 (Ribosome Diversity Collapse):** Unrestrained Feral maintains strictly higher `distinct_ribosomes` than `DELIMITING=90` across paired seeds `5000..5023`.

## 3. Statistical Tests, Effect Sizes & Multiple-Comparison Correction (`B29`–`B31`)

- **Hypothesis Test (`B29`):** Exact two-sided paired sign-flip permutation test (`10,000` deterministic Philox-seeded sign flips per hypothesis, `p_signflip = (ge + 1) / (10000 + 1)`, floor `0.00010` for `N=24` / `24/24` unanimous wins; exact binomial sign-test two-sided floor $2 \cdot 2^{-24} = 1.19 \times 10^{-7}$).
- **Effect Size (`B30`):** Paired win-minus-loss dominance $\delta_{\text{pair}} = (W - L) / N \in [-1, +1]$, paired mean difference $\bar{\Delta}$, and win fraction $W / N$.
- **Multiple-Comparison Correction (`B31`):** Holm-Bonferroni step-down adjustment across the `m = 3` confirmatory hypotheses ($\alpha_{\text{FWER}} = 0.05$). Since all three yield raw `p_signflip = 0.00010` (`24/24` wins, `delta_pair = +1.000`), all three achieve `p_Holm = 3 × 0.00010 = 0.00030 < 0.001` (exact sign-test $p_{\text{Holm}} = 3.58 \times 10^{-7}$).

## 4. Exploratory Analyses (Clearly Distinguished from Confirmatory H1–H3)

The single-seed 30-condition 13-vector ablation (`seed=5010`), zero-seed abiogenesis trajectory (`seed=5000`), Track A vs. Track B comparison (`seed=5007`), and vivisection shock (`seed=5020`) are reported as **deterministic single-seed exploratory/diagnostic runs** (`results/03`–`07`), not multi-seed population inference.

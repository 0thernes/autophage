module

public import AutophageProofs.Spec

/-!
# AUTOPHAGE v1.5.0 — Adversarial False-Variant Refutations & Negative Boundaries (`AdversarialTests.lean`)

Eight concrete machine-checked refutations (`AT01`–`AT08`, `¬P` theorems) demonstrating that
dropping hypotheses or overstating claims in `AutophageProofs.Spec` produces provably false
statements inside Pure Lean 4.35.0 (zero Mathlib, zero `sorry`, zero `native_decide`, zero
`bv_decide`, `autoImplicit = false`, axioms strictly `⊆ {propext, Classical.choice, Quot.sound}`).

1. `AT01_refute_toffoli_popcnt_conserved` — Toffoli does NOT preserve `popcnt64` (witness `(0, 1, 1)`).
2. `AT02_refute_singular_ribosome_invertible` — Rank-7 singular `F_2^(8x8)` matrix has no inverse map.
3. `AT03_refute_reducible_poly8_divisor_free` — Reducible `x^8 + 1` (`0x01`) is divisible by `x + 1` (`0x03`) and fails Cauchy decoding.
4. `AT04_refute_unguarded_ledger_transfer` — Uncompensated debit breaks conservation; negative radiation breaks sink monotonicity.
5. `AT05_refute_and_mask_zero_erasure` — Nonzero `AND` bit erasure increments the `popcnt64` counter (exact witness: `32` bits).
6. `AT06_refute_wrong_mask_pext_pdep` — Mismatched mask breaks `pdep64 (pext64 src m1) m2` and `2 ^ popcnt64 m2` bound.
7. `AT07_refute_unconstrained_philox_and_seed_bands` — Dropping `dom1 ≠ dom2` breaks Philox disjointness; seed-band edges are sharp.
8. `AT08_refute_germline_write_isolated` — Unguarded germline XOR mutates germline (soma-only hypothesis in `T42` is strictly needed).
-/

namespace Autophage

/-- AT01: Explicit counterexample refuting `popcnt64` conservation for Toffoli (`T17` is an
    involution only; only Fredkin `T20a` conserves Hamming weight).
    Witness `(d, s, t) = (0, 1, 1)` maps total popcount `0 + 1 + 1 = 2` to `1 + 1 + 1 = 3`. -/
public theorem AT01_refute_toffoli_popcnt_conserved :
    ¬ (∀ d s t : BitVec 64,
        let (d1, s1, t1) := toffoli64 d s t
        popcnt64 d1 + popcnt64 s1 + popcnt64 t1 = popcnt64 d + popcnt64 s + popcnt64 t) := by
  intro hAll
  have hw := hAll 0#64 1#64 1#64
  have hne :
      (let (d1, s1, t1) := toffoli64 0#64 1#64 1#64
       popcnt64 d1 + popcnt64 s1 + popcnt64 t1 ≠ popcnt64 0#64 + popcnt64 1#64 + popcnt64 1#64) := by
    decide
  exact hne hw

/-- AT02: Explicit refutation of invertibility for a singular (`rank = 7 < 8`) `F_2^(8x8)` ribosome
    matrix (`row 0 = 0x00`, rows `1..7` = identity): distinct codons `0x00` and `0x01` collide to
    `0x00`, so NO function `Minv : BitVec 8 → BitVec 8` can left-invert `gfniApply singMat · 0`. -/
public theorem AT02_refute_singular_ribosome_invertible :
    let singMat : BitVec 64 := 0x0002040810204080#64
    ¬ (∃ Minv : BitVec 8 → BitVec 8, ∀ x : BitVec 8, Minv (gfniApply singMat x 0#8) = x) := by
  dsimp only
  rintro ⟨Minv, hInv⟩
  have hCol : gfniApply 0x0002040810204080#64 0x00#8 0#8 =
              gfniApply 0x0002040810204080#64 0x01#8 0#8 := by decide
  have h0 := hInv 0x00#8
  have h1 := hInv 0x01#8
  rw [hCol, h1] at h0
  exact (by decide : (0x01#8 : BitVec 8) ≠ 0x00#8) h0

/-- AT03: Explicit negative boundary for reducible degree-8 polynomials: `0x01#8` (`x^8 + 1`) fails
    `isIrreduciblePoly8Full`, has an explicit degree-1 divisor `0x0003#16` (`x + 1`), and fails
    2x2 Cauchy MDS recovery on `(0x01, 0x00)`. -/
public theorem AT03_refute_reducible_poly8_divisor_free :
    ¬ (∀ p : BitVec 8, isIrreduciblePoly8Full p = true) ∧
    polyDividesDeg (0x0100#16 ||| (0x01#8).zeroExtend 16) 0x0003#16 1 = true ∧
    cauchyRecLo 0x01#8 0x01#8 0x00#8 ≠ 0x01#8 := by
  refine ⟨?_, by decide, ?_⟩
  · intro hAll
    have h01 := hAll 0x01#8
    have hFalse : isIrreduciblePoly8Full 0x01#8 = false := by decide
    rw [hFalse] at h01
    exact Bool.noConfusion h01
  · unfold cauchyRecLo
    have hInv : cauchyDetInv 0x01#8 = 0#8 := rfl
    rw [hInv]
    have hZero : ∀ b : BitVec 8, gf2MulMod8 0#8 b 0x01#8 = 0#8 := by
      intro b
      unfold gf2MulMod8 clmul8 polyReduceMod8 polyReduceStep
      simp
    rw [hZero]
    decide

/-- AT04: Explicit refutation of unguarded ledger updates:
    (1) An uncompensated single-pool detritus debit breaks `residual = 0` (`residual = -1`).
    (2) An unguarded negative radiation `.heatToSink (-1)` strictly decreases `e_heat_snk`,
        showing `TransferOp.isNonNegHeatRadiation` is strictly necessary for `T30`. -/
public theorem AT04_refute_unguarded_ledger_transfer :
    ¬ (∀ (s : LedgerState) (e_init amt : Int),
        s.residual e_init = 0 →
        ({ s with e_detritus := s.e_detritus - amt } : LedgerState).residual e_init = 0) ∧
    ¬ (∀ (s : LedgerState) (op : TransferOp),
        s.e_heat_snk ≤ (applyTransfer s op).e_heat_snk) := by
  constructor
  · intro hAll
    let s0 : LedgerState := ⟨0, 0, 100, 0, 0, 0, 0⟩
    have hRes : s0.residual 100 = 0 := by simp [s0, LedgerState.residual]
    have hBad := hAll s0 100 1 hRes
    simp [s0, LedgerState.residual] at hBad
  · intro hAll
    let s0 : LedgerState := ⟨0, 0, 0, 10, 50, 0, 0⟩
    have hBad := hAll s0 (.heatToSink (-1))
    simp [s0, applyTransfer] at hBad

/-- AT05: Explicit refutation of zero bit-erasure on irreversible `AND` masking:
    masking `0xFFFFFFFFFFFFFFFF` with `0x00000000FFFFFFFF` strictly erases `32` bits
    (`popcnt64 = 32 > 0`), matching C11 Gate `T06`. -/
public theorem AT05_refute_and_mask_zero_erasure :
    ¬ (∀ old mask : BitVec 64, popcnt64 (old ^^^ (old &&& mask)) = 0) ∧
    popcnt64 (0xFFFFFFFFFFFFFFFF#64 ^^^ (0xFFFFFFFFFFFFFFFF#64 &&& 0x00000000FFFFFFFF#64)) = 32 := by
  have h32 :
      popcnt64 (0xFFFFFFFFFFFFFFFF#64 ^^^ (0xFFFFFFFFFFFFFFFF#64 &&& 0x00000000FFFFFFFF#64)) = 32 := by
    decide
  refine ⟨?_, h32⟩
  intro hAll
  have hw := hAll 0xFFFFFFFFFFFFFFFF#64 0x00000000FFFFFFFF#64
  rw [h32] at hw
  omega

/-- AT06: Explicit refutation of wrong-mask `PEXT`/`PDEP` round-trip and tight-bound assumptions:
    with `src = 0xFF`, `m1 = 0x0F`, and wrong mask `m2 = 0x03`, `pdep64 (pext64 src m1) m2 ≠ (src &&& m1)`
    and `(pext64 src m1).toNat = 15 ≥ 2 ^ popcnt64 m2 = 4`. -/
public theorem AT06_refute_wrong_mask_pext_pdep :
    ¬ (∀ src m1 m2 : BitVec 64, pdep64 (pext64 src m1) m2 = (src &&& m1)) ∧
    ¬ (∀ src m1 m2 : BitVec 64, (pext64 src m1).toNat < 2 ^ popcnt64 m2) := by
  constructor
  · intro hAll
    have hw := hAll 0xFF#64 0x0F#64 0x03#64
    have hne : pdep64 (pext64 0xFF#64 0x0F#64) 0x03#64 ≠ (0xFF#64 &&& 0x0F#64) := by decide
    exact hne hw
  · intro hAll
    have hw := hAll 0xFF#64 0x0F#64 0x03#64
    have hge : ¬ ((pext64 0xFF#64 0x0F#64).toNat < 2 ^ popcnt64 0x03#64) := by decide
    exact hge hw

/-- AT07: Explicit refutation of unconstrained Philox counter disjointness and sharp seed-band
    membership: dropping `dom1 ≠ dom2` fails on `dom1 = dom2 = 0`, `500` is NOT a locked seed,
    and `2500` is NOT a practice seed. -/
public theorem AT07_refute_unconstrained_philox_and_seed_bands :
    ¬ (∀ dom1 dom2 : Nat,
        PhiloxCounterKey.mk 1 5000 dom1 0 0 0 0 0 ≠
        PhiloxCounterKey.mk 1 5000 dom2 0 0 0 0 0) ∧
    ¬ isLockedSeed 500 ∧
    ¬ isPracticeSeed 2500 := by
  refine ⟨?_, ?_, ?_⟩
  · intro hAll
    exact hAll 0 0 rfl
  · unfold isLockedSeed; omega
  · unfold isPracticeSeed; omega

/-- AT08: Explicit refutation of germline isolation under unguarded germline writes:
    XORing `germline` by `1#64` strictly alters `germline`, proving `T42` depends on the
    soma-only restriction (`perturbSomaOnly`). -/
public theorem AT08_refute_germline_write_isolated :
    ¬ (∀ (st : WeismannState) (mask : BitVec 64),
        ({ st with germline := st.germline ^^^ mask } : WeismannState).germline = st.germline) := by
  intro hAll
  let st0 : WeismannState := ⟨0#64, 0#64⟩
  have hw := hAll st0 1#64
  have hne : (0#64 ^^^ 1#64 : BitVec 64) ≠ 0#64 := by decide
  exact hne hw

end Autophage

module

public import AutophageProofs.Spec

/-!
# AUTOPHAGE v1.5.0 — Pure Kernel Formal Lean 4.35.0 Verification Suite (48 Core/Witness Theorems + 8 AdversarialTests = 56 Total, 0 `sorry`, 0 External Axioms)

Machine-checked proofs verified strictly inside the pure Lean 4.35.0 kernel
(no Mathlib, no `Std.Tactic.BVDecide`, no `bv_decide`, no `native_decide`, no custom axioms;
every `#print axioms` audit lies strictly within `{propext, Classical.choice, Quot.sound}`):
  Part I   : F_2^(8x8) Ribosome Algebra (`VGF2P8AFFINEQB`), Universal XOR-Linearity,
             Weismann Inverse Exactness, and Universal Elementary Row Ops (Fin 8) (T01–T07)
  Part II  : GF(2)[x] Carry-Less Polynomial Ring (`PCLMULQDQ`), Full Trial-Divisor Match
             to C11 `GF2_IRRED_POLY8[30]` (30 Irreducible / 226 Reducible), Dobzhansky-Muller
             Zero Divisors, and Universal Planarian Cauchy 2x2 MDS Recovery Across ALL 30 Fields (T08–T16a)
  Part III : Symplectic Reversible Gates (`TOFFOLI`, `FREDKIN`), Exact Fredkin `popcnt64` Conservation
             & Bit-Erasure Accounting (T17–T22; T21 is an elementary XOR helper)
  Part IV  : 2-Adic Ultrametric Centromere Geometry & Exact `tzcnt32` Strong Triangle Inequality
             (T23–T25a; T23, T24 are elementary XOR helpers)
  Part V   : JIT Mask Bounds, Gas Decrement & Multi-Step Conserved Ledger / Heat-Sink Monotonicity
             (T26–T30; T28, T29 are elementary arithmetic helpers; T30 proves multi-step `LedgerState`
             heat-sink monotonicity + conservation + zero-alive OVI gate)
  Part VI  : Domain-Isolated Philox Counter Keys, 3-Tier Seed Firewall, OEE Archive Monotonicity,
             & Weismann Soma Mutation Isolation (T39–T42)
  Part VII : Non-Vacuity Witnesses & Sharp Negative Boundaries (T43–T48)
  (Companion module `AdversarialTests.lean` adds 8 explicit false-variant refutations AT01–AT08.)
-/

namespace Autophage

/-! ## Pure Boolean & BitVec Algebraic Helpers -/

private theorem bool_xor_swap4 (a b c d : Bool) :
    ((a ^^ b) ^^ (c ^^ d)) = ((a ^^ c) ^^ (b ^^ d)) := by
  cases a <;> cases b <;> cases c <;> cases d <;> rfl

private theorem bool_xor8_distrib
    (a0 a1 a2 a3 a4 a5 a6 a7 b0 b1 b2 b3 b4 b5 b6 b7 : Bool) :
    ((a0 ^^ b0) ^^ (a1 ^^ b1) ^^ (a2 ^^ b2) ^^ (a3 ^^ b3) ^^
     (a4 ^^ b4) ^^ (a5 ^^ b5) ^^ (a6 ^^ b6) ^^ (a7 ^^ b7)) =
    ((a0 ^^ a1 ^^ a2 ^^ a3 ^^ a4 ^^ a5 ^^ a6 ^^ a7) ^^
     (b0 ^^ b1 ^^ b2 ^^ b3 ^^ b4 ^^ b5 ^^ b6 ^^ b7)) := by
  have h1 : ((a0 ^^ b0) ^^ (a1 ^^ b1)) = ((a0 ^^ a1) ^^ (b0 ^^ b1)) := bool_xor_swap4 _ _ _ _
  rw [h1]
  have h2 : (((a0 ^^ a1) ^^ (b0 ^^ b1)) ^^ (a2 ^^ b2)) = ((a0 ^^ a1 ^^ a2) ^^ (b0 ^^ b1 ^^ b2)) :=
    bool_xor_swap4 _ _ _ _
  rw [h2]
  have h3 : (((a0 ^^ a1 ^^ a2) ^^ (b0 ^^ b1 ^^ b2)) ^^ (a3 ^^ b3)) =
            ((a0 ^^ a1 ^^ a2 ^^ a3) ^^ (b0 ^^ b1 ^^ b2 ^^ b3)) :=
    bool_xor_swap4 _ _ _ _
  rw [h3]
  have h4 : (((a0 ^^ a1 ^^ a2 ^^ a3) ^^ (b0 ^^ b1 ^^ b2 ^^ b3)) ^^ (a4 ^^ b4)) =
            ((a0 ^^ a1 ^^ a2 ^^ a3 ^^ a4) ^^ (b0 ^^ b1 ^^ b2 ^^ b3 ^^ b4)) :=
    bool_xor_swap4 _ _ _ _
  rw [h4]
  have h5 : (((a0 ^^ a1 ^^ a2 ^^ a3 ^^ a4) ^^ (b0 ^^ b1 ^^ b2 ^^ b3 ^^ b4)) ^^ (a5 ^^ b5)) =
            ((a0 ^^ a1 ^^ a2 ^^ a3 ^^ a4 ^^ a5) ^^ (b0 ^^ b1 ^^ b2 ^^ b3 ^^ b4 ^^ b5)) :=
    bool_xor_swap4 _ _ _ _
  rw [h5]
  have h6 : (((a0 ^^ a1 ^^ a2 ^^ a3 ^^ a4 ^^ a5) ^^ (b0 ^^ b1 ^^ b2 ^^ b3 ^^ b4 ^^ b5)) ^^ (a6 ^^ b6)) =
            ((a0 ^^ a1 ^^ a2 ^^ a3 ^^ a4 ^^ a5 ^^ a6) ^^ (b0 ^^ b1 ^^ b2 ^^ b3 ^^ b4 ^^ b5 ^^ b6)) :=
    bool_xor_swap4 _ _ _ _
  rw [h6]
  exact bool_xor_swap4 _ _ _ _

private theorem bool_xor_and_right (a b c : Bool) :
    ((a ^^ b) && c) = ((a && c) ^^ (b && c)) := by
  cases a <;> cases b <;> cases c <;> rfl

private theorem bv_and_xor_distrib_left {w : Nat} (a b c : BitVec w) :
    a &&& (b ^^^ c) = (a &&& b) ^^^ (a &&& c) := by
  apply BitVec.eq_of_getLsbD_eq
  intro i _
  simp only [BitVec.getLsbD_and, BitVec.getLsbD_xor]
  cases a.getLsbD i <;> cases b.getLsbD i <;> cases c.getLsbD i <;> rfl

private theorem bv_xor_swap4 {w : Nat} (a b c d : BitVec w) :
    (a ^^^ b) ^^^ (c ^^^ d) = (a ^^^ c) ^^^ (b ^^^ d) := by
  rw [BitVec.xor_assoc, ← BitVec.xor_assoc b c d, BitVec.xor_comm b c,
      BitVec.xor_assoc c b d, ← BitVec.xor_assoc a c (b ^^^ d)]

/-! ## Part I: F_2^(8x8) Ribosome Algebra (Intel `VGF2P8AFFINEQB` Exact Bit Order) -/

private theorem parity8_eq_xor_bits (v : BitVec 8) :
    parity8 v =
      (v.getLsbD 0 ^^ v.getLsbD 1 ^^ v.getLsbD 2 ^^ v.getLsbD 3 ^^
       v.getLsbD 4 ^^ v.getLsbD 5 ^^ v.getLsbD 6 ^^ v.getLsbD 7) := by
  have h : ∀ w : BitVec 8, parity8 w =
      (w.getLsbD 0 ^^ w.getLsbD 1 ^^ w.getLsbD 2 ^^ w.getLsbD 3 ^^
       w.getLsbD 4 ^^ w.getLsbD 5 ^^ w.getLsbD 6 ^^ w.getLsbD 7) := by
    decide
  exact h v

private theorem gfniApply_getLsbD (mat : BitVec 64) (x bias : BitVec 8) (i : Nat) (hi : i < 8) :
    (gfniApply mat x bias).getLsbD i =
      (parity8 (gfniRow mat i &&& x) ^^ bias.getLsbD i) := by
  unfold gfniApply
  have hi_cases : i = 0 ∨ i = 1 ∨ i = 2 ∨ i = 3 ∨ i = 4 ∨ i = 5 ∨ i = 6 ∨ i = 7 := by omega
  rcases hi_cases with rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl <;>
    simp only [BitVec.getLsbD_xor, BitVec.getLsbD_or] <;>
    (cases parity8 (gfniRow mat 0 &&& x) <;>
     cases parity8 (gfniRow mat 1 &&& x) <;>
     cases parity8 (gfniRow mat 2 &&& x) <;>
     cases parity8 (gfniRow mat 3 &&& x) <;>
     cases parity8 (gfniRow mat 4 &&& x) <;>
     cases parity8 (gfniRow mat 5 &&& x) <;>
     cases parity8 (gfniRow mat 6 &&& x) <;>
     cases parity8 (gfniRow mat 7 &&& x) <;>
     simp)

/-- T01: Intel `GF2P8AFFINEQB` identity matrix `0x0102040810204080` fixes every byte x. -/
public theorem T01_gfni_identity_fixes_all (x : BitVec 8) :
    gfniApply gfniIdentity x 0#8 = x := by
  have h : ∀ w : BitVec 8, gfniApply gfniIdentity w 0#8 = w := by decide
  exact h x

/-- T02: Affine bias XOR is an exact involution. -/
public theorem T02_gfni_bias_involution (mat : BitVec 64) (x bias : BitVec 8) :
    gfniApply mat x bias ^^^ bias = gfniApply mat x 0#8 := by
  unfold gfniApply
  simp [BitVec.xor_assoc, BitVec.xor_self]

/-- T03: Parity over F_2 is strictly linear under XOR. -/
public theorem T03_parity8_xor_linear (a b : BitVec 8) :
    parity8 (a ^^^ b) = xor (parity8 a) (parity8 b) := by
  rw [parity8_eq_xor_bits, parity8_eq_xor_bits, parity8_eq_xor_bits]
  simp only [BitVec.getLsbD_xor]
  exact bool_xor8_distrib _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

/-- T03a: Universal F_2^(8x8) Linearity — for EVERY 64-bit ribosome matrix `mat` and EVERY `a, b`,
    linear `gfniApply mat · 0` distributes strictly over XOR! -/
public theorem T03a_gfni_xor_linear (mat : BitVec 64) (a b : BitVec 8) :
    gfniApply mat (a ^^^ b) 0#8 = gfniApply mat a 0#8 ^^^ gfniApply mat b 0#8 := by
  apply BitVec.eq_of_getLsbD_eq
  intro i hi
  rw [gfniApply_getLsbD mat (a ^^^ b) 0#8 i hi]
  rw [BitVec.getLsbD_xor]
  rw [gfniApply_getLsbD mat a 0#8 i hi]
  rw [gfniApply_getLsbD mat b 0#8 i hi]
  rw [bv_and_xor_distrib_left, T03_parity8_xor_linear]
  simp

/-- T04: Linear ribosome translation (bias = 0) fixes the zero codon. -/
public theorem T04_gfni_zero_fixed (mat : BitVec 64) :
    gfniApply mat 0#8 0#8 = 0#8 := by
  apply BitVec.eq_of_getLsbD_eq
  intro i hi
  rw [gfniApply_getLsbD mat 0#8 0#8 i hi]
  have hp0 : parity8 0#8 = false := by decide
  simp [BitVec.and_zero, hp0]

/-- T05a: General Linear Composition Theorem — for ANY ribosome matrix pair (M, Minv)
    whose linear maps invert each other (`Minv(M(x)) = x`) and distribute over XOR,
    affine reverse-transcription with `c_inv = Minv(bias)` recovers `x` for ALL codons & biases! -/
public theorem T05a_weismann_general_affine_inverse
    (M Minv : BitVec 8 → BitVec 8)
    (hLin : ∀ a b, Minv (a ^^^ b) = Minv a ^^^ Minv b)
    (hInv : ∀ x, Minv (M x) = x)
    (x bias : BitVec 8) :
    Minv (M x ^^^ bias) ^^^ Minv bias = x := by
  rw [hLin, hInv, BitVec.xor_assoc, BitVec.xor_self, BitVec.xor_zero]

private theorem gfniApply_split_bias (mat : BitVec 64) (x bias : BitVec 8) :
    gfniApply mat x bias = gfniApply mat x 0#8 ^^^ bias := by
  unfold gfniApply
  simp [BitVec.xor_zero]

private theorem asym_linear_inverse (x : BitVec 8) :
    gfniApply asymRibosomeInv (gfniApply asymRibosomeMat x 0#8) 0#8 = x := by
  have h : ∀ w : BitVec 8, gfniApply asymRibosomeInv (gfniApply asymRibosomeMat w 0#8) 0#8 = w := by
    decide
  exact h x

/-- T05: Exact Weismann Reverse-Transcription Round-Trip across ALL 256 codons x ALL 256 biases
    for an asymmetric non-involutive F_2 ribosome (M != M^-1). -/
public theorem T05_weismann_exact_roundtrip (x bias : BitVec 8) :
    let y := gfniApply asymRibosomeMat x bias
    let c_inv := gfniApply asymRibosomeInv bias 0#8
    gfniApply asymRibosomeInv y c_inv = x := by
  dsimp only
  rw [gfniApply_split_bias asymRibosomeMat x bias]
  rw [gfniApply_split_bias asymRibosomeInv]
  exact T05a_weismann_general_affine_inverse
    (fun v => gfniApply asymRibosomeMat v 0#8)
    (fun v => gfniApply asymRibosomeInv v 0#8)
    (T03a_gfni_xor_linear asymRibosomeInv)
    asym_linear_inverse
    x bias

private theorem testBit_of_lt_two_pow (n k : Nat) (h : n < 2 ^ k) : Nat.testBit n k = false := by
  simp only [Nat.testBit, Nat.shiftRight_eq_div_pow, Nat.div_eq_of_lt h]
  rfl

private theorem bv8_getLsbD_ge (v : BitVec 8) (m : Nat) (hm : 8 ≤ m) :
    v.getLsbD m = false := by
  rw [BitVec.getLsbD]
  exact testBit_of_lt_two_pow v.toNat m
    (Nat.lt_of_lt_of_le v.isLt (Nat.pow_le_pow_right (by omega) hm))

private theorem packRows_getLsbD (f : Fin 8 → BitVec 8) (k : Nat) (hk : k < 64) :
    (packRows f).getLsbD k = (f ⟨7 - k / 8, by omega⟩).getLsbD (k % 8) := by
  unfold packRows
  simp only [BitVec.getLsbD_or, BitVec.getLsbD_shiftLeft, BitVec.getLsbD_setWidth, hk,
             decide_true, Bool.true_and]
  have hbyte : k / 8 = 0 ∨ k / 8 = 1 ∨ k / 8 = 2 ∨ k / 8 = 3 ∨
               k / 8 = 4 ∨ k / 8 = 5 ∨ k / 8 = 6 ∨ k / 8 = 7 := by omega
  rcases hbyte with h | h | h | h | h | h | h | h
  · have hmod : k % 8 = k := by omega
    have hfin : (⟨7 - k / 8, by omega⟩ : Fin 8) = 7 := by apply Fin.ext; dsimp only; omega
    rw [hfin, hmod]
    simp [show k < 56 by omega, show k < 48 by omega, show k < 40 by omega,
          show k < 32 by omega, show k < 24 by omega, show k < 16 by omega, show k < 8 by omega]
  · have hmod : k % 8 = k - 8 := by omega
    have hfin : (⟨7 - k / 8, by omega⟩ : Fin 8) = 6 := by apply Fin.ext; dsimp only; omega
    rw [hfin, hmod]
    simp [show k < 56 by omega, show k < 48 by omega, show k < 40 by omega,
          show k < 32 by omega, show k < 24 by omega, show k < 16 by omega,
          show ¬(k < 8) by omega, show k - 8 < 64 by omega, bv8_getLsbD_ge (f 7) k (by omega)]
  · have hmod : k % 8 = k - 16 := by omega
    have hfin : (⟨7 - k / 8, by omega⟩ : Fin 8) = 5 := by apply Fin.ext; dsimp only; omega
    rw [hfin, hmod]
    simp [show k < 56 by omega, show k < 48 by omega, show k < 40 by omega,
          show k < 32 by omega, show k < 24 by omega, show ¬(k < 16) by omega,
          show ¬(k < 8) by omega, show k - 16 < 64 by omega,
          bv8_getLsbD_ge (f 6) (k - 8) (by omega), bv8_getLsbD_ge (f 7) k (by omega)]
  · have hmod : k % 8 = k - 24 := by omega
    have hfin : (⟨7 - k / 8, by omega⟩ : Fin 8) = 4 := by apply Fin.ext; dsimp only; omega
    rw [hfin, hmod]
    simp [show k < 56 by omega, show k < 48 by omega, show k < 40 by omega,
          show k < 32 by omega, show ¬(k < 24) by omega, show ¬(k < 16) by omega,
          show ¬(k < 8) by omega, show k - 24 < 64 by omega,
          bv8_getLsbD_ge (f 5) (k - 16) (by omega), bv8_getLsbD_ge (f 6) (k - 8) (by omega),
          bv8_getLsbD_ge (f 7) k (by omega)]
  · have hmod : k % 8 = k - 32 := by omega
    have hfin : (⟨7 - k / 8, by omega⟩ : Fin 8) = 3 := by apply Fin.ext; dsimp only; omega
    rw [hfin, hmod]
    simp [show k < 56 by omega, show k < 48 by omega, show k < 40 by omega,
          show ¬(k < 32) by omega, show ¬(k < 24) by omega, show ¬(k < 16) by omega,
          show ¬(k < 8) by omega, show k - 32 < 64 by omega,
          bv8_getLsbD_ge (f 4) (k - 24) (by omega), bv8_getLsbD_ge (f 5) (k - 16) (by omega),
          bv8_getLsbD_ge (f 6) (k - 8) (by omega), bv8_getLsbD_ge (f 7) k (by omega)]
  · have hmod : k % 8 = k - 40 := by omega
    have hfin : (⟨7 - k / 8, by omega⟩ : Fin 8) = 2 := by apply Fin.ext; dsimp only; omega
    rw [hfin, hmod]
    simp [show k < 56 by omega, show k < 48 by omega, show ¬(k < 40) by omega,
          show ¬(k < 32) by omega, show ¬(k < 24) by omega, show ¬(k < 16) by omega,
          show ¬(k < 8) by omega, show k - 40 < 64 by omega,
          bv8_getLsbD_ge (f 3) (k - 32) (by omega), bv8_getLsbD_ge (f 4) (k - 24) (by omega),
          bv8_getLsbD_ge (f 5) (k - 16) (by omega), bv8_getLsbD_ge (f 6) (k - 8) (by omega),
          bv8_getLsbD_ge (f 7) k (by omega)]
  · have hmod : k % 8 = k - 48 := by omega
    have hfin : (⟨7 - k / 8, by omega⟩ : Fin 8) = 1 := by apply Fin.ext; dsimp only; omega
    rw [hfin, hmod]
    simp [show k < 56 by omega, show ¬(k < 48) by omega, show ¬(k < 40) by omega,
          show ¬(k < 32) by omega, show ¬(k < 24) by omega, show ¬(k < 16) by omega,
          show ¬(k < 8) by omega, show k - 48 < 64 by omega,
          bv8_getLsbD_ge (f 2) (k - 40) (by omega), bv8_getLsbD_ge (f 3) (k - 32) (by omega),
          bv8_getLsbD_ge (f 4) (k - 24) (by omega), bv8_getLsbD_ge (f 5) (k - 16) (by omega),
          bv8_getLsbD_ge (f 6) (k - 8) (by omega), bv8_getLsbD_ge (f 7) k (by omega)]
  · have hmod : k % 8 = k - 56 := by omega
    have hfin : (⟨7 - k / 8, by omega⟩ : Fin 8) = 0 := by apply Fin.ext; dsimp only; omega
    rw [hfin, hmod]
    simp [show ¬(k < 56) by omega, show ¬(k < 48) by omega, show ¬(k < 40) by omega,
          show ¬(k < 32) by omega, show ¬(k < 24) by omega, show ¬(k < 16) by omega,
          show ¬(k < 8) by omega, show k - 56 < 64 by omega,
          bv8_getLsbD_ge (f 1) (k - 48) (by omega), bv8_getLsbD_ge (f 2) (k - 40) (by omega),
          bv8_getLsbD_ge (f 3) (k - 32) (by omega), bv8_getLsbD_ge (f 4) (k - 24) (by omega),
          bv8_getLsbD_ge (f 5) (k - 16) (by omega), bv8_getLsbD_ge (f 6) (k - 8) (by omega),
          bv8_getLsbD_ge (f 7) k (by omega)]

private theorem gfniRow_getLsbD (m : BitVec 64) (r : Fin 8) (b : Nat) (hb : b < 8) :
    (gfniRow m r.val).getLsbD b = m.getLsbD ((7 - r.val) * 8 + b) := by
  unfold gfniRow
  simp [BitVec.truncate, BitVec.getLsbD_ushiftRight, hb, Nat.add_comm]

private theorem pack_unpack (m : BitVec 64) : packRows (unpackRows m) = m := by
  apply BitVec.eq_of_getLsbD_eq
  intro k hk
  rw [packRows_getLsbD (unpackRows m) k hk]
  unfold unpackRows
  rw [gfniRow_getLsbD m ⟨7 - k / 8, by omega⟩ (k % 8) (by omega)]
  have hEq : (7 - (7 - k / 8)) * 8 + k % 8 = k := by omega
  rw [hEq]

private theorem unpack_pack (f : Fin 8 → BitVec 8) (r : Fin 8) :
    unpackRows (packRows f) r = f r := by
  apply BitVec.eq_of_getLsbD_eq
  intro b hb
  unfold unpackRows
  rw [gfniRow_getLsbD (packRows f) r b hb]
  rw [packRows_getLsbD f ((7 - r.val) * 8 + b) (by omega)]
  have h2 : ((7 - r.val) * 8 + b) % 8 = b := by omega
  have hfin : (⟨7 - ((7 - r.val) * 8 + b) / 8, by omega⟩ : Fin 8) = r := by
    apply Fin.ext
    dsimp only
    omega
  rw [hfin, h2]

/-- T06: Universal elementary ribosome row-swap is an exact involution for ALL `(i j : Fin 8)`
    and ALL 64-bit matrices `m` (preserves det(M) = 1). -/
public theorem T06_elem_row_swap_involution (i j : Fin 8) (m : BitVec 64) :
    swapRows i j (swapRows i j m) = m := by
  conv => rhs; rw [← pack_unpack m]
  unfold swapRows
  congr 1
  funext r
  simp only [unpack_pack]
  by_cases hri : r = i
  · subst hri
    by_cases hij : r = j
    · subst hij; simp
    · simp [Ne.symm hij]
  · by_cases hrj : r = j
    · subst hrj; simp [hri]
    · simp [hri, hrj]

/-- T07: Universal elementary ribosome row-shear is an exact involution for ALL distinct `(i j : Fin 8)`
    and ALL 64-bit matrices `m` (preserves det(M) = 1). -/
public theorem T07_elem_row_shear_involution (i j : Fin 8) (hij : i ≠ j) (m : BitVec 64) :
    shearRows i j (shearRows i j m) = m := by
  conv => rhs; rw [← pack_unpack m]
  unfold shearRows
  congr 1
  funext r
  simp only [unpack_pack]
  by_cases hri : r = i
  · subst hri
    simp [Ne.symm hij, BitVec.xor_assoc, BitVec.xor_self, BitVec.xor_zero]
  · simp [hri]

/-! ## Part II: GF(2)[x] Carry-Less Polynomial Ring (`PCLMULQDQ`), 30 Fields & ECC -/

private theorem clmul_term_bit (b : BitVec 8) (mask : BitVec 8) (j : Nat)
    (hm : mask = 1#8 <<< j) (hj : j < 8) :
    ((b &&& mask) != 0#8) = b.getLsbD j := by
  subst hm
  have h : ∀ w : BitVec 8, ((w &&& (1#8 <<< j)) != 0#8) = w.getLsbD j := by
    have hjc : j = 0 ∨ j = 1 ∨ j = 2 ∨ j = 3 ∨ j = 4 ∨ j = 5 ∨ j = 6 ∨ j = 7 := by omega
    rcases hjc with rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl <;> decide
  exact h b

private theorem clmul8_getLsbD_term (a : BitVec 8) (cond : Bool) (j k : Nat) (hk : k < 16) :
    (if cond then a.zeroExtend 16 <<< j else 0#16).getLsbD k =
      (cond && (decide (j ≤ k) && decide (k - j < 8) && a.getLsbD (k - j))) := by
  cases cond
  · simp
  · simp only [↓reduceIte, Bool.true_and, BitVec.getLsbD_shiftLeft, BitVec.zeroExtend,
               BitVec.getLsbD_setWidth, hk, decide_true]
    by_cases hjk : j ≤ k
    · by_cases h8 : k - j < 8
      · have hlt16 : k - j < 16 := by omega
        have hnlt : ¬(k < j) := by omega
        simp [hjk, h8, hlt16, hnlt]
      · have hge8 : 8 ≤ k - j := by omega
        have hnlt : ¬(k < j) := by omega
        simp [hjk, h8, hnlt, bv8_getLsbD_ge a (k - j) hge8]
    · have hlt : k < j := by omega
      simp [hjk, hlt]

private theorem clmul8_getLsbD (a b : BitVec 8) (k : Nat) (hk : k < 16) :
    (clmul8 a b).getLsbD k =
      ((b.getLsbD 0 && (decide (0 ≤ k) && decide (k - 0 < 8) && a.getLsbD (k - 0))) ^^
       (b.getLsbD 1 && (decide (1 ≤ k) && decide (k - 1 < 8) && a.getLsbD (k - 1))) ^^
       (b.getLsbD 2 && (decide (2 ≤ k) && decide (k - 2 < 8) && a.getLsbD (k - 2))) ^^
       (b.getLsbD 3 && (decide (3 ≤ k) && decide (k - 3 < 8) && a.getLsbD (k - 3))) ^^
       (b.getLsbD 4 && (decide (4 ≤ k) && decide (k - 4 < 8) && a.getLsbD (k - 4))) ^^
       (b.getLsbD 5 && (decide (5 ≤ k) && decide (k - 5 < 8) && a.getLsbD (k - 5))) ^^
       (b.getLsbD 6 && (decide (6 ≤ k) && decide (k - 6 < 8) && a.getLsbD (k - 6))) ^^
       (b.getLsbD 7 && (decide (7 ≤ k) && decide (k - 7 < 8) && a.getLsbD (k - 7)))) := by
  unfold clmul8
  simp only [BitVec.getLsbD_xor]
  have h0 : ((b &&& 0x01#8) != 0#8) = b.getLsbD 0 := clmul_term_bit b 0x01#8 0 rfl (by omega)
  have h1 : ((b &&& 0x02#8) != 0#8) = b.getLsbD 1 := clmul_term_bit b 0x02#8 1 rfl (by omega)
  have h2 : ((b &&& 0x04#8) != 0#8) = b.getLsbD 2 := clmul_term_bit b 0x04#8 2 rfl (by omega)
  have h3 : ((b &&& 0x08#8) != 0#8) = b.getLsbD 3 := clmul_term_bit b 0x08#8 3 rfl (by omega)
  have h4 : ((b &&& 0x10#8) != 0#8) = b.getLsbD 4 := clmul_term_bit b 0x10#8 4 rfl (by omega)
  have h5 : ((b &&& 0x20#8) != 0#8) = b.getLsbD 5 := clmul_term_bit b 0x20#8 5 rfl (by omega)
  have h6 : ((b &&& 0x40#8) != 0#8) = b.getLsbD 6 := clmul_term_bit b 0x40#8 6 rfl (by omega)
  have h7 : ((b &&& 0x80#8) != 0#8) = b.getLsbD 7 := clmul_term_bit b 0x80#8 7 rfl (by omega)
  have hs0 : (if (b &&& 0x01#8) != 0#8 then a.zeroExtend 16 else 0#16) =
             (if b.getLsbD 0 then a.zeroExtend 16 <<< 0 else 0#16) := by simp [h0]
  rw [hs0, h1, h2, h3, h4, h5, h6, h7]
  rw [clmul8_getLsbD_term a (b.getLsbD 0) 0 k hk,
      clmul8_getLsbD_term a (b.getLsbD 1) 1 k hk,
      clmul8_getLsbD_term a (b.getLsbD 2) 2 k hk,
      clmul8_getLsbD_term a (b.getLsbD 3) 3 k hk,
      clmul8_getLsbD_term a (b.getLsbD 4) 4 k hk,
      clmul8_getLsbD_term a (b.getLsbD 5) 5 k hk,
      clmul8_getLsbD_term a (b.getLsbD 6) 6 k hk,
      clmul8_getLsbD_term a (b.getLsbD 7) 7 k hk]

private theorem xor2_rev (x0 x1 : Bool) : (x0 ^^ x1) = (x1 ^^ x0) := by
  cases x0 <;> cases x1 <;> rfl
private theorem xor3_rev (x0 x1 x2 : Bool) : (x0 ^^ x1 ^^ x2) = (x2 ^^ x1 ^^ x0) := by
  cases x0 <;> cases x1 <;> cases x2 <;> rfl
private theorem xor4_rev (x0 x1 x2 x3 : Bool) :
    (x0 ^^ x1 ^^ x2 ^^ x3) = (x3 ^^ x2 ^^ x1 ^^ x0) := by
  cases x0 <;> cases x1 <;> cases x2 <;> cases x3 <;> rfl
private theorem xor5_rev (x0 x1 x2 x3 x4 : Bool) :
    (x0 ^^ x1 ^^ x2 ^^ x3 ^^ x4) = (x4 ^^ x3 ^^ x2 ^^ x1 ^^ x0) := by
  cases x0 <;> cases x1 <;> cases x2 <;> cases x3 <;> cases x4 <;> rfl
private theorem xor6_rev (x0 x1 x2 x3 x4 x5 : Bool) :
    (x0 ^^ x1 ^^ x2 ^^ x3 ^^ x4 ^^ x5) = (x5 ^^ x4 ^^ x3 ^^ x2 ^^ x1 ^^ x0) := by
  cases x0 <;> cases x1 <;> cases x2 <;> cases x3 <;> cases x4 <;> cases x5 <;> rfl
private theorem xor7_rev (x0 x1 x2 x3 x4 x5 x6 : Bool) :
    (x0 ^^ x1 ^^ x2 ^^ x3 ^^ x4 ^^ x5 ^^ x6) = (x6 ^^ x5 ^^ x4 ^^ x3 ^^ x2 ^^ x1 ^^ x0) := by
  cases x0 <;> cases x1 <;> cases x2 <;> cases x3 <;> cases x4 <;> cases x5 <;> cases x6 <;> rfl
private theorem xor8_rev (x0 x1 x2 x3 x4 x5 x6 x7 : Bool) :
    (x0 ^^ x1 ^^ x2 ^^ x3 ^^ x4 ^^ x5 ^^ x6 ^^ x7) =
    (x7 ^^ x6 ^^ x5 ^^ x4 ^^ x3 ^^ x2 ^^ x1 ^^ x0) := by
  cases x0 <;> cases x1 <;> cases x2 <;> cases x3 <;>
  cases x4 <;> cases x5 <;> cases x6 <;> cases x7 <;> rfl

/-- T08: Carry-less multiplication over GF(2)[x] is commutative. -/
public theorem T08_clmul8_comm (a b : BitVec 8) : clmul8 a b = clmul8 b a := by
  apply BitVec.eq_of_getLsbD_eq
  intro k hk
  rw [clmul8_getLsbD a b k hk, clmul8_getLsbD b a k hk]
  have hk_cases :
      k = 0 ∨ k = 1 ∨ k = 2 ∨ k = 3 ∨ k = 4 ∨ k = 5 ∨ k = 6 ∨ k = 7 ∨
      k = 8 ∨ k = 9 ∨ k = 10 ∨ k = 11 ∨ k = 12 ∨ k = 13 ∨ k = 14 ∨ k = 15 := by omega
  rcases hk_cases with
    rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl |
    rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl
  · simp [Bool.and_comm]
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.xor_false]
    rw [Bool.and_comm (b.getLsbD 0), Bool.and_comm (b.getLsbD 1)]; exact xor2_rev _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.xor_false]
    rw [Bool.and_comm (b.getLsbD 0), Bool.and_comm (b.getLsbD 1), Bool.and_comm (b.getLsbD 2)]
    exact xor3_rev _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.xor_false]
    rw [Bool.and_comm (b.getLsbD 0), Bool.and_comm (b.getLsbD 1),
        Bool.and_comm (b.getLsbD 2), Bool.and_comm (b.getLsbD 3)]
    exact xor4_rev _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.xor_false]
    rw [Bool.and_comm (b.getLsbD 0), Bool.and_comm (b.getLsbD 1), Bool.and_comm (b.getLsbD 2),
        Bool.and_comm (b.getLsbD 3), Bool.and_comm (b.getLsbD 4)]
    exact xor5_rev _ _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.xor_false]
    rw [Bool.and_comm (b.getLsbD 0), Bool.and_comm (b.getLsbD 1), Bool.and_comm (b.getLsbD 2),
        Bool.and_comm (b.getLsbD 3), Bool.and_comm (b.getLsbD 4), Bool.and_comm (b.getLsbD 5)]
    exact xor6_rev _ _ _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.xor_false]
    rw [Bool.and_comm (b.getLsbD 0), Bool.and_comm (b.getLsbD 1), Bool.and_comm (b.getLsbD 2),
        Bool.and_comm (b.getLsbD 3), Bool.and_comm (b.getLsbD 4), Bool.and_comm (b.getLsbD 5),
        Bool.and_comm (b.getLsbD 6)]
    exact xor7_rev _ _ _ _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, Bool.true_and]
    rw [Bool.and_comm (b.getLsbD 0), Bool.and_comm (b.getLsbD 1), Bool.and_comm (b.getLsbD 2),
        Bool.and_comm (b.getLsbD 3), Bool.and_comm (b.getLsbD 4), Bool.and_comm (b.getLsbD 5),
        Bool.and_comm (b.getLsbD 6), Bool.and_comm (b.getLsbD 7)]
    exact xor8_rev _ _ _ _ _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.false_xor]
    rw [Bool.and_comm (b.getLsbD 1), Bool.and_comm (b.getLsbD 2), Bool.and_comm (b.getLsbD 3),
        Bool.and_comm (b.getLsbD 4), Bool.and_comm (b.getLsbD 5), Bool.and_comm (b.getLsbD 6),
        Bool.and_comm (b.getLsbD 7)]
    exact xor7_rev _ _ _ _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.false_xor]
    rw [Bool.and_comm (b.getLsbD 2), Bool.and_comm (b.getLsbD 3), Bool.and_comm (b.getLsbD 4),
        Bool.and_comm (b.getLsbD 5), Bool.and_comm (b.getLsbD 6), Bool.and_comm (b.getLsbD 7)]
    exact xor6_rev _ _ _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.false_xor]
    rw [Bool.and_comm (b.getLsbD 3), Bool.and_comm (b.getLsbD 4), Bool.and_comm (b.getLsbD 5),
        Bool.and_comm (b.getLsbD 6), Bool.and_comm (b.getLsbD 7)]
    exact xor5_rev _ _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.false_xor]
    rw [Bool.and_comm (b.getLsbD 4), Bool.and_comm (b.getLsbD 5),
        Bool.and_comm (b.getLsbD 6), Bool.and_comm (b.getLsbD 7)]
    exact xor4_rev _ _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.false_xor]
    rw [Bool.and_comm (b.getLsbD 5), Bool.and_comm (b.getLsbD 6), Bool.and_comm (b.getLsbD 7)]
    exact xor3_rev _ _ _
  · simp only [Nat.reduceSub, Nat.reduceLeDiff, Nat.reduceLT, decide_true, decide_false,
               Bool.true_and, Bool.false_and, Bool.and_false, Bool.false_xor]
    rw [Bool.and_comm (b.getLsbD 6), Bool.and_comm (b.getLsbD 7)]
    exact xor2_rev _ _
  · simp [Bool.and_comm]
  · simp

/-- T09: Zero annihilates under carry-less multiplication. -/
public theorem T09_clmul8_zero_right (a : BitVec 8) : clmul8 a 0#8 = 0#16 := by
  unfold clmul8
  simp

/-- T10: Polynomial `1` is the multiplicative identity in GF(2)[x]. -/
public theorem T10_clmul8_one_right (a : BitVec 8) : clmul8 a 1#8 = a.zeroExtend 16 := by
  unfold clmul8
  simp

/-- T11: Carry-less multiplication distributes over polynomial addition (XOR). -/
public theorem T11_clmul8_distrib_xor (a b c : BitVec 8) :
    clmul8 a (b ^^^ c) = clmul8 a b ^^^ clmul8 a c := by
  apply BitVec.eq_of_getLsbD_eq
  intro k hk
  rw [BitVec.getLsbD_xor, clmul8_getLsbD a (b ^^^ c) k hk,
      clmul8_getLsbD a b k hk, clmul8_getLsbD a c k hk]
  simp only [BitVec.getLsbD_xor, bool_xor_and_right]
  exact bool_xor8_distrib _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

private theorem testBit_one_ne_zero (m : Nat) (hm : m ≠ 0) : Nat.testBit 1 m = false :=
  testBit_of_lt_two_pow 1 m
    (Nat.lt_of_lt_of_le (by omega : 1 < 2 ^ 1) (Nat.pow_le_pow_right (by omega) (by omega : 1 ≤ m)))

private theorem bit16_test_eq_getLsbD (z : BitVec 16) (j : Nat) (hj : j < 16) :
    ((z &&& (1#16 <<< j)) != 0#16) = z.getLsbD j := by
  by_cases hz : z.getLsbD j = true
  · have hne : (z &&& (1#16 <<< j)) ≠ 0#16 := by
      intro heq
      have hbit := congrArg (fun v => v.getLsbD j) heq
      rw [BitVec.getLsbD_and, BitVec.getLsbD_shiftLeft, BitVec.getLsbD_zero] at hbit
      have h1 : (1#16).getLsbD 0 = true := rfl
      simp only [hj, decide_true, Nat.not_lt_of_le (Nat.le_refl j),
                 Nat.sub_self, h1, Bool.true_and, Bool.and_true, hz] at hbit
      exact Bool.noConfusion hbit
    simp [hz, hne]
  · have hz' : z.getLsbD j = false := by cases h : z.getLsbD j <;> simp_all
    have heq : (z &&& (1#16 <<< j)) = 0#16 := by
      apply BitVec.eq_of_getLsbD_eq
      intro i _
      rw [BitVec.getLsbD_and, BitVec.getLsbD_shiftLeft, BitVec.getLsbD_zero]
      by_cases hij : j = i
      · subst hij; simp [hz']
      · by_cases hle : j ≤ i
        · have hne0 : i - j ≠ 0 := by omega
          have htb : (1#16).getLsbD (i - j) = false := by
            rw [BitVec.getLsbD_ofNat]
            simp [testBit_one_ne_zero (i - j) hne0]
          simp [htb]
        · have hlt : i < j := by omega
          simp [hlt]
    simp [hz', heq]

private theorem bit16_test_xor (x y : BitVec 16) (j : Nat) (hj : j < 16) :
    (((x ^^^ y) &&& (1#16 <<< j)) != 0#16) =
      (((x &&& (1#16 <<< j)) != 0#16) ^^ ((y &&& (1#16 <<< j)) != 0#16)) := by
  rw [bit16_test_eq_getLsbD (x ^^^ y) j hj, bit16_test_eq_getLsbD x j hj,
      bit16_test_eq_getLsbD y j hj, BitVec.getLsbD_xor]

private theorem polyReduceStep_xor (x y modShifted : BitVec 16) (j : Nat) (hj : j < 16) :
    polyReduceStep (x ^^^ y) modShifted (1#16 <<< j) =
      polyReduceStep x modShifted (1#16 <<< j) ^^^ polyReduceStep y modShifted (1#16 <<< j) := by
  unfold polyReduceStep
  rw [bit16_test_xor x y j hj]
  cases ((x &&& (1#16 <<< j)) != 0#16) <;> cases ((y &&& (1#16 <<< j)) != 0#16)
  · simp
  · simp [BitVec.xor_assoc]
  · simp only [Bool.true_xor, Bool.not_false, Bool.false_eq_true, ↓reduceIte]
    rw [BitVec.xor_assoc, BitVec.xor_comm y modShifted, ← BitVec.xor_assoc]
  · simp only [Bool.true_xor, Bool.not_true, Bool.false_eq_true, ↓reduceIte]
    rw [bv_xor_swap4 x modShifted y modShifted, BitVec.xor_self, BitVec.xor_zero]

private theorem polyReduceMod8_xor (x y : BitVec 16) (p : BitVec 8) :
    polyReduceMod8 (x ^^^ y) p = polyReduceMod8 x p ^^^ polyReduceMod8 y p := by
  unfold polyReduceMod8
  have h7 : (0x8000#16 : BitVec 16) = 1#16 <<< 15 := by decide
  have h6 : (0x4000#16 : BitVec 16) = 1#16 <<< 14 := by decide
  have h5 : (0x2000#16 : BitVec 16) = 1#16 <<< 13 := by decide
  have h4 : (0x1000#16 : BitVec 16) = 1#16 <<< 12 := by decide
  have h3 : (0x0800#16 : BitVec 16) = 1#16 <<< 11 := by decide
  have h2 : (0x0400#16 : BitVec 16) = 1#16 <<< 10 := by decide
  have h1 : (0x0200#16 : BitVec 16) = 1#16 <<< 9  := by decide
  have h0 : (0x0100#16 : BitVec 16) = 1#16 <<< 8  := by decide
  simp only [h7, h6, h5, h4, h3, h2, h1, h0]
  rw [polyReduceStep_xor _ _ _ 15 (by omega),
      polyReduceStep_xor _ _ _ 14 (by omega),
      polyReduceStep_xor _ _ _ 13 (by omega),
      polyReduceStep_xor _ _ _ 12 (by omega),
      polyReduceStep_xor _ _ _ 11 (by omega),
      polyReduceStep_xor _ _ _ 10 (by omega),
      polyReduceStep_xor _ _ _ 9  (by omega),
      polyReduceStep_xor _ _ _ 8  (by omega)]
  apply BitVec.eq_of_getLsbD_eq
  intro i hi
  simp [BitVec.truncate, hi]

private theorem gf2MulMod8_xor_right (a b c p : BitVec 8) :
    gf2MulMod8 a (b ^^^ c) p = gf2MulMod8 a b p ^^^ gf2MulMod8 a c p := by
  unfold gf2MulMod8
  rw [T11_clmul8_distrib_xor, polyReduceMod8_xor]

private theorem gf2MulMod8_xor_left (a b c p : BitVec 8) :
    gf2MulMod8 (a ^^^ b) c p = gf2MulMod8 a c p ^^^ gf2MulMod8 b c p := by
  unfold gf2MulMod8
  rw [T08_clmul8_comm (a ^^^ b) c, T11_clmul8_distrib_xor,
      T08_clmul8_comm c a, T08_clmul8_comm c b, polyReduceMod8_xor]

private theorem gf2MulMod8_zero_right (a p : BitVec 8) :
    gf2MulMod8 a 0#8 p = 0#8 := by
  unfold gf2MulMod8
  rw [T09_clmul8_zero_right]
  unfold polyReduceMod8 polyReduceStep
  simp

private theorem gf2MulMod8_zero_left (b p : BitVec 8) :
    gf2MulMod8 0#8 b p = 0#8 := by
  unfold gf2MulMod8
  rw [T08_clmul8_comm]
  exact gf2MulMod8_zero_right b p

/-- T12: Exact combinatorial theorem — there are EXACTLY 30 irreducible degree-8
    polynomials over GF(2)[x] (true finite fields GF(2^8)). -/
public theorem T12_irred_poly8_count_eq_30 : countIrredPoly8 = 30 := by
  decide

/-- T12a: Full trial-divisor (`d = 2..31`) matches both fast `isIrreduciblePoly8` AND
    the exact C11 `GF2_IRRED_POLY8[30]` table on ALL 256 polynomials (30 irred, 226 red). -/
public theorem T12a_full_trial_div_matches_c11_table :
    (∀ p : BitVec 8, isIrreduciblePoly8Full p = isIrreduciblePoly8 p) ∧
    (∀ p : BitVec 8, isIrreduciblePoly8Full p = c11IrredPoly8Table.contains p) ∧
    c11IrredPoly8Table.length = 30 ∧
    countReduciblePoly8 = 226 := by
  decide

/-- T13: The AES field polynomial x^8 + x^4 + x^3 + x + 1 (0x1B) is irreducible. -/
public theorem T13_aes_poly_irreducible : isIrreduciblePoly8 0x1B#8 = true := by
  decide

/-- T14: Dobzhansky-Muller Algebraic Speciation Barrier — on reducible polynomial x^8 + 1 (0x01),
    non-zero genomes `0x03` (x+1) and `0xFF` (x^7+...+1) collapse to 0 (zero divisor!). -/
public theorem T14_dobzhansky_muller_zero_divisor :
    (0x03#8 != 0#8) ∧ (0xFF#8 != 0#8) ∧ (gf2MulMod8 0x03#8 0xFF#8 0x01#8 = 0#8) := by
  decide

/-- T15: Planarian Cauchy ECC matrix [[1, 3], [5, 1]] has strictly non-zero determinant
    in ALL 30 irreducible GF(2^8) fields! -/
public theorem T15_cauchy_ecc_det_nonzero_all_30_fields : all30CauchyDetNonzero = true := by
  decide

private def checkDetInv (p : BitVec 8) : Bool :=
  gf2MulMod8 (0x01#8 ^^^ gf2MulMod8 0x03#8 0x05#8 p) (cauchyDetInv p) p == 1#8

private theorem checkDetInv_all : c11IrredPoly8Table.all checkDetInv = true := by
  decide

/-- T15a: `cauchyDetInv p` is the exact multiplicative inverse of the Cauchy determinant
    `1 ^ (3 * 5)` in ALL 30 irreducible GF(2^8) fields! -/
public theorem T15a_cauchy_det_inv_exact :
    ∀ p ∈ c11IrredPoly8Table,
      gf2MulMod8 (0x01#8 ^^^ gf2MulMod8 0x03#8 0x05#8 p) (cauchyDetInv p) p = 1#8 := by
  intro p hp
  have h := List.all_eq_true.mp checkDetInv_all p hp
  unfold checkDetInv at h
  exact beq_iff_eq.mp h

private theorem cauchyRecLo_split (p lo hi : BitVec 8) :
    cauchyRecLo p lo hi = cauchyRecLo p lo 0#8 ^^^ cauchyRecLo p 0#8 hi := by
  unfold cauchyRecLo
  simp only [gf2MulMod8_zero_left, BitVec.xor_zero, BitVec.zero_xor, gf2MulMod8_xor_right]
  exact bv_xor_swap4 _ _ _ _

private theorem cauchyRecHi_split (p lo hi : BitVec 8) :
    cauchyRecHi p lo hi = cauchyRecHi p lo 0#8 ^^^ cauchyRecHi p 0#8 hi := by
  unfold cauchyRecHi
  simp only [gf2MulMod8_zero_left, BitVec.xor_zero, BitVec.zero_xor, gf2MulMod8_xor_right]
  exact bv_xor_swap4 _ _ _ _

private theorem cauchyRecLo_lo_lin (p a b : BitVec 8) :
    cauchyRecLo p (a ^^^ b) 0#8 = cauchyRecLo p a 0#8 ^^^ cauchyRecLo p b 0#8 := by
  unfold cauchyRecLo
  simp only [gf2MulMod8_zero_left, BitVec.xor_zero, gf2MulMod8_xor_left, gf2MulMod8_xor_right]
  exact bv_xor_swap4 _ _ _ _

private theorem cauchyRecHi_hi_lin (p a b : BitVec 8) :
    cauchyRecHi p 0#8 (a ^^^ b) = cauchyRecHi p 0#8 a ^^^ cauchyRecHi p 0#8 b := by
  unfold cauchyRecHi
  simp only [gf2MulMod8_zero_left, BitVec.zero_xor, gf2MulMod8_xor_left, gf2MulMod8_xor_right]
  exact bv_xor_swap4 _ _ _ _

private theorem bv8_basis_decomp (x : BitVec 8) :
    x = (if x.getLsbD 0 then 0x01#8 else 0#8) ^^^
        (if x.getLsbD 1 then 0x02#8 else 0#8) ^^^
        (if x.getLsbD 2 then 0x04#8 else 0#8) ^^^
        (if x.getLsbD 3 then 0x08#8 else 0#8) ^^^
        (if x.getLsbD 4 then 0x10#8 else 0#8) ^^^
        (if x.getLsbD 5 then 0x20#8 else 0#8) ^^^
        (if x.getLsbD 6 then 0x40#8 else 0#8) ^^^
        (if x.getLsbD 7 then 0x80#8 else 0#8) := by
  have h : ∀ w : BitVec 8,
      w = (if w.getLsbD 0 then 0x01#8 else 0#8) ^^^
          (if w.getLsbD 1 then 0x02#8 else 0#8) ^^^
          (if w.getLsbD 2 then 0x04#8 else 0#8) ^^^
          (if w.getLsbD 3 then 0x08#8 else 0#8) ^^^
          (if w.getLsbD 4 then 0x10#8 else 0#8) ^^^
          (if w.getLsbD 5 then 0x20#8 else 0#8) ^^^
          (if w.getLsbD 6 then 0x40#8 else 0#8) ^^^
          (if w.getLsbD 7 then 0x80#8 else 0#8) := by decide
  exact h x

private theorem xor_linear_eq_on_basis (f g : BitVec 8 → BitVec 8)
    (hf : ∀ a b, f (a ^^^ b) = f a ^^^ f b)
    (hg : ∀ a b, g (a ^^^ b) = g a ^^^ g b)
    (h0 : f 0#8 = g 0#8)
    (hb0 : f 0x01#8 = g 0x01#8) (hb1 : f 0x02#8 = g 0x02#8)
    (hb2 : f 0x04#8 = g 0x04#8) (hb3 : f 0x08#8 = g 0x08#8)
    (hb4 : f 0x10#8 = g 0x10#8) (hb5 : f 0x20#8 = g 0x20#8)
    (hb6 : f 0x40#8 = g 0x40#8) (hb7 : f 0x80#8 = g 0x80#8)
    (x : BitVec 8) : f x = g x := by
  conv => lhs; rw [bv8_basis_decomp x]
  conv => rhs; rw [bv8_basis_decomp x]
  simp only [hf, hg]
  have step : ∀ (c : Bool) (v : BitVec 8), f v = g v →
      f (if c then v else 0#8) = g (if c then v else 0#8) := by
    intro c v hv; cases c <;> simp [h0, hv]
  rw [step _ _ hb0, step _ _ hb1, step _ _ hb2, step _ _ hb3,
      step _ _ hb4, step _ _ hb5, step _ _ hb6, step _ _ hb7]

private theorem gf2MulMod8_comm (a b p : BitVec 8) :
    gf2MulMod8 a b p = gf2MulMod8 b a p := by
  unfold gf2MulMod8
  rw [T08_clmul8_comm]

private theorem cauchyRecLo_zero_lo (p hi : BitVec 8) :
    cauchyRecLo p 0#8 hi = 0#8 := by
  unfold cauchyRecLo
  dsimp only
  rw [gf2MulMod8_zero_left, BitVec.zero_xor, BitVec.zero_xor,
      gf2MulMod8_comm hi 0x03#8 p, BitVec.xor_self, gf2MulMod8_zero_right]

private theorem cauchyRecHi_zero_hi (p lo : BitVec 8) :
    cauchyRecHi p lo 0#8 = 0#8 := by
  unfold cauchyRecHi
  dsimp only
  rw [gf2MulMod8_zero_left, BitVec.xor_zero, BitVec.xor_zero,
      gf2MulMod8_comm lo 0x05#8 p, BitVec.xor_self, gf2MulMod8_zero_right]

private def checkMul35 (p e : BitVec 8) : Bool :=
  (gf2MulMod8 (gf2MulMod8 e 0x05#8 p) 0x03#8 p == gf2MulMod8 e 0x0F#8 p) &&
  (gf2MulMod8 (gf2MulMod8 e 0x03#8 p) 0x05#8 p == gf2MulMod8 e 0x0F#8 p) &&
  (gf2MulMod8 (e ^^^ gf2MulMod8 e 0x0F#8 p) (cauchyDetInv p) p == e)

private theorem checkMul35_e0 : c11IrredPoly8Table.all (fun p => checkMul35 p 0x01#8) = true := by decide
private theorem checkMul35_e1 : c11IrredPoly8Table.all (fun p => checkMul35 p 0x02#8) = true := by decide
private theorem checkMul35_e2 : c11IrredPoly8Table.all (fun p => checkMul35 p 0x04#8) = true := by decide
private theorem checkMul35_e3 : c11IrredPoly8Table.all (fun p => checkMul35 p 0x08#8) = true := by decide
private theorem checkMul35_e4 : c11IrredPoly8Table.all (fun p => checkMul35 p 0x10#8) = true := by decide
private theorem checkMul35_e5 : c11IrredPoly8Table.all (fun p => checkMul35 p 0x20#8) = true := by decide
private theorem checkMul35_e6 : c11IrredPoly8Table.all (fun p => checkMul35 p 0x40#8) = true := by decide
private theorem checkMul35_e7 : c11IrredPoly8Table.all (fun p => checkMul35 p 0x80#8) = true := by decide

private theorem checkMul35_spec (p e : BitVec 8) (h : checkMul35 p e = true) :
    cauchyRecLo p e 0#8 = e ∧ cauchyRecHi p 0#8 e = e := by
  unfold checkMul35 at h
  simp only [Bool.and_eq_true, beq_iff_eq] at h
  have h35 := h.1.1
  have h53 := h.1.2
  have hinv := h.2
  constructor
  · unfold cauchyRecLo
    dsimp only
    rw [gf2MulMod8_zero_left, BitVec.xor_zero, BitVec.xor_zero,
        gf2MulMod8_comm 0x03#8, h35, gf2MulMod8_comm (cauchyDetInv p), hinv]
  · unfold cauchyRecHi
    dsimp only
    rw [gf2MulMod8_zero_left, BitVec.zero_xor, BitVec.zero_xor,
        gf2MulMod8_comm 0x05#8, h53, BitVec.xor_comm, gf2MulMod8_comm (cauchyDetInv p), hinv]

private theorem cauchyRec_zero_zero (p : BitVec 8) :
    cauchyRecLo p 0#8 0#8 = 0#8 ∧ cauchyRecHi p 0#8 0#8 = 0#8 :=
  ⟨cauchyRecLo_zero_lo p 0#8, cauchyRecHi_zero_hi p 0#8⟩

/-- T16: Universal 2x2 GF(2^8) Cauchy MDS Recovery Theorem across ALL 30 irreducible polynomials
    AND ALL 65,536 `(lo, hi)` byte pairs (1,966,080 combinations, proven algebraically via
    F_2-linearity + 30x8 basis verification inside the pure Lean 4 kernel!). -/
public theorem T16_planarian_50pct_exact_recovery (p : BitVec 8) (hp : isIrreduciblePoly8 p = true)
    (lo hi : BitVec 8) :
    cauchyRecLo p lo hi = lo ∧ cauchyRecHi p lo hi = hi := by
  have hmem : p ∈ c11IrredPoly8Table := by
    have h1 := T12a_full_trial_div_matches_c11_table.1 p
    have h2 := T12a_full_trial_div_matches_c11_table.2.1 p
    have hc : c11IrredPoly8Table.contains p = true := by rw [← h2, h1, hp]
    exact List.mem_of_elem_eq_true hc
  have b0 := checkMul35_spec p 0x01#8 (List.all_eq_true.mp checkMul35_e0 p hmem)
  have b1 := checkMul35_spec p 0x02#8 (List.all_eq_true.mp checkMul35_e1 p hmem)
  have b2 := checkMul35_spec p 0x04#8 (List.all_eq_true.mp checkMul35_e2 p hmem)
  have b3 := checkMul35_spec p 0x08#8 (List.all_eq_true.mp checkMul35_e3 p hmem)
  have b4 := checkMul35_spec p 0x10#8 (List.all_eq_true.mp checkMul35_e4 p hmem)
  have b5 := checkMul35_spec p 0x20#8 (List.all_eq_true.mp checkMul35_e5 p hmem)
  have b6 := checkMul35_spec p 0x40#8 (List.all_eq_true.mp checkMul35_e6 p hmem)
  have b7 := checkMul35_spec p 0x80#8 (List.all_eq_true.mp checkMul35_e7 p hmem)
  have hz := cauchyRec_zero_zero p
  have hlo1 : cauchyRecLo p lo 0#8 = lo :=
    xor_linear_eq_on_basis (fun x => cauchyRecLo p x 0#8) id
      (cauchyRecLo_lo_lin p) (fun _ _ => rfl) hz.1
      b0.1 b1.1 b2.1 b3.1 b4.1 b5.1 b6.1 b7.1 lo
  have hhi2 : cauchyRecHi p 0#8 hi = hi :=
    xor_linear_eq_on_basis (fun x => cauchyRecHi p 0#8 x) id
      (cauchyRecHi_hi_lin p) (fun _ _ => rfl) hz.2
      b0.2 b1.2 b2.2 b3.2 b4.2 b5.2 b6.2 b7.2 hi
  constructor
  · rw [cauchyRecLo_split, hlo1, cauchyRecLo_zero_lo, BitVec.xor_zero]
  · rw [cauchyRecHi_split, cauchyRecHi_zero_hi, hhi2, BitVec.zero_xor]

/-- T16a: Explicit AES field `0x1B` corollary (`det = 0x0E`, `det^-1 = 0xE5`). -/
public theorem T16a_planarian_aes_exact_recovery (lo hi : BitVec 8) :
    let p0 := lo ^^^ gf2MulMod8 hi 0x03#8 0x1B#8
    let p1 := gf2MulMod8 lo 0x05#8 0x1B#8 ^^^ hi
    let rec_lo := gf2MulMod8 0xE5#8 (p0 ^^^ gf2MulMod8 0x03#8 p1 0x1B#8) 0x1B#8
    let rec_hi := gf2MulMod8 0xE5#8 (gf2MulMod8 0x05#8 p0 0x1B#8 ^^^ p1) 0x1B#8
    rec_lo = lo ∧ rec_hi = hi := by
  have hinv : cauchyDetInv 0x1B#8 = 0xE5#8 := by decide
  have h := T16_planarian_50pct_exact_recovery 0x1B#8 T13_aes_poly_irreducible lo hi
  unfold cauchyRecLo cauchyRecHi at h
  rw [hinv] at h
  exact h

/-! ## Part III: Symplectic Reversible Silicon Gates (`TOFFOLI`, `FREDKIN`), `popcnt64` & Landauer -/

/-- T17: 64-bit hardware Toffoli gate is an exact self-involution (0 Landauer heat). -/
public theorem T17_toffoli64_involution (d s t : BitVec 64) :
    let (d1, s1, t1) := toffoli64 d s t
    toffoli64 d1 s1 t1 = (d, s, t) := by
  unfold toffoli64
  simp only [Prod.mk.injEq, and_true]
  rw [BitVec.xor_assoc, BitVec.xor_self, BitVec.xor_zero]

/-- T18: 64-bit hardware Fredkin gate is an exact self-involution (0 Landauer heat). -/
public theorem T18_fredkin64_involution (c s t : BitVec 64) :
    let (c1, s1, t1) := fredkin64 c s t
    fredkin64 c1 s1 t1 = (c, s, t) := by
  unfold fredkin64
  simp only [Prod.mk.injEq, true_and]
  constructor <;>
  · apply BitVec.eq_of_getLsbD_eq
    intro i _
    simp only [BitVec.getLsbD_xor, BitVec.getLsbD_and]
    cases c.getLsbD i <;> cases s.getLsbD i <;> cases t.getLsbD i <;> rfl

/-- T19: Fredkin gate conserves bitwise XOR (`s' ^^^ t' = s ^^^ t`). -/
public theorem T19_fredkin64_xor_conserved (c s t : BitVec 64) :
    let (_, s1, t1) := fredkin64 c s t
    s1 ^^^ t1 = s ^^^ t := by
  unfold fredkin64
  dsimp only
  apply BitVec.eq_of_getLsbD_eq
  intro i _
  simp only [BitVec.getLsbD_xor, BitVec.getLsbD_and]
  cases c.getLsbD i <;> cases s.getLsbD i <;> cases t.getLsbD i <;> rfl

/-- T20: Fredkin gate conserves bitwise AND (`s' &&& t' = s &&& t`). -/
public theorem T20_fredkin64_and_conserved (c s t : BitVec 64) :
    let (_, s1, t1) := fredkin64 c s t
    s1 &&& t1 = s &&& t := by
  unfold fredkin64
  dsimp only
  apply BitVec.eq_of_getLsbD_eq
  intro i _
  simp only [BitVec.getLsbD_xor, BitVec.getLsbD_and]
  cases c.getLsbD i <;> cases s.getLsbD i <;> cases t.getLsbD i <;> rfl

private theorem fredkin64_popcntUpTo (c s t : BitVec 64) (n : Nat) :
    let (_, s1, t1) := fredkin64 c s t
    popcntUpTo s1 n + popcntUpTo t1 n = popcntUpTo s n + popcntUpTo t n := by
  induction n with
  | zero => simp [popcntUpTo]
  | succ k ih =>
    unfold fredkin64 at ih ⊢
    dsimp only at ih ⊢
    unfold popcntUpTo
    have hbit :
        (if (s ^^^ ((s ^^^ t) &&& c)).getLsbD k then 1 else 0) +
        (if (t ^^^ ((s ^^^ t) &&& c)).getLsbD k then 1 else 0) =
        (if s.getLsbD k then 1 else 0) + (if t.getLsbD k then 1 else 0) := by
      simp only [BitVec.getLsbD_xor, BitVec.getLsbD_and]
      cases c.getLsbD k <;> cases s.getLsbD k <;> cases t.getLsbD k <;> rfl
    omega

/-- T20a: Exact Hamming-Weight Conservation — hardware `FREDKIN` strictly preserves
    total 64-bit `popcnt64(s') + popcnt64(t') = popcnt64(s) + popcnt64(t)`! -/
public theorem T20a_fredkin64_popcnt_conserved (c s t : BitVec 64) :
    let (_, s1, t1) := fredkin64 c s t
    popcnt64 s1 + popcnt64 t1 = popcnt64 s + popcnt64 t :=
  fredkin64_popcntUpTo c s t 64

/-- T21 [Elementary helper lemma]: BitVec 64 XOR self-cancellation (`x ^^^ x = 0`),
    used to model zero bit-erasure count on an unchanged 64-bit register. -/
public theorem T21_landauer_zero_change_zero_erasure (x : BitVec 64) :
    x ^^^ x = 0#64 :=
  BitVec.xor_self

/-- T22: Irreversible `AND` mask erases the exact bit-set `old &&& ~~~mask`. -/
public theorem T22_landauer_and_exact_erased_bits (old mask : BitVec 64) :
    old ^^^ (old &&& mask) = old &&& (~~~mask) := by
  apply BitVec.eq_of_getLsbD_eq
  intro i hi
  simp only [BitVec.getLsbD_xor, BitVec.getLsbD_and, BitVec.getLsbD_not, hi, decide_true,
             Bool.true_and]
  cases old.getLsbD i <;> cases mask.getLsbD i <;> rfl

/-! ## Part IV: 2-Adic Ultrametric Centromere Geometry (`TZCNT`) -/

/-- T23 [Elementary helper lemma]: BitVec 32 XOR self-cancellation (`a ^^^ a = 0`). -/
public theorem T23_ultrametric_self_zero (a : BitVec 32) : a ^^^ a = 0#32 :=
  BitVec.xor_self

/-- T24 [Elementary helper lemma]: BitVec 32 XOR commutativity (`a ^^^ b = b ^^^ a`). -/
public theorem T24_ultrametric_symmetry (a b : BitVec 32) : a ^^^ b = b ^^^ a :=
  BitVec.xor_comm a b

/-- T25: 2-Adic Mask Strong Triangle Inequality (Ultrametricity):
    If `a ^^^ b` and `b ^^^ c` both have all masked bits zero, then `a ^^^ c` does too. -/
public theorem T25_ultrametric_strong_triangle (a b c mask : BitVec 32) :
    (a ^^^ b) &&& mask = 0#32 →
    (b ^^^ c) &&& mask = 0#32 →
    (a ^^^ c) &&& mask = 0#32 := by
  intro hab hbc
  apply BitVec.eq_of_getLsbD_eq
  intro i _
  have h1 := congrArg (fun v => v.getLsbD i) hab
  have h2 := congrArg (fun v => v.getLsbD i) hbc
  rw [BitVec.getLsbD_and, BitVec.getLsbD_xor, BitVec.getLsbD_zero] at h1 h2 ⊢
  cases ha : a.getLsbD i <;> cases hb : b.getLsbD i <;>
  cases hc : c.getLsbD i <;> cases hm : mask.getLsbD i <;>
  rw [ha, hb] at h1 <;> rw [hb, hc] at h2 <;>
  rw [hm] at h1 h2 <;>
  simp_all

private theorem tzcntFrom_strong_triangle (a b c : BitVec 32) (rem : Nat) :
    ∀ i : Nat, min (tzcntFrom (a ^^^ b) i rem) (tzcntFrom (b ^^^ c) i rem) ≤
               tzcntFrom (a ^^^ c) i rem := by
  induction rem with
  | zero =>
    intro i; simp [tzcntFrom]
  | succ k ih =>
    intro i
    unfold tzcntFrom
    simp only [BitVec.getLsbD_xor]
    cases a.getLsbD i <;> cases b.getLsbD i <;> cases c.getLsbD i
    · have := ih (i + 1); simp; omega
    · simp
    · simp
    · simp
    · simp
    · simp
    · simp
    · have := ih (i + 1); simp; omega

/-- T25a: Exact 2-Adic Strong Triangle Inequality for Hardware `TZCNT`:
    `tzcnt32 (a ^^^ c) >= min (tzcnt32 (a ^^^ b)) (tzcnt32 (b ^^^ c))` for ALL 32-bit clades! -/
public theorem T25a_tzcnt32_strong_triangle (a b c : BitVec 32) :
    tzcnt32 (a ^^^ c) ≥ min (tzcnt32 (a ^^^ b)) (tzcnt32 (b ^^^ c)) :=
  tzcntFrom_strong_triangle a b c 32 0

/-! ## Part V: Native x86-64 JIT Memory Safety, Halting & Multi-Step Thermodynamic Ledger -/

/-- T26: Every JIT masked byte tape access `(h &&& 0x7F)` is strictly `< 128` (0 OOB!). -/
public theorem T26_jit_byte_mask_inbounds (h : BitVec 32) :
    (h &&& 0x7F#32).toNat < 128 := by
  rw [BitVec.toNat_and]
  have hle : h.toNat &&& (0x7F#32).toNat ≤ (0x7F#32).toNat := Nat.and_le_right
  have hv : (0x7F#32).toNat = 127 := rfl
  omega

/-- T27: Every JIT masked 64-bit qword tape access `(h &&& 0x78)` satisfies `off + 8 <= 128` (0 OOB!). -/
public theorem T27_jit_qword_mask_inbounds (h : BitVec 32) :
    (h &&& 0x78#32).toNat + 8 ≤ 128 := by
  rw [BitVec.toNat_and]
  have hle : h.toNat &&& (0x78#32).toNat ≤ (0x78#32).toNat := Nat.and_le_right
  have hv : (0x78#32).toNat = 120 := rfl
  omega

/-- T28 [Elementary helper lemma]: Positive `Nat` predecessor is strictly smaller (`g - 1 < g`),
    modeling the decreasing measure of the loop gas counter. -/
public theorem T28_jit_gas_strictly_decreasing (g : Nat) (hg : g > 0) :
    g - 1 < g := by
  omega

/-- T29 [Elementary helper lemma]: Single-transfer integer sum invariance
    `(e_src - amt) + (e_dst + amt) = e_src + e_dst`. -/
public theorem T29_first_law_transfer_conservation (e_src e_dst amt : Int) :
    (e_src - amt) + (e_dst + amt) = e_src + e_dst := by
  omega

private theorem applyTransfer_preserves_residual (s : LedgerState) (op : TransferOp) (e_init : Int) :
    (applyTransfer s op).residual e_init = s.residual e_init := by
  cases op <;> simp [applyTransfer, LedgerState.residual] <;> omega

/-- T29a: Multi-Step Conserved Transfer Ledger Invariance — ANY finite sequence of
    transfers across all 5 pools + inflow/export preserves exact 0 residual! -/
public theorem T29a_first_law_ledger_multistep (s : LedgerState) (ops : List TransferOp) (e_init : Int)
    (h0 : s.residual e_init = 0) :
    (applyTransfers s ops).residual e_init = 0 := by
  induction ops generalizing s with
  | nil => exact h0
  | cons op rest ih =>
    apply ih (applyTransfer s op)
    rw [applyTransfer_preserves_residual, h0]

private theorem applyTransfer_heat_sink_monotone (s : LedgerState) (op : TransferOp)
    (hNonNeg : op.isNonNegHeatRadiation) :
    s.e_heat_snk ≤ (applyTransfer s op).e_heat_snk := by
  cases op with
  | heatToSink a =>
    have h1 : s.e_heat_snk + 0 ≤ s.e_heat_snk + a := Int.add_le_add_left hNonNeg s.e_heat_snk
    rwa [Int.add_zero] at h1
  | detritusToCell _ | cellToHeat _ | cellToDetritus _ | cellToMembrane _
  | membraneToDetritus _ | externalInflow _ | externalExport _ =>
    exact Int.le_refl _

private theorem applyTransfers_heat_sink_monotone (s : LedgerState) (ops : List TransferOp)
    (hNonNeg : ∀ op ∈ ops, op.isNonNegHeatRadiation) :
    s.e_heat_snk ≤ (applyTransfers s ops).e_heat_snk := by
  induction ops generalizing s with
  | nil => exact Int.le_refl _
  | cons op rest ih =>
    have hHead : op.isNonNegHeatRadiation := hNonNeg op List.mem_cons_self
    have hTail : ∀ o ∈ rest, o.isNonNegHeatRadiation :=
      fun o ho => hNonNeg o (List.mem_cons_of_mem op ho)
    exact Int.le_trans (applyTransfer_heat_sink_monotone s op hHead)
                       (ih (applyTransfer s op) hTail)

/-- T30: Multi-Step Ledger Radiated-Heat Sink Monotonicity, Conservation, and Zero-Alive Gate —
    Over `Spec.lean`'s `LedgerState` and `applyTransfers`, any finite sequence `ops` whose
    `.heatToSink a` transfers satisfy `0 ≤ a` preserves exact `0` total ledger residual AND
    never decreases `e_heat_snk`; additionally `oviGate 0 raw_ovi = 0` for any `raw_ovi`.
    (Formalizes the discrete integer accounting rule in `soup_engine.c` and `metrology_ovbs.c`,
    not physical calorimetry.) -/
public theorem T30_ledger_heat_sink_monotone_and_zero_weight_gate
    (s : LedgerState) (ops : List TransferOp) (e_init : Int) (raw_ovi : Nat)
    (h0 : s.residual e_init = 0)
    (hNonNeg : ∀ op ∈ ops, op.isNonNegHeatRadiation) :
    s.e_heat_snk ≤ (applyTransfers s ops).e_heat_snk ∧
    (applyTransfers s ops).residual e_init = 0 ∧
    oviGate 0 raw_ovi = 0 :=
  ⟨applyTransfers_heat_sink_monotone s ops hNonNeg,
   T29a_first_law_ledger_multistep s ops e_init h0,
   rfl⟩

/-! ## Part VI: Philox Domain Isolation, 3-Tier Seed Firewall, OEE Archive & Weismann Isolation -/

/-- T39: Philox Domain Disjointness — distinct domain tags (`dom1 ≠ dom2`), including
    `WORLD_EXOG = 0` vs `ENTITY_INT = 1`, produce strictly distinct 8-tuple counter keys. -/
public theorem T39_philox_domain_disjoint
    (ver1 seed1 dom1 ent1 org1 tick1 purp1 idx1
     ver2 seed2 dom2 ent2 org2 tick2 purp2 idx2 : Nat)
    (hDom : dom1 ≠ dom2) :
    PhiloxCounterKey.mk ver1 seed1 dom1 ent1 org1 tick1 purp1 idx1 ≠
    PhiloxCounterKey.mk ver2 seed2 dom2 ent2 org2 tick2 purp2 idx2 ∧
    PhiloxCounterKey.mk ver1 seed1 PHILOX_DOM_WORLD_EXOG ent1 org1 tick1 purp1 idx1 ≠
    PhiloxCounterKey.mk ver1 seed1 PHILOX_DOM_ENTITY_INT ent1 org1 tick1 purp1 idx1 := by
  constructor
  · intro hEq
    exact hDom (congrArg PhiloxCounterKey.dom hEq)
  · intro hEq
    have h01 : PHILOX_DOM_WORLD_EXOG = PHILOX_DOM_ENTITY_INT :=
      congrArg PhiloxCounterKey.dom hEq
    exact Nat.zero_ne_one h01

/-- T40: 3-Tier Seed Firewall Pairwise Disjointness — Practice (`100..999`), Tuning (`2000..2999`),
    and Locked Held-Out (`≥ 5000`) ranges are strictly pairwise disjoint for every seed `s : Nat`. -/
public theorem T40_seed_firewall_3tier_disjoint (s : Nat) :
    ¬(isPracticeSeed s ∧ isTuningSeed s) ∧
    ¬(isPracticeSeed s ∧ isLockedSeed s) ∧
    ¬(isTuningSeed s ∧ isLockedSeed s) := by
  unfold isPracticeSeed isTuningSeed isLockedSeed
  refine ⟨?_, ?_, ?_⟩ <;> intro h <;> omega

private theorem oeeArchive_batch_monotone (a : OeeArchive) (ids : List Nat) :
    a.entries.length ≤ (a.appendNovelBatch ids).entries.length ∧
    a.cumNovelty ≤ (a.appendNovelBatch ids).cumNovelty := by
  induction ids generalizing a with
  | nil => exact ⟨Nat.le_refl _, Nat.le_refl _⟩
  | cons id rest ih =>
    have hNext := ih (a.appendNovel id)
    have hLen : a.entries.length ≤ (a.appendNovel id).entries.length := by
      simp [OeeArchive.appendNovel]
    have hNov : a.cumNovelty ≤ (a.appendNovel id).cumNovelty := by
      simp [OeeArchive.appendNovel]
    exact ⟨Nat.le_trans hLen hNext.1, Nat.le_trans hNov hNext.2⟩

/-- T41: OEE Archive Monotone Growth — appending a novel lineage strictly increases archive size
    (`n < n + 1`) and never decreases cumulative novelty count (both single-step and arbitrary batches). -/
public theorem T41_oee_archive_monotone_growth
    (a : OeeArchive) (lineageId : Nat) (batch : List Nat) :
    let a1 := a.appendNovel lineageId
    a1.entries.length = a.entries.length + 1 ∧
    a.entries.length < a1.entries.length ∧
    a.cumNovelty ≤ a1.cumNovelty ∧
    a.cumNovelty ≤ (a.appendNovelBatch batch).cumNovelty := by
  have hBatch := oeeArchive_batch_monotone a batch
  refine ⟨?_, ?_, ?_, hBatch.2⟩
  · simp [OeeArchive.appendNovel]
  · simp [OeeArchive.appendNovel]
  · simp [OeeArchive.appendNovel]

/-- T42: Weismann Soma Mutation Isolation — soma-only state perturbation (XOR mask, arbitrary
    soma map `f`, or finite sequence of somatic perturbations) preserves germline bitvector identically. -/
public theorem T42_weismann_soma_mutation_isolated
    (st : WeismannState) (mask : BitVec 64) (f : BitVec 64 → BitVec 64)
    (trace : List (BitVec 64)) :
    (perturbSomaOnly st mask).germline = st.germline ∧
    ({ st with soma := f st.soma } : WeismannState).germline = st.germline ∧
    (perturbSomaTrace st trace).germline = st.germline := by
  refine ⟨rfl, rfl, ?_⟩
  induction trace generalizing st with
  | nil => rfl
  | cons m rest ih =>
    simpa [perturbSomaTrace, List.foldl_cons, perturbSomaOnly] using ih (perturbSomaOnly st m)

/-! ## Part VII: Non-Vacuity Witnesses & Adversarial Sharp Negative Boundaries -/

/-- T43: Non-Vacuity Witness for Asymmetric Weismann Ribosome Inversion (`T05`):
    `asymRibosomeMat` is strictly non-involutive (`M ≠ M^-1`), non-identity (`M ≠ I`),
    strictly transforms `0xA5` under bias `0x5A`, and inverts back bit-exact. -/
public theorem T43_nonvacuous_weismann_asymmetric_witness :
    asymRibosomeMat ≠ asymRibosomeInv ∧
    asymRibosomeMat ≠ gfniIdentity ∧
    let y := gfniApply asymRibosomeMat 0xA5#8 0x5A#8
    let c_inv := gfniApply asymRibosomeInv 0x5A#8 0#8
    y ≠ 0xA5#8 ∧ gfniApply asymRibosomeInv y c_inv = 0xA5#8 := by
  decide

/-- T44: Non-Vacuity Witness for Planarian Cauchy MDS Recovery (`T16`) & Reducible Boundary:
    `0x1B` is irreducible and recovers `(0xDE, 0xAD)` bit-exact, while reducible `0x01`
    fails irreducibility and has sentinel `cauchyDetInv 0x01 = 0`. -/
public theorem T44_nonvacuous_cauchy_mds_witness :
    isIrreduciblePoly8 0x1B#8 = true ∧
    isIrreduciblePoly8 0x01#8 = false ∧
    cauchyRecLo 0x1B#8 0xDE#8 0xAD#8 = 0xDE#8 ∧
    cauchyRecHi 0x1B#8 0xDE#8 0xAD#8 = 0xAD#8 ∧
    cauchyDetInv 0x01#8 = 0#8 := by
  have hRec := T16_planarian_50pct_exact_recovery 0x1B#8 T13_aes_poly_irreducible 0xDE#8 0xAD#8
  have hRed : isIrreduciblePoly8 0x01#8 = false := by decide
  have hInv : cauchyDetInv 0x01#8 = 0#8 := by decide
  exact ⟨T13_aes_poly_irreducible, hRed, hRec.1, hRec.2, hInv⟩

/-- T45: Non-Vacuity Witness for 5-Pool Multi-Step Conservation (`T29a`) + 1-uq Leak Detection:
    Exercises every `TransferOp` constructor from a live `1,000,000 uq` state (`residual = 0`)
    and proves an uncompensated 1-uq detritus leak strictly breaks conservation (`residual = -1`). -/
public theorem T45_nonvacuous_ledger_multistep_witness :
    let s0 : LedgerState := ⟨200000, 50000, 750000, 0, 0, 0, 0⟩
    let ops : List TransferOp := [
      .detritusToCell 100000, .cellToHeat 40000, .cellToDetritus 20000,
      .cellToMembrane 15000, .membraneToDetritus 5000, .heatToSink 25000,
      .externalInflow 50000, .externalExport 10000
    ]
    let s1 := applyTransfers s0 ops
    let sLeak : LedgerState := { s1 with e_detritus := s1.e_detritus - 1 }
    s0.residual 1000000 = 0 ∧
    s1.residual 1000000 = 0 ∧
    sLeak.residual 1000000 = -1 := by
  simp only [applyTransfers, List.foldl_cons, List.foldl_nil, applyTransfer, LedgerState.residual]
  omega

/-- T46: Non-Vacuity Witness for the 3-Tier Seed Firewall (`T40`) and Philox Domains (`T39`):
    Constructs concrete inhabitants in Practice (`500`), Tuning (`2500`), and Locked (`5000`)
    bands alongside distinct Philox domain tags (`0 ≠ 1`). -/
public theorem T46_nonvacuous_seed_firewall_witness :
    isPracticeSeed 500 ∧ isTuningSeed 2500 ∧ isLockedSeed 5000 ∧
    PHILOX_DOM_WORLD_EXOG ≠ PHILOX_DOM_ENTITY_INT := by
  unfold isPracticeSeed isTuningSeed isLockedSeed PHILOX_DOM_WORLD_EXOG PHILOX_DOM_ENTITY_INT
  omega

/-- T47: Adversarial Negative-Boundary Refutation — Quotient rings GF(2)[x]/(x^8+p) are NOT
    integral domains for arbitrary `p`; reducible `0x01` has non-zero divisors `0x03 * 0xFF = 0`. -/
public theorem T47_refute_reducible_poly_integral_domain :
    ¬ (∀ p a b : BitVec 8, a ≠ 0#8 → b ≠ 0#8 → gf2MulMod8 a b p ≠ 0#8) := by
  intro hAll
  have hZero : gf2MulMod8 0x03#8 0xFF#8 0x01#8 = 0#8 := by decide
  have hNe3 : (0x03#8 : BitVec 8) ≠ 0#8 := by decide
  have hNeFF : (0xFF#8 : BitVec 8) ≠ 0#8 := by decide
  exact hAll 0x01#8 0x03#8 0xFF#8 hNe3 hNeFF hZero

/-- T48: Adversarial Negative-Boundary Refutation — Unmasked 32-bit tape heads can exceed `127`,
    and byte-masked `& 0x7F` qword accesses can overflow `128` (proving `& 0x78` is strictly required!). -/
public theorem T48_refute_unmasked_head_inbounds :
    ¬ (∀ h : BitVec 32, h.toNat < 128) ∧
    ¬ (∀ h : BitVec 32, (h &&& 0x7F#32).toNat + 8 ≤ 128) := by
  constructor
  · intro hAll
    have h128 := hAll 128#32
    have hv : (128#32).toNat = 128 := rfl
    omega
  · intro hAll
    have h127 := hAll 127#32
    have hv : (127#32 &&& 0x7F#32).toNat = 127 := rfl
    omega

end Autophage

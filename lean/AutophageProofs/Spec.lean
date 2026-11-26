module

/-!
# AutophageProofs.Spec — Pure Shared Mathematical Definitions Only (Zero Proofs, Zero `sorry`)

Canonical mathematical and hardware-model definitions shared identically by BOTH:
  - `Challenge.lean` (isolated statement-only specification — imports ONLY this module, ZERO proofs)
  - `AutophageProofs.lean` & `Solution.lean` (48 sorry-free Pure Lean 4.35.0 theorems verified across 6 kernels)

Zero Mathlib, zero `sorry`, zero `native_decide`, zero external axioms.
-/

namespace Autophage

/-! ## Part I Definitions: F_2^(8x8) Ribosome Algebra (`VGF2P8AFFINEQB`) -/

@[expose] public def parity8 (v : BitVec 8) : Bool :=
  let v1 := v ^^^ (v >>> 4)
  let v2 := v1 ^^^ (v1 >>> 2)
  let v3 := v2 ^^^ (v2 >>> 1)
  (v3 &&& 1#8) == 1#8

@[expose] public def gfniRow (mat : BitVec 64) (i : Nat) : BitVec 8 :=
  (mat >>> ((7 - i) * 8)).truncate 8

@[expose] public def gfniApply (mat : BitVec 64) (x : BitVec 8) (bias : BitVec 8) : BitVec 8 :=
  let b0 : BitVec 8 := if parity8 (gfniRow mat 0 &&& x) then 0x01#8 else 0#8
  let b1 : BitVec 8 := if parity8 (gfniRow mat 1 &&& x) then 0x02#8 else 0#8
  let b2 : BitVec 8 := if parity8 (gfniRow mat 2 &&& x) then 0x04#8 else 0#8
  let b3 : BitVec 8 := if parity8 (gfniRow mat 3 &&& x) then 0x08#8 else 0#8
  let b4 : BitVec 8 := if parity8 (gfniRow mat 4 &&& x) then 0x10#8 else 0#8
  let b5 : BitVec 8 := if parity8 (gfniRow mat 5 &&& x) then 0x20#8 else 0#8
  let b6 : BitVec 8 := if parity8 (gfniRow mat 6 &&& x) then 0x40#8 else 0#8
  let b7 : BitVec 8 := if parity8 (gfniRow mat 7 &&& x) then 0x80#8 else 0#8
  (b0 ||| b1 ||| b2 ||| b3 ||| b4 ||| b5 ||| b6 ||| b7) ^^^ bias

@[expose] public def gfniIdentity : BitVec 64 := 0x0102040810204080#64

/-- Non-self-inverse asymmetric invertible F_2 ribosome matrix M (3-cycle + shear)
    and its distinct exact Galois inverse M^-1. -/
@[expose] public def asymRibosomeMat : BitVec 64 := 0x0204031810204080#64
@[expose] public def asymRibosomeInv : BitVec 64 := 0x0501021810204080#64

/-- Pack 8 byte rows back into Intel `GF2P8AFFINEQB` 64-bit matrix layout (row i at byte 7 - i). -/
@[expose] public def packRows (f : Fin 8 → BitVec 8) : BitVec 64 :=
  ((f 0).setWidth 64 <<< 56) |||
  ((f 1).setWidth 64 <<< 48) |||
  ((f 2).setWidth 64 <<< 40) |||
  ((f 3).setWidth 64 <<< 32) |||
  ((f 4).setWidth 64 <<< 24) |||
  ((f 5).setWidth 64 <<< 16) |||
  ((f 6).setWidth 64 <<< 8)  |||
  ((f 7).setWidth 64)

@[expose] public def unpackRows (m : BitVec 64) : Fin 8 → BitVec 8 :=
  fun i => gfniRow m i.val

/-- Universal elementary row swap on ANY two rows `i, j : Fin 8` of an F_2^(8x8) ribosome matrix. -/
@[expose] public def swapRows (i j : Fin 8) (m : BitVec 64) : BitVec 64 :=
  packRows fun r =>
    if r = i then unpackRows m j
    else if r = j then unpackRows m i
    else unpackRows m r

/-- Universal elementary row shear on ANY distinct rows `i, j : Fin 8` (`row_i <- row_i XOR row_j`). -/
@[expose] public def shearRows (i j : Fin 8) (m : BitVec 64) : BitVec 64 :=
  packRows fun r =>
    if r = i then unpackRows m i ^^^ unpackRows m j
    else unpackRows m r

/-! ## Part II Definitions: GF(2)[x] Carry-Less Polynomial Ring (`PCLMULQDQ`), 30 Fields & Cauchy ECC -/

@[expose] public def clmul8 (a b : BitVec 8) : BitVec 16 :=
  let a16 := a.zeroExtend 16
  let t0 := if (b &&& 0x01#8) != 0#8 then a16 else 0#16
  let t1 := if (b &&& 0x02#8) != 0#8 then a16 <<< 1 else 0#16
  let t2 := if (b &&& 0x04#8) != 0#8 then a16 <<< 2 else 0#16
  let t3 := if (b &&& 0x08#8) != 0#8 then a16 <<< 3 else 0#16
  let t4 := if (b &&& 0x10#8) != 0#8 then a16 <<< 4 else 0#16
  let t5 := if (b &&& 0x20#8) != 0#8 then a16 <<< 5 else 0#16
  let t6 := if (b &&& 0x40#8) != 0#8 then a16 <<< 6 else 0#16
  let t7 := if (b &&& 0x80#8) != 0#8 then a16 <<< 7 else 0#16
  t0 ^^^ t1 ^^^ t2 ^^^ t3 ^^^ t4 ^^^ t5 ^^^ t6 ^^^ t7

@[expose] public def polyReduceStep (r modShifted mask : BitVec 16) : BitVec 16 :=
  if (r &&& mask) != 0#16 then r ^^^ modShifted else r

@[expose] public def polyReduceMod8 (prod : BitVec 16) (polyLow8 : BitVec 8) : BitVec 8 :=
  let mod16 : BitVec 16 := 0x0100#16 ||| polyLow8.zeroExtend 16
  let r7 := polyReduceStep prod (mod16 <<< 7) 0x8000#16
  let r6 := polyReduceStep r7   (mod16 <<< 6) 0x4000#16
  let r5 := polyReduceStep r6   (mod16 <<< 5) 0x2000#16
  let r4 := polyReduceStep r5   (mod16 <<< 4) 0x1000#16
  let r3 := polyReduceStep r4   (mod16 <<< 3) 0x0800#16
  let r2 := polyReduceStep r3   (mod16 <<< 2) 0x0400#16
  let r1 := polyReduceStep r2   (mod16 <<< 1) 0x0200#16
  let r0 := polyReduceStep r1   mod16         0x0100#16
  r0.truncate 8

@[expose] public def gf2MulMod8 (a b polyLow8 : BitVec 8) : BitVec 8 :=
  polyReduceMod8 (clmul8 a b) polyLow8

/-- Fast unrolled BitVec 16 remainder of degree-8 polynomial `p` divided by degree `degM` divisor `d`. -/
@[expose] public def polyDividesDeg (p d : BitVec 16) (degM : Nat) : Bool :=
  let r8 := if degM ≤ 8 && (p  &&& 0x0100#16) != 0#16 then p  ^^^ (d <<< (8 - degM)) else p
  let r7 := if degM ≤ 7 && (r8 &&& 0x0080#16) != 0#16 then r8 ^^^ (d <<< (7 - degM)) else r8
  let r6 := if degM ≤ 6 && (r7 &&& 0x0040#16) != 0#16 then r7 ^^^ (d <<< (6 - degM)) else r7
  let r5 := if degM ≤ 5 && (r6 &&& 0x0020#16) != 0#16 then r6 ^^^ (d <<< (5 - degM)) else r6
  let r4 := if degM ≤ 4 && (r5 &&& 0x0010#16) != 0#16 then r5 ^^^ (d <<< (4 - degM)) else r5
  let r3 := if degM ≤ 3 && (r4 &&& 0x0008#16) != 0#16 then r4 ^^^ (d <<< (3 - degM)) else r4
  let r2 := if degM ≤ 2 && (r3 &&& 0x0004#16) != 0#16 then r3 ^^^ (d <<< (2 - degM)) else r3
  let r1 := if degM ≤ 1 && (r2 &&& 0x0002#16) != 0#16 then r2 ^^^ (d <<< (1 - degM)) else r2
  r1 == 0#16

@[expose] public def polyDeg16 (d : BitVec 16) : Nat :=
  if (d &&& 0x0010#16) != 0#16 then 4
  else if (d &&& 0x0008#16) != 0#16 then 3
  else if (d &&& 0x0004#16) != 0#16 then 2
  else if (d &&& 0x0002#16) != 0#16 then 1
  else 0

/-- Full trial-division irreducibility test matching C11 `gf2_poly8_is_irreducible` literally
    (tests EVERY divisor `d = 2..31` of degree 1..4). -/
@[expose] public def isIrreduciblePoly8Full (low8 : BitVec 8) : Bool :=
  let p : BitVec 16 := 0x0100#16 ||| low8.zeroExtend 16
  if (low8 &&& 1#8) == 0#8 then false
  else if parity8 low8 then false
  else
    let rec loop (d : Nat) : Bool :=
      match d with
      | 0 | 1 => true
      | k + 1 =>
        let dv : BitVec 16 := BitVec.ofNat 16 (k + 1)
        if polyDividesDeg p dv (polyDeg16 dv) then false
        else loop k
    loop 31

@[expose] public def isIrreduciblePoly8 (low8 : BitVec 8) : Bool :=
  let p : BitVec 16 := 0x0100#16 ||| low8.zeroExtend 16
  if (low8 &&& 1#8) == 0#8 then false
  else if parity8 low8 then false
  else
    !(polyDividesDeg p 0x0007#16 2 ||
      polyDividesDeg p 0x000B#16 3 ||
      polyDividesDeg p 0x000D#16 3 ||
      polyDividesDeg p 0x0013#16 4 ||
      polyDividesDeg p 0x0019#16 4 ||
      polyDividesDeg p 0x001F#16 4)

/-- Exact literal C11 `GF2_IRRED_POLY8[30]` table from `src/galois_math.c`. -/
@[expose] public def c11IrredPoly8Table : List (BitVec 8) :=
  [ 0x1B#8, 0x1D#8, 0x2B#8, 0x2D#8, 0x39#8, 0x3F#8, 0x4D#8, 0x5F#8, 0x63#8, 0x65#8,
    0x69#8, 0x71#8, 0x77#8, 0x7B#8, 0x87#8, 0x8B#8, 0x8D#8, 0x9F#8, 0xA3#8, 0xA9#8,
    0xB1#8, 0xBD#8, 0xC3#8, 0xCF#8, 0xD7#8, 0xDD#8, 0xE7#8, 0xF3#8, 0xF5#8, 0xF9#8 ]

@[expose] public def countIrredBlock16 (base : Nat) : Nat :=
  let rec go (j : Nat) (acc : Nat) : Nat :=
    match j with
    | 0 => acc
    | k + 1 => go k (if isIrreduciblePoly8 (BitVec.ofNat 8 (base + k)) then acc + 1 else acc)
  go 16 0

@[expose] public def countIrredPoly8 : Nat :=
  let rec go (hi : Nat) (acc : Nat) : Nat :=
    match hi with
    | 0 => acc
    | k + 1 => go k (acc + countIrredBlock16 (k * 16))
  go 16 0

@[expose] public def countReduciblePoly8 : Nat := 256 - countIrredPoly8

@[expose] public def cauchyBlock16 (base : Nat) : Bool :=
  let rec go (j : Nat) : Bool :=
    match j with
    | 0 => true
    | k + 1 =>
      let p := BitVec.ofNat 8 (base + k)
      (!isIrreduciblePoly8 p || ((0x01#8 ^^^ gf2MulMod8 0x03#8 0x05#8 p) != 0#8)) && go k
  go 16

@[expose] public def all30CauchyDetNonzero : Bool :=
  let rec go (hi : Nat) : Bool :=
    match hi with
    | 0 => true
    | k + 1 => cauchyBlock16 (k * 16) && go k
  go 16

/-- Exact multiplicative inverse of Cauchy determinant `det = 1 ^ (3 * 5) = 0x0E` in every
    one of the 30 irreducible finite fields GF(2^8) / (x^8 + p). -/
@[expose] public def cauchyDetInv (p : BitVec 8) : BitVec 8 :=
  match p with
  | 0x1B#8 => 0xE5#8
  | 0x1D#8 => 0x5D#8
  | 0x2B#8 => 0x52#8
  | 0x2D#8 => 0xF4#8
  | 0x39#8 => 0x57#8
  | 0x3F#8 => 0xFB#8
  | 0x4D#8 => 0x3A#8
  | 0x5F#8 => 0x39#8
  | 0x63#8 => 0x46#8
  | 0x65#8 => 0xC8#8
  | 0x69#8 => 0x3C#8
  | 0x71#8 => 0x43#8
  | 0x77#8 => 0xC7#8
  | 0x7B#8 => 0x3F#8
  | 0x87#8 => 0x2D#8
  | 0x8B#8 => 0x9D#8
  | 0x8D#8 => 0x75#8
  | 0x9F#8 => 0x70#8
  | 0xA3#8 => 0x2B#8
  | 0xA9#8 => 0x7F#8
  | 0xB1#8 => 0x28#8
  | 0xBD#8 => 0x8C#8
  | 0xC3#8 => 0xA1#8
  | 0xCF#8 => 0x21#8
  | 0xD7#8 => 0x64#8
  | 0xDD#8 => 0x22#8
  | 0xE7#8 => 0xBF#8
  | 0xF3#8 => 0x6E#8
  | 0xF5#8 => 0xB0#8
  | 0xF9#8 => 0x24#8
  | _      => 0#8

@[expose] public def cauchyRecLo (p lo hi : BitVec 8) : BitVec 8 :=
  let p0 := lo ^^^ gf2MulMod8 hi 0x03#8 p
  let p1 := gf2MulMod8 lo 0x05#8 p ^^^ hi
  gf2MulMod8 (cauchyDetInv p) (p0 ^^^ gf2MulMod8 0x03#8 p1 p) p

@[expose] public def cauchyRecHi (p lo hi : BitVec 8) : BitVec 8 :=
  let p0 := lo ^^^ gf2MulMod8 hi 0x03#8 p
  let p1 := gf2MulMod8 lo 0x05#8 p ^^^ hi
  gf2MulMod8 (cauchyDetInv p) (gf2MulMod8 0x05#8 p0 p ^^^ p1) p

/-! ## Part III Definitions: Symplectic Reversible Silicon Gates (`TOFFOLI`, `FREDKIN`), `popcnt64` -/

@[expose] public def toffoli64 (d s t : BitVec 64) : BitVec 64 × BitVec 64 × BitVec 64 :=
  (d ^^^ (s &&& t), s, t)

@[expose] public def fredkin64 (ctrl s t : BitVec 64) : BitVec 64 × BitVec 64 × BitVec 64 :=
  let mask := (s ^^^ t) &&& ctrl
  (ctrl, s ^^^ mask, t ^^^ mask)

/-- Exact Hamming weight (hardware `POPCNT`) on 64-bit registers. -/
@[expose] public def popcntUpTo (x : BitVec 64) : Nat → Nat
  | 0 => 0
  | n + 1 => popcntUpTo x n + (if x.getLsbD n then 1 else 0)

@[expose] public def popcnt64 (x : BitVec 64) : Nat := popcntUpTo x 64

/-! ## Part IV Definitions: 2-Adic Ultrametric Centromere Geometry (`TZCNT`) -/

/-- Exact 32-bit trailing-zero count matching x86-64 `TZCNT` (`0` maps to `32`). -/
@[expose] public def tzcntFrom (x : BitVec 32) (i : Nat) : Nat → Nat
  | 0 => 0
  | rem + 1 => if x.getLsbD i then 0 else 1 + tzcntFrom x (i + 1) rem

@[expose] public def tzcnt32 (x : BitVec 32) : Nat := tzcntFrom x 0 32

/-! ## Part V Definitions: 5-Pool Exact Integer Thermodynamic Ledger State -/

public structure LedgerState where
  e_cell     : Int
  e_membrane : Int
  e_detritus : Int
  e_heat_loc : Int
  e_heat_snk : Int
  e_inflow   : Int
  e_export   : Int

@[expose] public def LedgerState.residual (s : LedgerState) (e_init : Int) : Int :=
  (s.e_cell + s.e_membrane + s.e_detritus + s.e_heat_loc + s.e_heat_snk + s.e_export) -
  (e_init + s.e_inflow)

public inductive TransferOp where
  | detritusToCell     (amt : Int)
  | cellToHeat         (amt : Int)
  | cellToDetritus     (amt : Int)
  | cellToMembrane     (amt : Int)
  | membraneToDetritus (amt : Int)
  | heatToSink         (amt : Int)
  | externalInflow     (amt : Int)
  | externalExport     (amt : Int)

@[expose] public def applyTransfer (s : LedgerState) : TransferOp → LedgerState
  | .detritusToCell a     => { s with e_detritus := s.e_detritus - a, e_cell := s.e_cell + a }
  | .cellToHeat a         => { s with e_cell := s.e_cell - a, e_heat_loc := s.e_heat_loc + a }
  | .cellToDetritus a     => { s with e_cell := s.e_cell - a, e_detritus := s.e_detritus + a }
  | .cellToMembrane a     => { s with e_cell := s.e_cell - a, e_membrane := s.e_membrane + a }
  | .membraneToDetritus a => { s with e_membrane := s.e_membrane - a, e_detritus := s.e_detritus + a }
  | .heatToSink a         => { s with e_heat_loc := s.e_heat_loc - a, e_heat_snk := s.e_heat_snk + a }
  | .externalInflow a     => { s with e_detritus := s.e_detritus + a, e_inflow := s.e_inflow + a }
  | .externalExport a     => { s with e_detritus := s.e_detritus - a, e_export := s.e_export + a }

@[expose] public def applyTransfers (s : LedgerState) (ops : List TransferOp) : LedgerState :=
  ops.foldl applyTransfer s

/-! ## Part VI Definitions: Philox Domain Isolation, 3-Tier Seed Firewall, OEE Archive & Weismann -/

public structure PhiloxCounterKey where
  ver  : Nat
  seed : Nat
  dom  : Nat
  ent  : Nat
  org  : Nat
  tick : Nat
  purp : Nat
  idx  : Nat
  deriving DecidableEq

@[expose] public def PHILOX_DOM_WORLD_EXOG : Nat := 0
@[expose] public def PHILOX_DOM_ENTITY_INT : Nat := 1

@[expose] public def isPracticeSeed (s : Nat) : Prop := 100 ≤ s ∧ s ≤ 999
@[expose] public def isTuningSeed   (s : Nat) : Prop := 2000 ≤ s ∧ s ≤ 2999
@[expose] public def isLockedSeed   (s : Nat) : Prop := 5000 ≤ s

public structure OeeArchive where
  entries    : List Nat
  cumNovelty : Nat

@[expose] public def OeeArchive.appendNovel (a : OeeArchive) (lineageId : Nat) : OeeArchive :=
  { entries := lineageId :: a.entries, cumNovelty := a.cumNovelty + 1 }

@[expose] public def OeeArchive.appendNovelBatch (a : OeeArchive) (novelIds : List Nat) : OeeArchive :=
  novelIds.foldl OeeArchive.appendNovel a

public structure WeismannState where
  germline : BitVec 64
  soma     : BitVec 64

@[expose] public def perturbSomaOnly (st : WeismannState) (mask : BitVec 64) : WeismannState :=
  { st with soma := st.soma ^^^ mask }

@[expose] public def perturbSomaTrace (st : WeismannState) (masks : List (BitVec 64)) : WeismannState :=
  masks.foldl perturbSomaOnly st

end Autophage

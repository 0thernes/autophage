module

public import AutophageProofs
public import AdversarialTests

/-!
# AUTOPHAGE v1.5.0 Formal Solution Entrypoint (`Solution.lean`)

Exports all 56 machine-checked Pure Lean 4.35.0 theorems from `AutophageProofs` (48 core,
non-vacuity, and boundary theorems) and `AdversarialTests` (8 explicit false-variant refutations
`AT01`–`AT08`) with zero Mathlib, zero `sorry`, zero `native_decide`, zero `bv_decide`,
`autoImplicit = false`, and `relaxedAutoImplicit = false`, for isolated 6-kernel judging against
`Challenge.lean` via:
    lake comparator --config comparator.json --paranoid
-/

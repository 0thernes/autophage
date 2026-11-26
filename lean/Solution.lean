module

public import AutophageProofs

/-!
# AUTOPHAGE v1.4.0 Formal Solution Entrypoint (`Solution.lean`)

Exports all 48 machine-checked Pure Lean 4.35.0 theorems from `AutophageProofs`
(zero Mathlib, zero `sorry`, zero `native_decide`, zero `bv_decide`, `autoImplicit = false`,
`relaxedAutoImplicit = false`) for isolated 6-kernel judging against `Challenge.lean` via:
    lake comparator --config comparator.json --paranoid
-/

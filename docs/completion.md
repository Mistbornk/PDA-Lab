# Remaining AGENTS work: implementation record

Baseline: commit `89a0239`. Earlier evidence is retained as historical measurement.
This sequence adds cohesive commits without rewriting the already-pushed history.

## 1. Ownership and module boundaries

- Lab02 owns its compatibility random generator; tree mutations receive it explicitly.
  CPU budgets are per thread on Linux. See [randomness](randomness.md).
- Lab01 separates `parser.cpp`, `layout.cpp`, `report.cpp` and the CLI.
- Lab04's legacy router lives in `legacy_router.cpp`, alongside the layered variant.
- Regression oracles, 700,000 PRNG sequence comparisons and eight concurrent solves
  protect behavior while removing the global-state restriction.

Validation: Release and ASan/UBSan CTest each pass all five groups. All three
official Lab02 cases pass at seed 1 / 300,000 iterations and match the prior
solution hashes exactly ([record](../benchmarks/results/owned-random.json)).

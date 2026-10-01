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

## 2. Lab01 spatial candidate selection

The profile after edge-bucket indexing spent 91.86% of sampled time in
`Block_Creating` on the 3,600-block grid. Insertion still scanned all tiles to
reject solid overlap and find the top/bottom split space. A separate R-tree now
selects candidates for these three queries. All ownership mutations update both
indexes; geometry is immutable while registered. Integer half-open predicates
filter inclusive R-tree intersections, and original insertion order resolves ties.
`--geometry scan` retains the reference path independently of `--stitches`.

On 10,000 isolated blocks, median wall time is **5.82 s scan → 0.52 s spatial**
(11.19×, three serial runs). Output matches an independently known grid answer.
The 400-seed/mode combinations check every live stitch, neighbor set, point query
and overlap result against geometry/raster oracles. See
[raw measurements](../benchmarks/results/lab1-geometry.json). Small cases may not
benefit from the extra index; R-tree queries and edge buckets still have linear
worst cases. This is candidate selection, not a new corner-stitching algorithm.

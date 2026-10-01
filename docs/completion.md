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

## 3. Lab02 coordinate-independent packing and incremental rollback

The measured contour hotspot now uses a reusable vector skyline. Trial rollback
records touched nodes/rotations instead of copying every macro and name. Dense
packing and complete snapshots remain selectable references. All 36 paired
official runs are legal; all nine fixed-work groups produce identical solutions.
See [design, complexity and measurements](floorplanning-engineering.md).

## 4. Solver-internal parallel independent restarts

Lab02 adds a bounded worker pool over independent seed trajectories. Ownership
and disjoint result slots remove shared mutable optimizer state; futures join
before deterministic legal-result selection. Failure and seed-range handling are
tested. Eight fixed-work searches take median 21.128 / 10.761 / 5.488 / 3.040 s
at 1 / 2 / 4 / 8 threads (6.95× at eight). All 12 official outputs and each seed's
outcomes agree. See [parallelism design and raw evidence](parallel-floorplanning.md).

## 5. Strict compiler gates and generated scale tests

GCC 11.4 and Clang 14 build with conversion/shadow warnings and `-Werror` enabled.
Signed IDs remain signed while carrying the -1 sentinel; checked access boundaries
convert only valid IDs to vector indices. Grid neighbor arithmetic similarly stays
signed until bounds checks. Shadowed locals and implicit narrowing were removed.
CI now enforces these diagnostics for Release and sanitizer builds.

The new stress suite independently checks known corner-stitch grids, pairwise
floorplan overlap/HPWL, each banking step's occupied sites, and analytically known
shortest routing costs. Its smoke size runs by default in CTest; large mode covers
10,000 inserted rectangles, 499 macros on a 2-billion-coordinate outline,
200,000 cells / 5,000 banking steps, and a 1,000 × 600 routing grid. All pass.
Stress timing records are validation evidence, not controlled speedup comparisons.

## 6. Broader equal-time quality evidence

Ten seeds × three cases × two CPU budgets × two implementations produce 120
attempts. All 75 legal solutions pass the official verifier; 45 expected budget
exhaustions remain in the record. Baseline/optimized feasibility is 31/60 versus
44/60. Paired legal costs never regress in this sample, but this is not a general
quality guarantee. See [per-dataset results and timing-policy caveat](quality-study.md).

## 7. Verification, documentation and coherent history

The new work is split into module/random-state ownership, Lab01 geometry indexing,
Lab02 skyline/journal, internal parallel restarts, strict diagnostics/stress tests,
and experiment/documentation commits. The earlier published history is retained.
The README, architecture, per-lab test instructions and CI documentation now describe
the current behavior; historical measurements are explicitly labeled.

The full [remote run](https://github.com/Mistbornk/PDA-Lab/actions/runs/36856085884)
at `7800201` passes GCC Release, Clang Release, GCC ASan/UBSan, all 16 official
inputs, all four layered-routing inputs and generated large workloads. A later
small CLI fix rejects whitespace-prefixed negative iteration budgets (previously
accepted by `stoull` as an enormous unsigned value), with regression coverage for
malformed and out-of-range new search options.

## Mapping back to the requested gaps

| Previously open item | Resolution / evidence |
|---|---|
| Lab02 process-global random state | Owned PRNG; 700,000 sequence checks, concurrent solves |
| Lab01 / Lab04 module boundaries | Parsing/solving/reporting APIs; dedicated legacy routing module |
| Remaining measured scans / dense contour / full trial copies | R-tree candidates, skyline and undo journal; controlled ablations |
| Solver-internal parallelism | Independent multi-start; race analysis and 1/2/4/8-thread scaling |
| Three-seed-only / no equal-time or scale study | Ten seeds, 120 timed runs, generated large cases in all labs |
| Strict warning cleanup | GCC/Clang conversion + shadow diagnostics with -Werror in CI |
| Stale CI and implementation documentation | Current docs plus linked remote run and JSON evidence |
| One monolithic implementation commit | New work committed in coherent stages; no published-history rewrite |

The example directory tree and choice of threading library are optional directions;
the implementation keeps distinct lab domains and uses standard C++ tasks. Global
optimality, moving existing Lab03 cells, and further HPWL/packing improvements remain
algorithm research topics. Their limits are documented in the README.

Final-source checksums and test logs: [completion-validation.json](completion-validation.json).

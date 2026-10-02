# Interview roadmap implementation record

Baseline: `616d52e`; scope and acceptance criteria: [roadmap](interview-roadmap.md).
This log distinguishes implemented features from pending experiments.

## M1 — Core sessions and structured results

- All four algorithms now have separate CMake core libraries. Public Lab03 / Lab04
  model names live in their own namespaces and can be included in the same client.
- Lab03 exposes `Legalizer::apply`, `StepResult`, `Stats`, a live-cell snapshot and
  invariant validation. A banking request is planned with an exclusion set before
  committing cell/index changes. Invalid requests and no-space failures preserve
  live placement and row tie order. An allocation/internal failure during commit
  poisons the session: discard it rather than retrying on potentially partial state.
- Lab04 exposes structured routes, independently callable formatting, aggregate
  metrics and a `RoutingState` with validated replacement and reversible removal.
  Replacement allocates/checks its edge deltas before changing usage. The borrowed
  problem must outlive the session; constructing from an rvalue is forbidden.
- Both routing algorithms return paths; output no longer modifies capacity. The
  legacy course CLI remains available. Lab03 can consume step results incrementally.
- GCC Release with strict warnings passes 9/9 CTest groups, including a new test
  that links both libraries and exercises failed banking, 1,000 remove/reinsert
  operations, invalid route replacement, known aggregate costs and pure reporting.

Official before/after comparisons are recorded under `benchmarks/results/interview-m1-*`.
All five Lab03 and all four Lab04 official cases are legal and byte-identical to
the baseline. One-pass timings exposed overhead from the initial hash exclusion
filter; a generation array replaces its hot predicate, with a paired ablation. The new result/transaction APIs have storage and
validation costs which must be measured explicitly.

## M2 — Input and concurrency diagnostics

Four stream parser fuzz targets ran 20,000 requested mutations each with fixed
seed, size/time/RSS bounds and ASan/UBSan instrumentation; all completed without
findings. Typed InputError rejection is caught, while unexpected exceptions remain
visible. Mutable corpora and crash artifacts live outside tracked seed inputs.
Clang TSan passes both parallel-search and private-RNG groups, including expected
failure paths. Clang Static Analyzer checked 24 production translation units;
three dead stores were removed, and the rerun reports zero diagnostics.

GCC Release, Clang Release and GCC ASan/UBSan pass 9/9 groups. Translation invariance
and transaction failure tests supplement the existing independent oracles. CI now
has bounded fuzz/static-analysis and TSan jobs. Exact checkpoint records are in
[interview-m2-validation.json](interview-m2-validation.json).

## M3 — Budgeted rerouting

Structured routes now support optional negotiated rerouting, congestion ordering,
history=0 ablation, round/stagnation/time budgets and best complete-result retention.
Independent path/usage/objective oracles and official checks pass. All 72 official
equal-budget runs are legal but show no cost gain and additional runtime. Thirty-six
generated congestion runs are legal; simple rerouting improves original cost by
1.58–6.81%, with more search time. History=1 is weaker on these cases. A rejected
generator text format is preserved as a failed experiment, not hidden or counted
as an algorithm win. See [design record](decisions/002-negotiated-routing.md).

## M4 — Search policies and diagnostic ablation

Iteration-driven progress and feasibility-first policies are optional. Diagnostics
include first legal time, outline excess, acceptance, sampled packing/HPWL times
and bounded best-cost traces. Fixed-work tests preserve outcomes with diagnostics,
packing/journal ablations and 1/2/4/8 threads. The 120-run held-out comparison finds
12/40 legal legacy attempts and 40/40 for each new policy; each new policy wins 8
and loses 4 among the 12 mutually legal pairs. In particular, vda317b quality
regresses. Legacy therefore remains the default. An instrumentation ablation
motivated sparse timer sampling, with identical fixed-seed solutions before/after.
See [design record](decisions/003-floorplanning-policies.md) and raw policy JSONs.

## M5 — Minimum displacement and bounded repair

All 45 official and 54 generated runs pass the course evaluator; Move Times,
Total Distance and Total agree with internal metrics. Legacy outputs remain
byte-identical to the baseline on all five cases. Minimum reduces official total
cost by 22.3–84.1%, while median runtime grows by 1.54–13.42× and RSS remains similar.
Repair at the declared 20-unit radius matches minimum on official cases; generated
cases exercise actual moves and expose both sequence improvements and regressions.
The exhaustive-site oracle covers 200 small placements, fractional coordinates,
multi-row cells, FIX invariance, failed planning and score/name-lifetime semantics.
See [design record](decisions/004-bounded-legalization.md) and the complete legalizer
study JSONs. This is a quality/runtime option, not a claim of faster legalization.

## M6 — Final verification

The offline SVG/HTML demo independently checks emitted geometry and original cost.
Its routing case has 24 nets and 20 overflow units, which the view can inspect by
net or congestion layer. The English brief, reproduction guide, four design records
and unified report cover 336 attempts with CSV/JSON, range tables and three SVG
figures. All generated SVGs were visually inspected; RGB colors keep them portable
across browser and standalone renderers.

GCC/Clang strict Release and GCC ASan/UBSan each pass all 12 CTest groups. Clang TSan
passes all three concurrency groups. Four parser fuzz campaigns pass; Static
Analyzer checks 26 production units with zero diagnostics. All 66 pinned resources
remain unchanged. Lab02 fixed-work solution hashes and all four Lab04 legacy output
hashes match their baselines. A separate official evaluator check confirms the
repair example's two moves, zero final distance and total cost 2.

An additional [Lab01 distribution study](lab1-capacity.md) passes 72 comparisons of
dense, strip, shared-boundary and fragmented layouts. It records all three sizes,
RSS, tile counts and candidate visits; no speculative arena rewrite was added.
Clean-checkout and remote CI verification are the final pending checks.

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

## Pending milestones

 M3: negotiated rerouting.
M4: floorplanning policy/quality experiments. M5: displacement and bounded repair.
M6: reproducible demo, combined reports and interview documentation.

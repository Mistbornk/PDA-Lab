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
Performance observations and the completed integration outcome will be added after
the serial experiment finishes. The new result/transaction APIs have storage and
validation costs which must be measured explicitly.

## Pending milestones

M2: fuzzing, TSan and expanded state invariants. M3: negotiated rerouting.
M4: floorplanning policy/quality experiments. M5: displacement and bounded repair.
M6: reproducible demo, combined reports and interview documentation.

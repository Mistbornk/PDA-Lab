# Structured results and transaction boundaries

Status: implemented in M1; measured against `616d52e`.

Lab03 previously removed banked cells before knowing whether the replacement could
be placed. Lab04 coupled path formatting with edge-usage updates. A reusable API
needs explicit behavior after a failed operation and when a result is inspected.

The selected boundary is a per-domain session over a validated model. Lab03 returns
one `StepResult` at a time. Lab04 stores routes with an ordinal `NetId`; grid search
states encode cell and arrival layer. Reporting only consumes values. Separate
namespaces let one client include both public APIs without model-name collisions.

Copying the whole placement/index before every banking step would provide simple
rollback but scale with all cells. Instead, queries temporarily ignore the banked
IDs, and commit begins only after finding a valid result. A compact generation
array implements the hot exclusion predicate; a small ID set detects duplicates
and enumerates the commit. The generation wrap path clears the array explicitly.
Expected input/no-space failures leave cells, indexes and row tie order unchanged;
diagnostic counters may increase. Allocation or internal invariant failures during
commit poison the session, and the caller must discard it. This is a stated basic
failure guarantee, not a claim of strong exception safety under out-of-memory.

For routing, `replace` validates the entire path and builds/checks its usage delta
before updating the stored route. Removing and reinserting a route must preserve
usage exactly. A reconstruction checker derives usage from stored paths, while
the separate Python Dijkstra oracle checks the search cost model.

The cost is explicit: storing routes needs O(total path length) memory; checking a
replacement needs temporary O(affected edges) storage. Lab03's generation array
needs O(initial cells + inserted cells) space. The original algorithms remain
selectable, and course-format compatibility is verified separately from API tests.

The first official comparison found identical outputs on all five Lab03 and four
Lab04 datasets. The initial hash exclusion filter slowed some Lab03 workloads;
the paired filter ablation records the subsequent generation-array change. See
`benchmarks/results/interview-m1-*` and `interview-session-ablation.json` once present.

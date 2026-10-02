# Negotiated rerouting under the original objective

Status: implemented as the optional `--router negotiated` strategy.

The layered router minimizes one net's incremental cost while other routes are
fixed. Sequential one-pass routing can leave poor shared-capacity decisions. The
new strategy revisits all nets, prioritizing those touching the most overflow;
ordinal net ID resolves ties. Each round ranks the current complete solution once.

Search virtually subtracts the selected net's old edge usage. It does not remove
the stored route first: a deadline or search exception leaves a complete routing
state. A successful candidate is submitted to `RoutingState::replace`, whose delta
is validated before mutation. This keeps the inherently dependent net loop serial.

Two variants are available for ablation. `--history 0` performs coordinate-style
rerouting using the original incremental objective. Positive history adds a
nonnegative per-edge penalty after each round's overflow measurement. The penalty
unit is the larger of one minimum-pitch wire step and one weighted overflow unit,
scaled by `--history`. Weighted Manhattan remains an admissible consistent lower
bound because these extra edge costs are nonnegative.

History is a search heuristic, not the reported objective. Every complete round
(including a partial round stopped by budget) is scored from all paths and actual
aggregate overflow. The best original-objective complete solution is retained,
starting with the one-pass layered result. This gives a best-so-far guarantee for
that run, not global optimality or guaranteed improvement under another time budget.
Overflow is penalized by the course objective; it is not automatically illegal.

`--seconds` is a cooperative wall budget for the routing solve, including initial
routing but excluding file parsing and reporting. Search checks every 256 expanded
states and at the start of a net. Allocation, scoring and reporting can add overhead
beyond that deadline; wrapper wall time is recorded separately. Expiry before the
first complete solution throws `RoutingBudgetExceeded` (CLI exit 3, no new report).
Later expiry returns the best complete solution with `budget_exhausted=true`.
Round and stagnation limits also bound work. Fixed-round runs are deterministic;
deadline-limited runs need not complete the same number of searches.

Costs: the grid workspace/history are O(V), stored paths O(total path length), and
one search roughly O((V+E) log V). A virtual exclusion map needs O(old path length)
space. Multiple rounds intentionally spend more search work; compare legal
original cost, overflow, wirelength, vias, elapsed time and RSS together.

Tests retain the independent four-state Dijkstra oracle for one-pass routing.
Generated rerouting tests independently reconstruct paths and total objective,
check deterministic repeated runs and verify best-so-far preservation. The official
equal-budget study compares layered, reroute without history and reroute with
history in rotating serial order; all failed attempts remain in its JSON.

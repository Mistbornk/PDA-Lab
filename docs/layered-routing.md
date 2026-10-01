# Layer-state routing model

`--router layered` is an optional Lab04 variant. `legacy` remains the default so
existing course-output comparisons remain available. Both route nets sequentially
in input order; earlier nets contribute edge usage to later searches. The variant
minimizes the next net's incremental objective on its graph, not the jointly
optimal routing of all nets.

## Why a separate layer state matters

The legacy router stores one distance per GCell and remembers the incoming layer
only on that winning entry. Two arrivals at the same location can have different
future turn costs, so discarding the more expensive arrival may discard the best
complete route. The new graph uses `(cell, arrival layer)`, two states per cell.
M1 moves vertically and M2 horizontally. Changing the departure layer places a
via at the current cell.

The cost contract follows the local
[Guide, Q5–Q7 and Q11](../Lab04/Lab4_Guide/IEE%20PDA_Lab4%20Die-to-Die%20Global%20Routing.md).
Every net starts and finishes on M1. Count each visited cell once: on a straight
pass, its layer cost; at a via, the average of its M1/M2 costs plus the via cost.
This includes both endpoint cells. A one-cell route pays that cell's M1 cost.

## Nonnegative transition costs

For a state at cell `u` with arrival layer `a`, departing on layer `d` towards an
adjacent cell `v` costs:

```text
cell(u,a,d) = gamma * (a == d ? C[d,u] : (C[M1,u]+C[M2,u])/2)
              + (a == d ? 0 : delta * via_cost)
edge(u,v,d) = alpha * physical_edge_length
              + beta * (max(0, usage+1-capacity) - max(0, usage-capacity)) * maxCellCost/2
transition  = cell(u,a,d) + edge(u,v,d)
```

The cell is charged on **departure**, when both layers are known. Charging the
arrival cell immediately and later replacing its cost at a via can introduce
negative corrections, which are unsuitable for ordinary Dijkstra/A*.

An explicit terminal sink charges the destination cell with departure M1, including
any final via. Reaching a destination GCell alone does not terminate the search.
Overflow uses the change in aggregate overflow from adding the current net;
charging the entire previously accumulated overflow would overcount it.

All input weights and costs are finite and nonnegative. The heuristic is
`alpha * Manhattan physical distance`. It is a lower bound; along any movement its
decrease is at most the weighted wirelength term. The other terms are nonnegative,
and the sink heuristic is zero. Thus it is consistent in exact arithmetic.
Priority-queue entries carry their `g` value and stale entries are discarded.
Ties are ordered deterministically by `(f,g,state ID)`; tests compare costs with
floating-point tolerances rather than claiming bitwise equality to another solver.

With V cells there are 2V+1 states and O(V) implicit edges; a binary heap gives
O(V log V) worst-case work per net under the usual nonnegative shortest-path model,
plus O(V) state initialization. Distances and predecessors are reused across nets.
The original router remains useful as a faster heuristic comparison on public data.

## Independent validation

`tests/lab4_oracle.py` constructs a separate **four-state** graph per cell:
entry/exit × layer. Internal edges charge the cell; movement edges charge physical
length and the direct before/after overflow difference. A Python Dijkstra oracle
has no Manhattan heuristic and no access to the C++ search internals.

80 generated grids × 5 sequential nets test:

- independent shortest cost and the cost recomputed from emitted paths;
- direction, continuity, endpoints, explicit vias, alignment and bounds;
- zero weights/costs, zero capacities and shared-edge congestion;
- nonzero origin, nonsquare pitch, one-row/column grids and same-cell endpoints;
- coordinate distances above INT_MAX on a tiny 3×3 grid.

Release and ASan/UBSan tests pass. All four public/toy inputs also pass the course
evaluator. The sum of diagnostic incremental costs agrees with its weighted total
to displayed rounding precision. Measurements, including slower runtimes and
quality improvements, are in [benchmarks](../benchmarks/README.md).

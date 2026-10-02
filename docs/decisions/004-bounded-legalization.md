# Minimum displacement and bounded transactional repair

Status: optional `minimum` and `repair` strategies; compatibility default unchanged.

Nearest-row order is not the same as minimum Manhattan displacement: a closer
row can require a much longer horizontal detour. `minimum` sorts rows by vertical
distance, projects intersecting R-tree obstacles onto that row's x axis, merges
occupied intervals, and tests the floor/ceil sites nearest the desired x in each
free gap. It stops only when the next row's vertical distance exceeds the best
full Manhattan distance. Ties use `(distance, y, x)`. With other cells fixed,
this enumerates the relevant nearest candidate in every feasible gap and finds
the minimum-distance insertion. It preserves fractional sites, nonzero origins
and cells spanning multiple rows.

For R rows and k_i intersecting obstacles in a visited row, the cost is roughly
O(R log R + sum(k_i log k_i)), in addition to R-tree traversal. The index is not a
worst-case logarithmic oracle. Candidate intervals are temporary; stable cell
slots grow with the initial placement plus banking steps.

`repair` begins with the minimum strategy's candidate. It explores at most
`repair_candidates` nearby insertion sites within Manhattan `repair_radius`,
rejecting FIX blockers and candidates requiring more than `repair_cells` moves.
Defaults are 16 sites, radius 20 coordinate units, and two movable cells. Temporary
candidate construction is also bounded by the candidate count per row; counts
are checked (at most 256 sites and eight blockers). Direct blockers are virtually
excluded, the inserted cell is staged as an obstacle, and blockers are relocated
within the same radius of their current positions. Two stable ID orders are tried,
not factorial permutations or recursive displacement chains.

Only a strict improvement in immediate official score replaces an existing
no-move candidate. An optimistic lower bound skips repairs that cannot improve
that score, even if all blockers returned to their original locations. This saves
unnecessary placement searches but does not guarantee bounded wall time independent
of placement size. A repair may also find a legal candidate when no insertion-only
gap exists. No geometry or live index changes before selection; expected failure
leaves placement unchanged. Commit allocation/internal failures poison the session
as described in [the transaction contract](001-state-transactions.md).

Official score is `alpha * cumulative move count + beta * total displacement`.
It is not cumulative distance travelled. The pinned evaluator retains the first
recorded cell object for each name, including after banking removes that cell.
The implementation retains stable score references to match that behavior; name
reuse does not replace an earlier record. `maximum_displacement_seen` is an
additional diagnostic over reported positions, not an official objective term.

Immediate improvement does not imply whole-sequence improvement. A repair changes
the obstacle map seen by later requests. The 90%-density, single-height generated
case scores 138 with minimum and 141 with repair; the corresponding double-height
case improves from 246 to 237. This counterexample rules out advertising repair as
a globally dominating policy. Both remain explicit options.

Tests compare minimum insertion against exhaustive site enumeration in 200 small
random placements, including FIX cells, half-unit sites and multiple heights.
Repair tests independently check pairwise overlap, unchanged FIX coordinates,
immediate score, no-space rollback and repeated movement back to the original
location. A two-step gap-creation example ends with two moves, zero final distance,
and cost 2. Official studies use all five public datasets and six generated
density/height distributions, retaining every attempt and independently checking
Move Times, Total Distance and Total against the downloaded evaluator.

CLI exit 3 means no legal placement was found by the selected bounded strategy;
it does not prove the entire problem infeasible. Because the CLI streams completed
steps, a failure can leave a valid prefix in the output file. Callers must check
the exit status before treating it as a complete solution. The structured core
allows callers to choose their own buffering/atomic publication policy.

The five official cases show 22.3–84.1% lower total cost for minimum versus legacy,
with additional runtime (especially the original filename-dispatched nearest-row
case). At the declared default radius of 20 physical coordinate units, repair
makes no extra official-case score improvement. It is not an adaptive DBU-scaled
window: use explicit radius/cell/candidate settings for another workload, and
re-measure both quality and cost. Generated cases and the gap-creation example
exercise actual moved-cell output.

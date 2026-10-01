# Lab02 packing and rollback engineering

## Algorithm and invariants

A B*-tree node is one rectangular macro. A left child starts at its parent's
right edge; a right child starts at the parent's x coordinate. In preorder, place
each macro at the maximum occupied height over its horizontal span and update
the contour to its new top edge. This constructs non-overlapping rectangles;
outline feasibility is a separate check on the resulting width and height.
Rotation swaps width/height, move detaches and reinserts a node, and swap exchanges
two node positions while repairing parents/children and the root.

The retained annealer first searches with outline overflow as its objective. It
then evaluates `alpha * bounding_box_area + (1-alpha) * total_net_HPWL`, preserving
the course implementation's acceptance and cooling rules. It records the best
legal placement; intermediate states need not all fit the outline. HPWL is each
net's x-span plus y-span over terminal coordinates and macro centers, using the
implementation's integer-center convention. Budget exhaustion can legitimately
produce no legal result. This is a heuristic, not a proof of a global optimum.

## Measured implementation changes

The `89a0239` gprof baseline spent 70.83% of ami49 sampled time updating the dense
coordinate contour. The new skyline stores `(x,height)` breakpoints and assigns
half-open intervals. Packing walks the B*-tree iteratively in the same preorder;
the workspace reuses its buffers. There are at most `2*n+1` breakpoints: memory is
O(n), independent of coordinate magnitude. Vector insertion/range scanning means
O(n²) worst-case packing, versus dense O(outline width + sum of macro widths).
Neither representation is universally faster; `--packing dense|skyline` keeps the
reference and measured alternative available.

After this change, vda317b's profile attributed 16.70% of samples to the annealer
outside packing, including per-trial copies. An undo journal now records each
changed node and rotation once per transaction. Names are immutable; x/y are
derived and repacked after rejection. Root, parent/child links and rotated sizes
are restored. A transaction can contain several perturbations in timed mode.
`--rollback snapshot|journal` supports controlled ablations. Journal work scales
with touched nodes (a move can still touch a full tree path), rather than always
copying every macro/string. Accepted best solutions still require a full copy.

## Evidence

The fixed-work experiment uses alpha 0.5, 300,000 iterations, seeds 1/7/19, all
three official datasets, four variants, serial execution and alternating variant
order. All **36 runs pass the official verifier**, and all nine groups have
identical solution hashes after removing the report's timing line.

| Dataset | Original 89a0239 | Dense / snapshot | Skyline / snapshot | Skyline / journal | Original / final |
|---|---:|---:|---:|---:|---:|
| ami33 | 2.222 s | 2.017 s | 1.486 s | 1.346 s | 1.65× |
| ami49 | 8.059 s | 7.473 s | 2.941 s | 2.709 s | 2.98× |
| vda317b | 19.370 s | 17.876 s | 6.631 s | 5.473 s | 3.54× |

These are medians across three paired seeds, not confidence intervals. The dense
reference also benefits from workspace reuse and iterative traversal, so compare
with the rebuilt original column for the full change. Memory and every command,
input/executable hash and verifier result are in the [raw record](../benchmarks/results/lab2-engineering-fixed.json).
Profiles are diagnostic samples; release measurements above support speed claims.
[Profiles](../benchmarks/results/gaps-profile.json) show the remaining hotspot
varies by case: HPWL dominates ami49, whereas packing dominates vda317b. Incremental
HPWL is not assumed safe: one tree mutation can shift many block centers.

The core test checks both packers against an independent rectangle oracle after
20,000 tree mutations; a coordinate-scale test uses an outline of 2 billion.
A separate test compares 10,000 multi-mutation undo transactions with full snapshots,
including committed trajectories and subsequent rollbacks. ASan/UBSan are included.

```bash
python3 benchmarks/build_revision.py 89a0239
python3 benchmarks/lab2_comparison.py --suite fixed \
  --before benchmarks/work/revision-89a023969a2d/build \
  --output benchmarks/results/lab2-engineering-fixed.json
```

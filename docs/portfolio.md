# Physical Design Automation: an algorithm-engineering portfolio

This C++17 project turns four NYCU course implementations into independently
testable EDA tools: corner stitching, fixed-outline macro floorplanning,
incremental banking legalization, and two-layer die-to-die routing. They are
separate problem models, not a complete industrial place-and-route flow.

The engineering contribution is the measured transformation: ownership and state
contracts, core-library APIs, independent correctness oracles, alternative search
policies, profiling-driven data structures, safe parallel restarts, and reproducible
quality/runtime comparisons. Course specifications, starter algorithms, public
datasets and supplied evaluators retain their original attribution.

## Two design discussions

**Reliable incremental optimization.** Lab03 plans banking and bounded cell moves
against virtual exclusions before committing geometry and R-tree updates. Lab04
validates route deltas before changing edge usage; rerouting searches with the old
path virtually subtracted, so a deadline does not destroy the complete incumbent.
Expected search failures preserve state. Allocation failures during legalization
commit have an explicit discard-session contract. Typed IDs, RAII and separate
formatters make these boundaries directly testable. See
[state transactions](decisions/001-state-transactions.md) and
[negotiated routing](decisions/002-negotiated-routing.md).

**Measurement versus assumptions.** The floorplanner retains dense packing and
full snapshots as ablations for skyline packing and an undo journal. Independent
seed trajectories share immutable input and own all mutable state; fixed-total-work
eight-worker scaling previously measured 6.95× with identical selected solutions.
The new policies improve short-budget feasibility, but lose quality on vda317b.
The routing history heuristic also underperforms plain rerouting in the generated
sample. Neither becomes the compatibility default just because it is new.

## Selected evidence and limits

| Experiment | Observed result | Interpretation |
|---|---|---|
| 120 floorplanning trials; five cases, four held-out seeds, 1/3 CPU seconds | Legacy 12/40 legal; each new policy 40/40 | Among mutually legal pairs each new policy wins 8, loses 4; feasibility is not quality |
| 72 official routing trials | All legal, same cost as one-pass, more runtime | No official-case improvement claim |
| 36 generated congestion trials | Plain rerouting reduces original cost 1.58–6.81% | More search time; not a global optimality guarantee |
| Five official legalization cases | Minimum reduces total cost 22.3–84.1% versus legacy | More row searches and runtime; other cells remain fixed |
| Dense generated banking sequence | Repair can score 141 versus minimum's 138 | An immediate improvement can hurt later operations |

[Full tables, variation, plots and raw evidence](experiments/README.md) report
legality, objective, runtime and RSS separately. Failed attempts are retained.
Timing repetitions and stochastic seeds have distinct meanings; this shared host
does not support universal speedup or calibrated CI performance claims.

Validation combines GCC/Clang strict builds, ASan/UBSan, separate TSan runs,
bounded parser fuzzing, static analysis, exhaustive site checks, raster/HPWL/path
oracles, and official course evaluators. Small tests run without downloads; an
offline demo independently checks emitted numerical results before drawing them.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
python3 scripts/demo.py --output build/demo
```

Open `build/demo/index.html` to inspect a floorplan, individual nets and overflowing
routing edges. [Reproduction guide](reproduce.md) gives experiment and diagnostic
commands. Full LEF/DEF support, timing-driven optimization and industrial design
rules remain outside the supported course formats.

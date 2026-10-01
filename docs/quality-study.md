# Equal-CPU-budget quality study

Ten preselected seeds (1, 7, 19, 31, 43, 59, 71, 83, 97, 109), three public
floorplanning datasets, two CPU budgets (3 and 8 seconds), and two implementations
produce **120 attempts**. The baseline is `89a0239`; the optimized path is skyline
packing plus journal rollback, with one restart and one worker. Alpha is 0.5.
Runs are serial and variant order alternates by seed. GCC 11.4 Release, Intel Xeon
E5-2620 v4, Linux thread CPU accounting for the optimized solver and process CPU
accounting for the single-thread baseline. No builds/other local benchmarks ran
concurrently; CPU frequency and external machine load were not controlled.

## Feasibility and paired objective

Each feasibility column has ten attempts. Wins/ties/losses compare the optimized
objective only where **both runs of the same seed are legal**. A lower objective
is better. Percent changes use those same paired seeds; a negative change is an
improvement. Never substitute a cost for an infeasible run or average objectives
across datasets.

| Dataset | CPU seconds | Baseline legal | Optimized legal | Both legal | Wins / ties / losses | Paired median cost change |
|---|---:|---:|---:|---:|---:|---:|
| ami33 | 3 | 10/10 | 10/10 | 10 | 0 / 10 / 0 | +0.00% |
| ami33 | 8 | 10/10 | 10/10 | 10 | 0 / 10 / 0 | +0.00% |
| ami49 | 3 | 0/10 | 0/10 | 0 | 0 / 0 / 0 | — |
| ami49 | 8 | 0/10 | 6/10 | 0 | 0 / 0 / 0 | — |
| vda317b | 3 | 3/10 | 8/10 | 3 | 3 / 0 / 0 | -13.74% |
| vda317b | 8 | 8/10 | 10/10 | 8 | 8 / 0 / 0 | -6.93% |

The baseline produces 31 legal results, the optimized path 44. All 75 legal results
pass the official verifier; the other 45 attempts explicitly exhaust the budget.
There are no unexpected errors and no lost baseline-feasible seed in this sample.
ami33 reaches the same per-seed objective in both paths. ami49 shows improved
feasibility at eight seconds but has no paired legal baseline result for a cost
claim. vda317b improves feasibility and the cost of all 11 paired legal runs.

These are finite exploratory samples, not a statistical success-rate guarantee.
Short budgets deliberately include failures; they do not replace the course's
longer-budget integration tests. Every attempt, including failure, command,
executable/input hash, CPU/wall time, peak RSS, area/HPWL/objective and verifier
log is in [the raw JSON](../benchmarks/results/lab2-equal-cpu.json).

## Why fixed work and fixed time answer different questions

Fixed-iteration runs establish implementation equivalence and speed: all nine
seed/dataset groups in the separate 36-run ablation have identical solutions.
The retained timed annealer changes the number of perturbations per trial on a
CPU-time schedule and can reheat while searching for an outline-feasible state.
Fixed-iteration mode disables those clock-dependent choices. Faster packing can
therefore change a timed search trajectory, not merely run farther along exactly
the same sequence. A 300,000-iteration run and an eight-second run are not
interchangeable correctness or quality comparisons.

Use fixed iterations for exact regression/scaling and equal CPU budgets to study
feasibility and solution quality. `--restarts` changes total work unless the
per-restart budget is adjusted, so multi-start quality is not silently included
in this single-restart experiment.

```bash
python3 benchmarks/build_revision.py 89a0239
python3 benchmarks/lab2_comparison.py --suite time \
  --before benchmarks/work/revision-89a023969a2d/build \
  --output benchmarks/work/equal-cpu.json
```

The runner marks intermediate reports `complete=false` and records `expected_runs`;
completed reports contain `summary` and paired feasibility/quality comparisons.
Timed values and exact trajectories may vary between executions. The source
benchmark record preserves the observed experiment instead of choosing favorable
reruns.

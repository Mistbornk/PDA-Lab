# Independent floorplanning restarts

`Lab2 ... --seed 1 --iterations 300000 --restarts 8 --threads 4 --stats`
runs seeds 1 through 8, each with 300,000 perturbation iterations. A worker pool
claims whole searches through one relaxed atomic counter. Each search owns its
PRNGs, B*-tree, packing workspace, undo journal and best solution. The parsed
problem is shared read-only. Workers write to distinct pre-sized result slots;
futures join before selection and output. No optimizer state or stream is shared
between searches. Exceptions are retained until workers have joined. Budget
exhaustion is a separate outcome, while unexpected errors still fail the run.

After joining, choose the lowest objective among legal solutions; ties select the
lowest restart index. Failed seeds are retained in statistics. With a fixed
iteration budget, the chosen placement and each attempt are identical at 1/2/4/8
threads. Timed budgets naturally permit different work counts and solutions.

`--iterations` and `--seconds` apply **per restart**, not to the whole batch.
On Linux, seconds are thread CPU time; report line five is the sum of restart CPU
times. Wall time is recorded independently by the benchmark runner. On systems
without `CLOCK_THREAD_CPUTIME_ID`, the fallback is monotonic elapsed time, so timed
CPU comparisons are Linux-specific. The original one-restart CLI retains its
report semantics, and defaults remain one restart / one worker. Counts are in
[1,1024], actual workers are min(threads,restarts), and seed-range overflow is
rejected. More restarts may improve quality but spend proportionately more work;
thread speedups below hold total restart work fixed.

The concurrency tests compare all attempt outcomes and the selected geometry,
including mixed feasible/infeasible seeds, ties, all-infeasible batches and seed
overflow. ASan/UBSan check memory/undefined behavior; they are not race detectors.
Ownership/disjoint-write analysis establishes the intended race-free design.

Reproduce scaling with:

```bash
python3 benchmarks/lab2_scaling.py --case ami49 --iterations 300000 --repeat 3
```

This is solver-internal parallelism over independent trajectories. A single
annealing trajectory and congestion-dependent net routing remain sequential.

## Measured scaling

GCC 11.4 Release on Intel Xeon E5-2620 v4, ami49, alpha 0.5, eight seeds ×
300,000 iterations, three serial repetitions per worker count:

| Threads | Median wall time | Speedup | Median peak RSS |
|---:|---:|---:|---:|
| 1 | 21.128 s | 1.00× | 4.03 MiB |
| 2 | 10.761 s | 1.96× | 4.27 MiB |
| 4 | 5.488 s | 3.85× | 4.39 MiB |
| 8 | 3.040 s | 6.95× | 4.29 MiB |

All 12 selected results pass the official verifier, and all individual restart
outcomes and solution hashes agree across worker counts. This is a fixed-total-work
speedup, not evidence that eight searches cost as little CPU as one search. Small
budgets may be dominated by task-launch overhead. [Raw record](../benchmarks/results/lab2-scaling.json).

## Dynamic race checks

A separate Clang 14 ThreadSanitizer build now runs the private-RNG, parallel-restart
and policy groups, including successful, partially failed and all-failed searches.
The new policies own sampled timers and bounded traces per attempt. Tests compare
fixed-work winners at 1/2/4/8 workers; the TSan CI job executes rather than marking
unsupported runtimes as successful skips. Reproduction and final records are in
[the validation runner](../scripts/validate_portfolio.py) and
[final validation](interview-final-validation.json).

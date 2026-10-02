# Iteration-driven floorplanning policies and honest failure accounting

Status: two optional policies; the default remains `legacy`.

The compatibility annealer changes its feasibility pressure with elapsed CPU
time. That makes it a poor reference for short-budget reliability and fixed-work
policy comparisons. The new policies retain the same B*-tree, mutation journal,
skyline packer, HPWL evaluator and per-attempt RNG. Only candidate scoring,
acceptance and the temperature schedule change.

`progress` normalizes the original weighted area/HPWL cost by the initial cost
and adds a configurable normalized outline-excess penalty. `feasibility` first
minimizes outline excess with a small bounding-box tie preference, then restricts
proposals to legal floorplans while annealing the original objective. Both retain
the best legal candidate encountered, including candidates not accepted as the
current state. Neither proves feasibility or optimality.

Temperature cools by epochs of `epoch_moves * block_count` iterations and reheats
after `reheat_epochs`. The feasibility policy restarts the schedule at its first
legal solution. The clock only enforces the optional per-attempt CPU budget;
fixed-iteration trajectories do not depend on scheduling or diagnostic clocks.
Parameters are checked and legacy rejects inapplicable schedule flags. A budget
without a legal candidate returns exit 3, not an out-of-outline report.

Diagnostics record first legal iteration/CPU time, minimum outline excess,
accepted/uphill moves, and best-cost trace points. Evaluation timings sample
every 256 evaluations; trace storage is capped at 10,000 points and explicitly
marks truncation. These are sampled component timings, not a full profiler or
unbiased estimate of every allocation. Diagnostics are off by default. An initial
instrumentation experiment exposed clock overhead: reading the CPU clock only
for first-legal and trace events reduced the observed median stats overhead from
about 17% to about 0.1%; trace-every-5000 overhead was about 2.7% in the rerun.
Three fixed seeds produced identical solutions with diagnostics off/on.

The predeclared evaluation uses three official and two generated cases, four
held-out solver seeds, and 1/3 CPU-second budgets (120 serial trials). Tuning used
only ami33 and seeds 1/7; defaults were not retuned after evaluation. Final results:
legacy 12/40 legal, progress 40/40, feasibility 40/40. All 80 new-policy results
pass the official verifier. This is evidence on this sample, not a universal
success-rate guarantee. All 28 failed legacy attempts remain in the report.

Among the 12 pairs where legacy was legal, each new policy wins 8 and loses 4.
On vda317b at 3 CPU seconds, median costs are 19,973,369 (legacy), 30,256,868
(progress), and 30,956,745.5 (feasibility). Earlier feasibility can sacrifice
quality. These losses are why the compatibility default is unchanged. Aggregate
sampled packing and HPWL times are similar, so there is no evidence here to
justify a speculative incremental-HPWL rewrite.

Fixed-work tests cover journal/snapshot and skyline/dense equivalence,
diagnostics neutrality, monotonic best legal cost, failed searches, alpha=0,
and identical multi-start winners at 1/2/4/8 workers. Each worker owns its RNG,
state, trace and timers; the existing join/reduction boundary remains unchanged.
CPU budgets are per attempt: more restarts consume more total CPU work.

Raw records: `benchmarks/results/interview-policy-{tuning,evaluation,instrumentation}.json`.
The `*-initial.json` records preserve the instrumentation ablation. Wall times
vary with shared-host load; CPU-budget quality comparisons and fixed-work timing
comparisons must not be conflated. See the consolidated experiment report for
reproduction commands and all distributions.

# Owned random state and time budgets

Lab02 no longer calls process-global `srand`/`rand`. Each `solve` owns a 31-word
additive-feedback generator and the existing `std::mt19937` used by tree initialization.
Its values match glibc's default `rand` sequence so the validated Linux seeded
trajectories remain comparable. This generator is for search, not cryptography.

The recurrence is addition modulo 2^32 of entries separated by three positions in
a ring of 31 words, followed by dropping the low bit. Initialization uses the
Park–Miller recurrence modulo 2^31−1, treating zero seed as one, and 310 warm-up
steps. The implementation uses fixed-width unsigned arithmetic and integer indices,
so copies have independent state and no self-referencing pointers.
Algorithm reference: [glibc random_r.c](https://github.com/bminor/glibc/blob/master/stdlib/random_r.c).
The C++ implementation expresses these recurrences directly; it does not call or
vendor glibc implementation code. Tests compare 100,000 outputs for each of seven
boundary seeds against libc on glibc hosts. The tree's `std::shuffle` is still a
standard-library implementation detail; cross-library placement identity is not
claimed merely because the scalar random sequence is fixed.

On Linux, `--seconds` uses CLOCK_THREAD_CPUTIME_ID. Another search thread cannot
consume this search's budget. Other platforms without that clock use elapsed time.
Iteration budgets remain deterministic work limits. Acceptance/reheating retains
its original policy; only the state and budget ownership changed.

Eight concurrent searches are compared to serial runs using the same immutable
problem, seeds and work budgets. Per-search placement, tree, random state, workspace
and result are private; no process-global mutable solver state is needed.

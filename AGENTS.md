# AGENTS.md

## Project Mission

Transform this repository from a collection of Physical Design Automation course labs into a well-engineered, understandable, benchmarkable EDA codebase while preserving the original algorithmic intent of each lab.

The goal is not cosmetic cleanup. The goal is to demonstrate:

- algorithm engineering
- modern C++ design
- performance analysis
- scalable data structures
- profiling-driven optimization
- parallelization where appropriate
- reproducible experiments
- testing and correctness validation
- clear technical documentation

Do not assume what each Lab01–Lab04 implements. Inspect the repository first and infer the purpose, input format, output format, algorithms, constraints, and performance characteristics of every lab.

---

## Operating Mode

Work autonomously.

Do not stop after producing recommendations.

Inspect, implement, build, test, benchmark, document, and iterate.

When a task is ambiguous, prefer investigating the existing implementation and course format rather than asking for clarification.

You may make substantial architectural changes when justified, but preserve observable correctness.

Do not rewrite working algorithms merely for stylistic reasons.

---

## Phase 0 — Repository Audit

Before changing architecture:

1. Inspect every Lab directory.
2. Identify:
   - problem being solved
   - algorithm used
   - input/output format
   - computational complexity
   - major data structures
   - duplicated utilities
   - correctness risks
   - performance bottlenecks
3. Determine how each lab is currently built and executed.
4. Locate existing test cases and sample datasets.
5. Establish a correctness baseline.
6. Establish a runtime and memory baseline where possible.

Create:

`docs/architecture.md`

Include a short description of every lab and the relationship, if any, between them.

Do not fabricate missing design rationale.

---

## Architecture Direction

If the labs contain reusable concepts, gradually extract them into shared components.

Possible target structure:

```text
.
├── CMakeLists.txt
├── cmake/
├── include/
│   └── pda/
├── src/
├── apps/
│   ├── lab01/
│   ├── lab02/
│   ├── lab03/
│   └── lab04/
├── tests/
├── benchmarks/
├── datasets/
├── scripts/
└── docs/
```

This is a guideline, not a requirement.

Do not force unrelated algorithms into artificial abstractions.

Prefer:

- explicit ownership
- RAII
- value semantics where sensible
- const correctness
- standard library containers/algorithms
- strong types for IDs/coordinates where useful
- separation between parsing, core algorithms, and output formatting

Avoid:

- giant classes
- unnecessary inheritance
- excessive abstraction
- premature template metaprogramming
- global mutable state

---

## Build System

Move toward a consistent CMake-based build if practical.

Desired commands:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Enable appropriate compiler warnings.

For GCC/Clang development builds, consider:

```text
-Wall
-Wextra
-Wpedantic
-Wconversion
-Wshadow
```

Do not blindly enable flags that make third-party dependencies unusable.

Provide optional sanitizer builds:

- AddressSanitizer
- UndefinedBehaviorSanitizer

---

## Correctness

Correctness has higher priority than optimization.

Before optimizing an algorithm:

1. capture representative outputs
2. create regression tests
3. identify expected invariants
4. compare optimized and baseline implementations

Where exact outputs may legitimately differ because several equivalent solutions exist, test objective value and validity rather than byte-for-byte identity.

For EDA optimization problems, distinguish:

- solution legality
- objective value
- runtime
- memory consumption

Do not claim an optimization is better solely because runtime decreased if solution quality regressed.

---

## Performance Engineering

Do not perform speculative optimization.

Use measurements.

For important algorithms:

1. establish baseline runtime
2. profile
3. identify hotspots
4. optimize the hotspot
5. rerun correctness tests
6. benchmark again

Potential areas to investigate:

- unnecessary copying
- poor asymptotic behavior
- repeated parsing
- repeated graph traversal
- allocation-heavy inner loops
- cache-unfriendly structures
- unnecessary pointer chasing
- inefficient priority queues/maps
- duplicate computation
- avoidable string processing
- excessive file I/O

Where appropriate evaluate:

- `std::vector` vs node-based containers
- contiguous structures
- preallocation
- move semantics
- custom compact graph representations
- integer-width reductions
- spatial indexing
- incremental updates
- memoization

Never replace an implementation merely because another structure is theoretically faster.

Benchmark it.

---

## Parallelization

Parallelization is encouraged only when the computation contains safe independent work.

First determine dependencies.

Possible options:

- `std::thread`
- `std::async`
- OpenMP
- parallel STL

Prefer the simplest mechanism appropriate for the workload.

Do not parallelize inherently sequential optimization loops without proving correctness.

Document:

- what work is parallelized
- why it is race-free
- synchronization strategy
- scaling results

Benchmark at multiple thread counts when possible:

```text
1
2
4
8
```

---

## Benchmarking

Create a reproducible benchmark framework.

Store benchmark scripts under:

`benchmarks/`

Record at minimum:

- dataset
- executable
- parameters
- runtime
- peak memory if obtainable
- objective/quality metric where applicable
- compiler
- build type
- number of threads

Prefer machine-readable output such as CSV or JSON.

Do not commit huge benchmark datasets unless licensing and repository size make sense.

---

## Testing

Add tests around:

- parsers
- core data structures
- algorithm primitives
- boundary cases
- malformed input
- empty/small input
- regression cases

If existing official lab inputs exist, preserve them as integration tests when practical.

---

## Documentation

Create a strong root README.

It should eventually contain:

1. project overview
2. what each lab solves
3. algorithms implemented
4. architecture
5. build instructions
6. usage examples
7. benchmark methodology
8. performance results
9. important engineering decisions
10. limitations

Explain algorithms at a level suitable for a technical interviewer.

Avoid marketing language.

---

## Modernization Policy

Prefer C++17 or newer where supported by the repository.

Modernize unsafe/manual patterns where useful.

Examples:

- raw owning pointers → RAII
- manual arrays → `std::vector`
- C-style casts → safe casts
- magic constants → named constants
- manual resource cleanup → scoped ownership

Do not mechanically rewrite every line.

---

## Git Discipline

Make changes in coherent units.

Suggested progression:

1. audit and baseline
2. build/test infrastructure
3. architecture cleanup
4. correctness improvements
5. performance optimization
6. parallelization
7. features
8. benchmarks
9. documentation

Keep the repository buildable after major milestones.

---

## Definition of Done

The project is substantially improved only when:

- all existing functionality still works
- build process is reproducible
- important algorithms have tests
- architecture is understandable
- measurable bottlenecks have been investigated
- performance claims contain benchmark evidence
- major algorithmic decisions are documented
- README allows another engineer to build and run the project
- improvements are technically defensible in an interview

---

# CODEX MASTER PROMPT

You are the principal engineer responsible for turning this Physical Design Automation course repository into a portfolio-quality EDA engineering project.

Work autonomously and aggressively, but do not sacrifice correctness.

Do not merely review the repository or give me suggestions. Make the changes.

Start by reading AGENTS.md and inspecting the entire repository.

First determine exactly what Lab01, Lab02, Lab03, and Lab04 implement. Do not assume their purpose from their names.

Then execute the following program of work:

### Stage 1 — Reverse engineer and baseline

Understand each lab's:

- algorithm
- input/output
- data structures
- complexity
- build process
- correctness constraints
- performance characteristics

Build and execute whatever can currently run.

Preserve baseline outputs and benchmark representative workloads.

Write your findings to `docs/architecture.md`.

### Stage 2 — Engineering infrastructure

Create a clean and reproducible build/test structure.

Prefer CMake if practical.

Add:

- appropriate warning flags
- debug/release builds
- sanitizer support
- automated tests
- scripts for representative workloads

Do not break compatibility with existing lab inputs.

### Stage 3 — Refactor

Refactor code where there is clear engineering value.

Prioritize:

- separating parsing from algorithms
- removing dangerous global state
- fixing ownership/lifetime issues
- removing duplicated utilities
- making important data structures explicit
- making algorithm boundaries understandable

Do not over-engineer unrelated labs into one framework.

### Stage 4 — Performance

Profile the algorithms.

Find actual hotspots rather than guessing.

Investigate algorithmic improvements before micro-optimization.

Consider:

- asymptotic improvements
- more suitable data structures
- allocation reduction
- cache locality
- duplicate computation
- faster traversal
- preallocation
- incremental computation

Every significant optimization must have before/after evidence.

### Stage 5 — Parallelism

Identify workloads with real independent work.

Parallelize only where correctness is clear.

Measure scaling at different thread counts.

Do not add concurrency solely so the README can say "parallel".

### Stage 6 — Features and algorithm improvements

After understanding each lab, identify extensions that genuinely improve its technical depth.

Implement useful extensions when feasible.

Examples may include:

- alternative algorithm variants
- configurable heuristics
- improved solution-quality/runtime tradeoffs
- better diagnostics
- visualization/export formats
- benchmark modes

Choose features based on what the actual labs implement.

### Stage 7 — Portfolio quality

Create a professional README explaining:

- the physical-design problems
- implemented algorithms
- architecture
- build and usage
- benchmark results
- algorithmic tradeoffs
- performance work
- future work

Preserve attribution to the original course context.

Do not falsely present course specifications or starter code as original work.

### Required engineering rule

Whenever you think an optimization is complete, ask:

1. Did correctness remain intact?
2. Is solution quality unchanged or intentionally traded off?
3. Do benchmarks prove the improvement?
4. Can the design be explained clearly in a systems/EDA interview?

Continue iterating while there are high-value improvements available.

You have permission to make broad codebase changes, add tests, restructure directories, add benchmarks, improve algorithms, add parallel execution, and implement technically meaningful new features.

Do not stop after a superficial cleanup.

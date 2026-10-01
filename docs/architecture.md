# Architecture audit and engineering boundaries

The repository contains four independent NYCU Physical Design Automation course
programs. They share file/CLI handling and experiment infrastructure, not a common
optimization model. Original specifications, attribution, and validators remain in
`LabXX/*_Guide` and `LabXX/tests`. Per-lab input/output contracts are in `TESTING.md`.
This document describes observed implementation; missing design rationale is not inferred.

## Original baseline (before modernization)

| Lab | Problem and algorithm | Representation / cost drivers | Original build |
|---|---|---|---|
| 01 | Corner stitching; rectangle insertion, point location, adjacency counts | Linked list of owning raw tile pointers, four corner stitches. Full tile scans to repair each affected neighbor; list removal is linear. With T live tiles and K affected neighbors, repairs cost O(KT). Queries walk stitches, O(T) worst case. | C++14, `-O0 -g`, `Lab1` |
| 02 | Fixed-outline hard-macro floorplanning; B*-tree and simulated annealing | Vectors of blocks/tree nodes, a coordinate-sized dense contour, string net pins and hash lookups. Each trial copies the tree and blocks, repacks across summed macro widths, then recomputes all pin HPWL. Cost depends on coordinate magnitudes, not only block count. | C++11, `-O3`, `Lab2` |
| 03 | Incremental FF banking legalization; nearest-row search and R-tree collision queries | R-tree entries copy full cells/strings; every candidate materializes two result vectors. Every banking step scans/compacts all cells with linear search of removed names: O(CB) per step. Rows sorted by distance O(R log R), then repeated R-tree queries until a gap is found. Spatial queries have data-dependent worst-case O(C), not guaranteed logarithmic. | C++14, `-O3 -static`, Boost headers, `Legalizer` |
| 04 | Two-layer die-to-die routing; sequential A* heuristic search | Dense nested grids, binary heap, per-net O(V) initialization, per-expanded-node heap allocations for direction lists. Search is roughly O((V+E) log V) with bounded-degree grid, and O(V) state. Capacity updates couple nets. | C++14, `-O3`, `D2DGRter` |

The previous resource audit established 16 passing official/public/toy integrations;
see `tests/VALIDATION.md` and `tests/validation_snapshot.json`. Those timings used
concurrent cases, so they are correctness evidence, not controlled speedup evidence.
Baseline sources/binaries for this work are preserved under ignored
`benchmarks/work/baseline/`; measurements and profiler summaries will be recorded
separately with exact commands and flags.

## Correctness risks found by reading the baseline

- All programs dereference CLI arguments before checking their count and largely
  trust stream extraction. Missing/malformed files can crash or hang.
- Lab01 deletes a merged side tile but may dereference the old side pointer again;
  live tiles are not freed on exit. Corner-only contacts are not neighbors; point
  queries must observe insertion time, not final placement.
- Lab02 mixes integer widths/products with larger area values, can attempt two
  distinct random IDs with a one-block input, and has a time-dependent perturbation
  count that can become zero (leaving a candidate uninitialized). Seed and work
  budget are not configurable. Performance comparisons require deterministic work.
- Lab03 appends to output on every step, dispatches a heuristic by filename, infers
  fixed status from name prefixes, truncates output coordinates, and only logs
  placement failures. The local search does not move existing cells. Equal-distance
  row order is stateful; changing it can change solution quality.
- Lab04 treats layer as a property of a single grid state instead of an independent
  search dimension; its heuristic/cost model is not a proof of optimal routing.
  Source=destination produces a one-point path and indexes past its end. Per-net
  capacity dependence prevents independent routing without changing semantics.

## Modernization policy for this iteration

Retain each lab's algorithm unless measurements or a correctness failure justify a
change. Use C++17 and a root CMake build with separate executables, development
warnings and optional sanitizers. Add reusable checked CLI/file helpers only where
semantics are genuinely shared. Keep per-lab parsing and algorithm models local.
Use the official validators plus independent small/reference tests; separate legality,
quality, runtime and memory. Preserve baseline comparison artifacts, and report
limitations (including unchanged heuristic restrictions) explicitly.

## Implemented architecture

- `CMakeLists.txt` builds independent C++17 executables into per-lab directories.
  `pda_options` carries compiler diagnostics and optional ASan/UBSan; downloaded
  evaluators are not linked into production solvers. Original Makefiles still work.
- `include/pda/io.hpp` contains checked stream/CLI primitives only. Parse errors
  produce a diagnostic and nonzero status rather than proceeding with uninitialized data.
- Lab01's `TileList` owns `unique_ptr<Block>` and four edge-coordinate indexes.
  Tile geometry is immutable; insert/extract update indexes and list positions
  together. Stitches remain non-owning. Retiring owners stay alive until repair
  finishes. `--stitches indexed` restricts each stitch lookup to an edge bucket,
  visiting candidates newest-first to preserve the last-match rule during
  overlapping intermediate splits/merges. `--stitches scan` preserves full-scan
  lookup as a reference. Given B candidates on a coordinate, indexed lookup is
  O(B) worst case, insertion/removal O(log B) per ordered bucket plus expected O(1)
  hash lookup; it is not an interval-tree or a worst-case O(log T) point index.
  A separate R-tree selects solid-overlap and top/bottom split candidates; exact
  integer half-open predicates filter intersections. All insertion/extraction
  paths update both indexes. `--geometry scan` preserves the reference. Spatial
  queries remain O(T) worst case. `parser.cpp`, `layout.cpp`, `report.cpp` separate
  input, algorithm orchestration and output from the CLI.
- Lab02 now builds a reusable `lab2_core` library. `include/model.hpp` defines
  immutable problem data separately from mutable placement and search options.
  `src/parser.cpp`, `tree.cpp`, `packing.cpp`, `annealer.cpp`, and `report.cpp`
  separate parsing, B*-tree mutations, geometry/HPWL, search policy, and formatting.
  The CLI only parses options and coordinates these APIs. Tree and packing can be
  tested without running annealing or opening files. A reusable skyline uses at
  most 2*n+1 breakpoints, O(n) space independent of coordinates, and O(n²) worst-case
  packing. An undo journal restores touched structural nodes/rotations; derived
  coordinates are repacked. Dense packing and full snapshots remain selectable
  ablations. Pin IDs avoid hot-loop hash lookups; `--hpwl strings` is the comparison
  path. Fixed iterations disable time-dependent reheating decisions. Each solve
  owns its random state, including a fixed-width generator preserving Linux libc's
  seeded scalar sequence; the initialization shuffle still depends on the standard
  library. See [packing/journal evidence](floorplanning-engineering.md) and
  [randomness contract](randomness.md). `parallel.cpp` schedules independent
  restarts and deterministically selects a legal result after joining workers.
- Lab03 separates `parser.cpp`, `legalizer.cpp`, and CLI. A solver owns stable cell
  slots, a name-to-slot map, compact `(Box, ID)` R-tree entries and row order.
  Removed slots are retained for stable IDs (O(initial cells + banking steps)
  storage); live names are removed directly. First-fit has selectable `interval`
  and `point` paths. With k intersecting obstacles, interval search uses one query
  plus O(k log k) sorting per attempted row instead of repeated point queries.
  Input FIX attributes, floating-point output and output truncation are respected.
  Legacy filename heuristic dispatch remains only as a compatibility default;
  explicit strategies make new experiments independent of paths.
- Lab04 separates parsing, CLI and two routing modules: `legacy_router.cpp` keeps
  the contiguous legacy Router for compatibility; `layered_router.cpp` supplies
  the layer-aware alternative. Its state is `(GCell, arrival layer)`; cell costs are charged
  on departure, with an explicit sink for destination M1/via accounting. Edge cost
  uses marginal overflow, and A* uses weighted Manhattan distance. Workspace is
  reused but each net still depends on previous capacity usage. See
  [cost model and proof boundary](layered-routing.md) for the nonnegative graph,
  independent four-state Dijkstra oracle, and per-net versus joint optimality.

## Validation boundaries and tradeoffs

`tests/regression.py` supplies an independent raster oracle for Lab01, geometric
and HPWL checks for Lab02, point/interval equivalence plus incremental legality for
Lab03, and path/layer/grid checks for Lab04. It covers missing arguments/files,
truncated syntax, duplicate/unknown IDs, alpha endpoints, one macro, 64-bit area,
fractional rows, FIX attributes, overlap, and impossible placement. Course
validators provide separate public integrations; their zero exit code alone is
not trusted. Tests remain active in Release builds.

The optimized Lab03 output is byte-identical to the historical program on all five
public inputs. Its first-fit equivalence is also checked on generated small cases.
A proposed extra restriction on the *top* of multi-row cells was rejected: the
course validates the bottom-left site and die boundary, so that restriction would
have changed valid solutions. Row/die semantics follow the supplied contracts.
Lab04 outputs match all four public/toy historical outputs; Lab02 reference and
ID paths match at a fixed seed and iteration budget, excluding the runtime line.
This does not prove global optimality or complete coverage of hidden inputs.

Independent benchmark/test processes own separate output directories and logs.
Lab02 additionally parallelizes independent seed trajectories inside one solver:
immutable input is shared, mutable state is per search, and result slots are
pre-sized and disjoint. An atomic index assigns work; futures join before selecting
and emitting a result. A single annealing trajectory, per-net routing and banking
remain sequential because their state transitions are dependent. See
[parallel floorplanning](parallel-floorplanning.md) for budgets, tie-breaking,
exception handling, race analysis and fixed-total-work scaling.

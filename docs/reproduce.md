# Reproduce the portfolio evidence

The supported reference environment is Ubuntu 22.04 x86-64, GCC 11.4 or Clang 14,
CMake 3.16+, Python 3.10+, Boost headers and GNU time. Course evaluator binaries
are Linux x86-64. Core tests and the offline demo require no downloaded evaluator.

```bash
sudo apt-get install build-essential cmake libboost-dev python3 time
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DPDA_STRICT_WARNINGS=ON -DPDA_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
python3 scripts/demo.py --bin-root build --output build/demo
```

Open `build/demo/index.html` directly in a browser. It has no CDN, server, telemetry
or runtime network dependency. Its two SVGs, numerical reports, generated inputs,
logs and hash manifest are saved alongside it. The demo checks macro names,
dimensions, overlap, outline, bounding area and HPWL; routing checks connectivity,
layers, usage, overflow, vias and the original aggregate objective. No second
optimizer is embedded in the visualization.

## Development diagnostics

```bash
sudo apt-get install clang-14 libclang-rt-14-dev
python3 scripts/validate_portfolio.py --jobs 4
```

The script records commands, source hashes, return codes and log tails. It runs
three strict compiler/sanitizer configurations, TSan concurrency tests, four
bounded libFuzzer targets and Clang Static Analyzer. TSan is a separate build and
must actually run; an unsupported runtime is a failed check, not a passing skip.
ASan leak checks and UBSan halt-on-error are enabled. Fuzz campaigns request
20,000 mutations per parser with a 30-second cap, seed 1337, 16 KiB inputs,
512 MiB RSS cap and a 3-second per-input timeout. Lab04 fuzzing limits grids to
4,096 cells. Mutation corpora and reproducers remain in ignored work directories.

Before running the full validation script, restore the pinned course files for its final resource check:

```bash
python3 tests/fetch_resources.py
python3 tests/fetch_resources.py --check
```

## Quality and performance studies

Run the [study commands](../benchmarks/README.md#interview-roadmap-studies) serially,
without concurrent builds or tests. Original records under `benchmarks/results/`
remain immutable evidence; write reruns to `benchmarks/work/` or a new named file.
Every run retains parameters, input/executable hashes, toolchain, legality,
quality, wall time, GNU user/system CPU time and peak RSS where available.
Compare within the same toolchain and parameters, and report failed attempts.

The optional chart generator needs matplotlib (`python3-matplotlib` on Ubuntu):

```bash
python3 benchmarks/report.py --output build/experiment-report
```

It summarizes the checked-in final study filenames into CSV/JSON, tables and
standalone SVG figures. It labels report-generation provenance separately from
historical run provenance, including dirty working trees. Do not present newly
captured source hashes as if they had been recorded before a historical run.

Fixed-work floorplan results are reproducible within the same toolchain; initial
`std::shuffle` may differ across standard libraries. CPU-budget searches can do
different work on another host. Routing wall deadlines are cooperative and may
overshoot during allocation/scoring. Full banking/routing trajectories are serial;
only independent floorplanning restarts are parallel.

## Scope and attribution

Guide files, supplied datasets and reference binaries are course assets. Downloaded
Lab03 MIT licensing is preserved. Other course assets have not been given a new
blanket license. Self-authored generated inputs are created by
`benchmarks/generated.py`; large files and mutable experiment outputs stay outside
Git. The original lab directories and compatible CLI/report formats remain intact.

The [milestone record](interview-progress.md), [final validation record](interview-final-validation.json)
and [GitHub Actions](https://github.com/Mistbornk/PDA-Lab/actions/workflows/ci.yml)
distinguish local checks from remote CI. The clean-checkout record identifies the
exact archived revision used for the final offline build/test/demo verification.

To repeat the isolated archive check on a committed revision (including all four Makefiles):

```bash
python3 scripts/clean_checkout.py --revision HEAD --output build/clean-checkout.json
```

The archived source has no ignored downloaded datasets or pre-existing CMake cache.
Tracked historical course executables are preserved in the main checkout; Makefile
rebuilds overwrite only their copies inside the isolated archive.

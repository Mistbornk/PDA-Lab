# Continuous integration

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) runs the same CMake/CTest
entrypoints used locally. The default push/PR matrix is:

| Compiler | Build | Sanitizers | Tests |
|---|---|---|---|
| GCC | Release | off | all eight default CTest groups |
| Clang 14 | Release | off | all eight default CTest groups |
| GCC | Debug | ASan + UBSan, leak detection | all eight default CTest groups |

GCC and Clang enforce `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`
on project targets; Boost is a system include. Dependencies are installed on Ubuntu 22.04. Core tests use bundled small files or
generated inputs and do not download course evaluators. Failure logs are retained
for seven days. The workflow has read-only repository permissions; checkout and
artifact actions use exact verified v7.0.0 commit IDs. Upstream action contracts:
[checkout](https://github.com/actions/checkout/tree/v7.0.0),
[upload-artifact](https://github.com/actions/upload-artifact/tree/v7.0.0).

The optional `workflow_dispatch` input `official=true` enables a separate job that
restores the manifest-pinned course resources, builds Release, runs all 16 course
cases with two independent workers, and validates the layered router on all four
routing datasets, and the generated large stress suite. Integration jobs run on Linux x86-64 for the course binaries.
Full course tests are not required on every small pull request.

Local validation completed on GCC 11.4 and Clang 14, and on GCC ASan/UBSan.
`actionlint` 1.7.12 validates the workflow syntax and expressions; its optional
shellcheck and pyflakes integrations were disabled because they are not installed.
The downloaded actionlint archive was checked against the release SHA256 manifest:
`8aca8db96f1b94770f1b0d72b6dddcb1ebb8123cb3712530b08cc387b349a3d8`.
The tool is only in ignored `benchmarks/work/tools/` and is not a project dependency.

The first modernization commit `89a0239` completed the remote GCC/Clang/sanitizer
matrix successfully ([run 36851128857](https://github.com/Mistbornk/PDA-Lab/actions/runs/36851128857)).
That run skipped the optional official job. Subsequent revisions use the stricter
warning gate, PRNG/concurrency/journal tests and generated stress smoke cases.
Current runs are available on [GitHub Actions](https://github.com/Mistbornk/PDA-Lab/actions/workflows/ci.yml).

The stricter full [workflow_dispatch run 36856085884](https://github.com/Mistbornk/PDA-Lab/actions/runs/36856085884)
at `7800201` succeeds in all four jobs, including official integrations, layered
routing and large stress inputs. [Machine-readable remote evidence](remote-validation.json)
records the exact head, jobs and step conclusions. Subsequent CLI regression
validation and local source hashes are recorded separately in
[completion-validation.json](completion-validation.json).

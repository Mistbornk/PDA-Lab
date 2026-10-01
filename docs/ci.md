# Continuous integration

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) runs the same CMake/CTest
entrypoints used locally. The default push/PR matrix is:

| Compiler | Build | Sanitizers | Tests |
|---|---|---|---|
| GCC | Release | off | all four default CTest groups |
| Clang 14 | Release | off | all four default CTest groups |
| GCC | Debug | ASan + UBSan, leak detection | all four default CTest groups |

Dependencies are installed on Ubuntu 22.04. Core tests use bundled small files or
generated inputs and do not download course evaluators. Failure logs are retained
for seven days. The workflow has read-only repository permissions; checkout and
artifact actions use exact verified v7.0.0 commit IDs. Upstream action contracts:
[checkout](https://github.com/actions/checkout/tree/v7.0.0),
[upload-artifact](https://github.com/actions/upload-artifact/tree/v7.0.0).

The optional `workflow_dispatch` input `official=true` enables a separate job that
restores the manifest-pinned course resources, builds Release, runs all 16 course
cases with two independent workers, and validates the layered router on all four
routing datasets. Integration jobs run on Linux x86-64 for the course binaries.
Full course tests are not required on every small pull request.

Local validation completed on GCC 11.4 and Clang 14, and on GCC ASan/UBSan.
`actionlint` 1.7.12 validates the workflow syntax and expressions; its optional
shellcheck and pyflakes integrations were disabled because they are not installed.
The downloaded actionlint archive was checked against the release SHA256 manifest:
`8aca8db96f1b94770f1b0d72b6dddcb1ebb8123cb3712530b08cc387b349a3d8`.
The tool is only in ignored `benchmarks/work/tools/` and is not a project dependency.

The workflow has not been pushed or run on GitHub in this session. Local test
success and workflow linting are the available evidence; remote runner execution
will first occur after these changes are committed and pushed.

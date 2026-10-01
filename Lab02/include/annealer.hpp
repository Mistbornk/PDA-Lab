#pragma once
#include "model.hpp"
#include "timing.hpp"
namespace lab2 {
// Each invocation owns its random state, placement, and thread CPU budget.
Result solve(const Problem &problem, const Options &options, double start = search_seconds());
void write_report(const std::string &path, const Result &result, double elapsed_seconds);
} // namespace lab2

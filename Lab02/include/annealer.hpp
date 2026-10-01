#pragma once
#include "model.hpp"
namespace lab2 {
// Keeps the original libc rand sequence for course-result compatibility.
// Calls must be serialized within a process; benchmark workers use separate processes.
Result solve(const Problem &problem, const Options &options, std::clock_t start = std::clock());
void write_report(const std::string &path, const Result &result, double elapsed_seconds);
} // namespace lab2

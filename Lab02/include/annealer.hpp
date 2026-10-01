#pragma once
#include "model.hpp"
#include "timing.hpp"
#include <stdexcept>
namespace lab2 {
class NoLegalPlacement : public std::runtime_error {
  public:
    std::uint64_t iterations;
    explicit NoLegalPlacement(std::uint64_t count)
        : std::runtime_error("No legal floorplan found within budget"), iterations(count) {}
};
// Each invocation owns its random state, placement, and thread CPU budget.
Result solve(const Problem &problem, const Options &options, double start = search_seconds());
void write_report(const std::string &path, const Result &result, double elapsed_seconds);
} // namespace lab2

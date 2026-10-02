#pragma once
#include "model.hpp"
#include "timing.hpp"
#include <iosfwd>
#include <stdexcept>
namespace lab2 {
class NoLegalPlacement : public std::runtime_error {
  public:
    std::uint64_t iterations;
    SearchStats stats;
    explicit NoLegalPlacement(std::uint64_t count, SearchStats diagnostics = {})
        : std::runtime_error("No legal floorplan found within budget"), iterations(count),
          stats(std::move(diagnostics)) {}
};
// Each invocation owns its random state, placement, and thread CPU budget.
Result solve(const Problem &problem, const Options &options, double start = search_seconds());
Result solve_progress(const Problem &problem, const Options &options, double start);
double outline_excess(const Problem &problem, const Cost &cost);
void observe(SearchStats &stats, const Problem &problem, const Cost &cost, std::uint64_t iteration,
             double start, long long best, std::uint64_t trace_every);
void write_search_stats(std::ostream &output, const SearchStats &stats);
void write_report(const std::string &path, const Result &result, double elapsed_seconds);
} // namespace lab2

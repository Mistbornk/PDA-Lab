#include "annealer.hpp"
#include <algorithm>
#include <iomanip>
#include <ostream>
namespace lab2 {
double outline_excess(const Problem &problem, const Cost &cost) {
    return std::max(0., static_cast<double>(cost.width) / problem.outline_width - 1) +
           std::max(0., static_cast<double>(cost.height) / problem.outline_height - 1);
}
void observe(SearchStats &stats, const Problem &problem, const Cost &cost, std::uint64_t iteration,
             double start, long long best, std::uint64_t trace_every) {
    const double excess = outline_excess(problem, cost);
    const bool first_legal = excess == 0 && stats.first_legal_cpu_seconds < 0;
    const bool traced = trace_every && iteration % trace_every == 0;
    const double elapsed = first_legal || traced ? search_seconds() - start : 0;
    if (stats.minimum_outline_excess < 0 || excess < stats.minimum_outline_excess)
        stats.minimum_outline_excess = excess;
    if (excess == 0 && stats.first_legal_cpu_seconds < 0) {
        stats.first_legal_cpu_seconds = elapsed;
        stats.first_legal_iteration = iteration;
    }
    if (trace_every && iteration % trace_every == 0) {
        TracePoint point{iteration, elapsed, excess, best};
        if (!stats.trace.empty() && stats.trace.back().iteration == iteration)
            stats.trace.back() = point;
        else if (stats.trace.size() < 10000)
            stats.trace.push_back(point);
        else
            stats.trace_truncated = true;
    }
}
void write_search_stats(std::ostream &output, const SearchStats &stats) {
    output << std::setprecision(17) << "{\"first_legal_cpu_seconds\":";
    if (stats.first_legal_cpu_seconds < 0)
        output << "null";
    else
        output << stats.first_legal_cpu_seconds;
    output << ",\"first_legal_iteration\":" << stats.first_legal_iteration
           << ",\"minimum_outline_excess\":" << stats.minimum_outline_excess
           << ",\"accepted\":" << stats.accepted << ",\"uphill_accepted\":" << stats.uphill_accepted
           << ",\"profile_samples\":" << stats.evaluation.samples
           << ",\"sampled_packing_seconds\":" << stats.evaluation.packing_seconds
           << ",\"sampled_hpwl_seconds\":" << stats.evaluation.hpwl_seconds
           << ",\"trace_truncated\":" << (stats.trace_truncated ? "true" : "false")
           << ",\"trace\":[";
    for (std::size_t i = 0; i < stats.trace.size(); ++i) {
        const auto &point = stats.trace[i];
        if (i)
            output << ',';
        output << "{\"iteration\":" << point.iteration << ",\"cpu_seconds\":" << point.cpu_seconds
               << ",\"outline_excess\":" << point.outline_excess << ",\"best_objective\":";
        if (point.best_objective < 0)
            output << "null";
        else
            output << point.best_objective;
        output << '}';
    }
    output << "]}";
}
} // namespace lab2

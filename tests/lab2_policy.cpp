#include "annealer.hpp"
#include "parallel.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
void same(const lab2::Result &a, const lab2::Result &b) {
    check(a.cost.cost == b.cost.cost && a.blocks.size() == b.blocks.size(),
          "Policy result changed");
    for (std::size_t i = 0; i < a.blocks.size(); ++i) {
        const auto &x = a.blocks[i], &y = b.blocks[i];
        check(x.x == y.x && x.y == y.y && x.width == y.width && x.height == y.height,
              "Instrumentation/storage/scheduling changed placement");
    }
}
void legal(const lab2::Problem &problem, const lab2::Result &result) {
    for (std::size_t i = 0; i < result.blocks.size(); ++i) {
        const auto &a = result.blocks[i];
        check(a.x >= 0 && a.y >= 0 && a.x + a.width <= problem.outline_width &&
                  a.y + a.height <= problem.outline_height,
              "Invalid outline");
        for (std::size_t j = 0; j < i; ++j) {
            const auto &b = result.blocks[j];
            check(a.x + a.width <= b.x || b.x + b.width <= a.x || a.y + a.height <= b.y ||
                      b.y + b.height <= a.y,
                  "Overlapping macros");
        }
    }
}
int main() {
    lab2::Problem problem;
    problem.outline_width = problem.outline_height = 100;
    for (int i = 0; i < 16; ++i)
        problem.blocks.push_back({std::to_string(i), 0, 0, 2 + i % 5, 3 + i % 3, false});
    for (auto policy : {lab2::SearchPolicy::Progress, lab2::SearchPolicy::FeasibilityFirst}) {
        lab2::Options options;
        options.policy = policy;
        options.iterations = 12000;
        options.seed = 7;
        const auto plain = lab2::solve(problem, options);
        legal(problem, plain);
        options.diagnostics = true;
        options.trace_every = 300;
        const auto instrumented = lab2::solve(problem, options);
        same(plain, instrumented);
        check(instrumented.stats.first_legal_iteration == 0 &&
                  instrumented.stats.first_legal_cpu_seconds >= 0 &&
                  instrumented.stats.accepted <= options.iterations &&
                  instrumented.stats.uphill_accepted <= instrumented.stats.accepted,
              "Invalid search diagnostics");
        check(instrumented.stats.trace.back().best_objective == instrumented.cost.cost,
              "Trace missed final best result");
        long long previous = instrumented.stats.trace.front().best_objective;
        for (const auto &point : instrumented.stats.trace) {
            check(point.best_objective <= previous, "Anytime best cost increased");
            previous = point.best_objective;
        }
        options.packing = lab2::PackingMode::Dense;
        options.undo_journal = false;
        same(plain, lab2::solve(problem, options));
        options.packing = lab2::PackingMode::Skyline;
        options.undo_journal = true;
        options.iterations = 2000;
        const auto serial = lab2::solve_multi(problem, options, {8, 1});
        for (unsigned threads : {2u, 4u, 8u}) {
            const auto parallel = lab2::solve_multi(problem, options, {8, threads});
            same(serial.best, parallel.best);
            check(serial.winning_seed == parallel.winning_seed, "Policy winner depends on threads");
        }
        auto impossible = problem;
        impossible.outline_width = impossible.outline_height = 1;
        bool failed = false;
        try {
            (void)lab2::solve(impossible, options);
        } catch (const lab2::NoLegalPlacement &error) {
            failed = error.iterations == options.iterations &&
                     error.stats.first_legal_cpu_seconds < 0 &&
                     error.stats.minimum_outline_excess > 0;
        }
        check(failed, "Infeasible policy run lost its diagnostics");
        options.alpha = 0;
        check(lab2::solve(problem, options).cost.cost == 0, "Zero objective policy failed");
    }
}

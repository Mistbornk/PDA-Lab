#include "parallel.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
void equal(const lab2::MultiResult &a, const lab2::MultiResult &b) {
    check(a.winning_seed == b.winning_seed && a.best.cost.cost == b.best.cost.cost &&
              a.total_iterations == b.total_iterations,
          "Thread count changed selected result");
    for (std::size_t i = 0; i < a.best.blocks.size(); ++i) {
        const auto &x = a.best.blocks[i];
        const auto &y = b.best.blocks[i];
        check(x.x == y.x && x.y == y.y && x.width == y.width && x.height == y.height,
              "Thread count changed placement");
    }
    for (std::size_t i = 0; i < a.attempts.size(); ++i)
        check(a.attempts[i].seed == b.attempts[i].seed &&
                  a.attempts[i].legal == b.attempts[i].legal &&
                  a.attempts[i].cost.cost == b.attempts[i].cost.cost &&
                  a.attempts[i].iterations == b.attempts[i].iterations,
              "Thread count changed one restart");
}
int main() {
    try {
        lab2::Problem problem;
        problem.outline_width = problem.outline_height = 100;
        for (int i = 0; i < 30; ++i)
            problem.blocks.push_back({std::to_string(i), 0, 0, 2 + i % 4, 3 + i % 7, false});
        lab2::Options options;
        options.iterations = 5000;
        const auto serial = lab2::solve_multi(problem, options, {8, 1});
        for (unsigned threads : {2U, 4U, 8U})
            equal(serial, lab2::solve_multi(problem, options, {8, threads}));
        // A tiny budget can leave some seeds infeasible; retain all outcomes and
        // choose among legal attempts rather than abandoning the whole batch.
        lab2::Problem mixed;
        mixed.outline_width = 5;
        mixed.outline_height = 3;
        mixed.blocks = {{"a", 0, 0, 2, 2, false}, {"b", 0, 0, 3, 3, false}};
        auto short_options = options;
        short_options.iterations = 1;
        const auto partially_legal = lab2::solve_multi(mixed, short_options, {32, 4});
        std::size_t legal = 0;
        for (const auto &attempt : partially_legal.attempts)
            legal += attempt.legal;
        check(legal > 0 && legal < partially_legal.attempts.size(),
              "Mixed-outcome fixture lost coverage");
        equal(partially_legal, lab2::solve_multi(mixed, short_options, {32, 1}));
        // Identical one-block solutions exercise the stable tie-break across workers.
        problem.blocks.resize(1);
        const auto tied = lab2::solve_multi(problem, options, {8, 8});
        check(tied.winning_seed == options.seed, "Tie must choose earliest seed");
        problem.outline_width = problem.outline_height = 1;
        bool failed = false;
        try {
            (void)lab2::solve_multi(problem, options, {8, 4});
        } catch (const lab2::NoLegalPlacement &error) {
            failed = error.iterations == 8 * options.iterations;
        }
        check(failed, "All-infeasible restarts must propagate budget exhaustion");
        options.seed = std::numeric_limits<unsigned>::max();
        failed = false;
        try {
            (void)lab2::solve_multi(problem, options, {2, 1});
        } catch (const std::runtime_error &) {
            failed = true;
        }
        check(failed, "Restart seed overflow accepted");
        std::cout
            << "Multi-start results stable at 1/2/4/8 threads; tie and failure checks passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

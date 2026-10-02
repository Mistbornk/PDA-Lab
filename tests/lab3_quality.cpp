#include "legalizer.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <tuple>
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
bool overlap(const lab3::Cell &a, const lab3::Cell &b) {
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height &&
           b.y < a.y + a.height;
}
lab3::Cell cell(std::string name, double x, double y, double w, double h, bool fixed = false) {
    return {std::move(name), x, y, x, y, w, h, fixed};
}
int main() {
    std::mt19937 rng(71);
    for (int seed = 0; seed < 200; ++seed) {
        lab3::Input input;
        input.alpha = 1;
        input.beta = 2;
        input.die = {-3, 7, 5, 11};
        for (int y = 0; y < 4; ++y) {
            input.rows.push_back({-3, 7. + y, .5, 1, 16});
            for (int x = 0; x < 16; ++x) {
                if ((x == 0 && y == 0) || rng() % 100 < 65)
                    input.cells.push_back(cell(
                        x == 0 && y == 0 ? "bank" : std::to_string(x) + "_" + std::to_string(y),
                        -3 + x * .5, 7. + y, .5, 1, !(x == 0 && y == 0) && rng() % 4 == 0));
            }
        }
        auto target =
            cell("new", -4 + static_cast<double>(rng() % 40) * .25,
                 6 + static_cast<double>(rng() % 12) * .5, .5 * static_cast<double>(1 + rng() % 4),
                 static_cast<double>(1 + rng() % 2));
        std::optional<std::tuple<double, double, double>> expected;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 16; ++x) {
                auto candidate = target;
                candidate.x = -3 + x * .5;
                candidate.y = 7 + y;
                if (candidate.x + candidate.width > input.die.uRX ||
                    candidate.y + candidate.height > input.die.uRY)
                    continue;
                bool free = true;
                for (const auto &old : input.cells)
                    if (old.name != "bank" && overlap(old, candidate))
                        free = false;
                if (!free)
                    continue;
                const auto value = std::make_tuple(std::abs(candidate.x - target.x) +
                                                       std::abs(candidate.y - target.y),
                                                   candidate.y, candidate.x);
                if (!expected || value < *expected)
                    expected = value;
            }
        for (auto strategy : {lab3::Strategy::MinimumDisplacement, lab3::Strategy::Repair}) {
            lab3::Options options;
            options.strategy = strategy;
            options.repair_candidates = 8;
            options.repair_cells = 2;
            options.repair_radius = 3;
            lab3::Legalizer solver(input, options);
            try {
                const auto result = solver.apply({{"bank"}, target});
                solver.validate();
                const auto live = solver.cells();
                for (std::size_t i = 0; i < live.size(); ++i) {
                    for (std::size_t j = 0; j < i; ++j)
                        check(!overlap(live[i], live[j]), "Repair overlaps live cells");
                    for (const auto &old : input.cells)
                        if (old.fixed && old.name == live[i].name)
                            check(old.x == live[i].x && old.y == live[i].y,
                                  "Repair moved FIX cell");
                }
                if (strategy == lab3::Strategy::MinimumDisplacement) {
                    check(expected.has_value(), "Oracle says no site exists");
                    check(result.placed.x == std::get<2>(*expected) &&
                              result.placed.y == std::get<1>(*expected),
                          "Minimum displacement differs from exhaustive site oracle");
                }
                if (expected)
                    check(solver.stats().objective <= 2 * std::get<0>(*expected) + 1e-9,
                          "Repair worsened immediate score");
            } catch (const lab3::NoLegalPlacement &) {
                check(!expected, "Rejected a feasible oracle placement");
                solver.validate();
                check(solver.cells().size() == input.cells.size(), "Failure changed placement");
            }
        }
    }
    lab3::Input repair_input;
    repair_input.alpha = repair_input.beta = 1;
    repair_input.die = {0, 0, 10, 1};
    repair_input.rows = {{0, 0, 1, 1, 10}};
    repair_input.cells = {cell("m", 1, 0, 1, 1), cell("fixed", 3, 0, 5, 1, true),
                          cell("bank", 8, 0, 1, 1)};
    lab3::Options options;
    options.strategy = lab3::Strategy::Repair;
    lab3::Legalizer solver(repair_input, options);
    auto first = solver.apply({{"bank"}, cell("new", 0, 0, 3, 1)});
    check(first.placed.x == 0 && first.moved.size() == 1 && first.moved[0].x == 8,
          "Repair did not create a gap");
    check(solver.stats().objective == 8, "Wrong first-step objective");
    solver.validate();
    auto second = solver.apply({{"new"}, cell("next", 8, 0, 2, 1)});
    check(second.placed.x == 8 && second.moved.size() == 1 && second.moved[0].x == 1,
          "Repair did not restore old cell");
    check(solver.stats().moved_cells == 2 && solver.stats().total_distance == 0 &&
              solver.stats().objective == 2,
          "Score should use final displacement and cumulative move count");
    solver.validate();
    lab3::Input reuse;
    reuse.alpha = reuse.beta = 1;
    reuse.die = {0, 0, 10, 1};
    reuse.rows = {{0, 0, 1, 1, 10}};
    reuse.cells = {cell("bank", 0, 0, 1, 1)};
    options.strategy = lab3::Strategy::MinimumDisplacement;
    lab3::Legalizer reused(reuse, options);
    reused.apply({{"bank"}, cell("new", 10, 0, 1, 1)});
    reused.apply({{"new"}, cell("new", 0, 0, 1, 1)});
    check(reused.stats().total_distance == 1, "Reused name replaced evaluator score entry");
    reused.validate();
}

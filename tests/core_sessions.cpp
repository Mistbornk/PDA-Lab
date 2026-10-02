#include "legalizer.hpp"
#include "router.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F function) {
    bool rejected = false;
    try {
        function();
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    check(rejected, "Expected rejection");
}
lab4::Input routing_input() {
    lab4::Input input;
    input.area = {11, 23, 6, 9, 3, 3};
    input.grid = {2, 3};
    input.nets = {{1, 11, 23, 15, 23}, {2, 11, 23, 15, 23}};
    input.costs = {1,
                   1,
                   1,
                   1,
                   1,
                   1,
                   std::vector<std::vector<double>>(3, std::vector<double>(3, 1)),
                   std::vector<std::vector<double>>(3, std::vector<double>(3, 1))};
    input.cells.resize(3);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            input.cells[static_cast<std::size_t>(y)].push_back(
                {1, 1, 0, 0, 11 + 2 * x, 23 + 3 * y});
    return input;
}
int main() {
    std::istringstream input("Alpha 10 Beta 1 DieSize 0 0 6 2\n"
                             "f 0 0 1 1 NOTFIX\nb 3 0 1 1 FIX\n"
                             "PlacementRows 0 0 1 1 6\nPlacementRows 0 1 1 1 6\n");
    lab3::Legalizer legalizer(lab3::parse_placement(input), {});
    legalizer.validate();
    lab3::Step step{{"f"}, {"new", 0, 0, 0, 0, 9, 1, false}};
    rejects([&] { legalizer.apply(step); });
    legalizer.validate();
    check(legalizer.cells().size() == 2 && legalizer.cells()[0].name == "f",
          "Failed banking removed an old cell");
    step.remove = {"b"};
    rejects([&] { legalizer.apply(step); });
    step.remove = {"f", "f"};
    rejects([&] { legalizer.apply(step); });
    step.remove = {"f"};
    step.cell.width = 2;
    auto result = legalizer.apply(step);
    check(result.placed.x == 0 && result.placed.y == 0 && result.moved.empty(), "Wrong first fit");
    legalizer.validate();
    check(legalizer.stats().committed_steps == 1, "Failure committed a step");
    std::ostringstream report;
    lab3::write_step(report, result);
    check(report.str() == "0 0\n0\n", "Changed legacy report");

    auto grid = routing_input();
    const auto routed = lab4::solve_layered(grid);
    check(std::abs(routed.metrics.objective - 19) < 1e-12 && routed.metrics.overflow == 2 &&
              routed.metrics.vias == 4,
          "Wrong known routing objective");
    lab4::RoutingState state(grid);
    for (const auto &route : routed.routes)
        state.replace(route);
    state.validate();
    const auto usage = state.usage();
    for (int i = 0; i < 1000; ++i) {
        auto route = state.remove(lab4::NetId{static_cast<std::size_t>(i % 2)});
        state.validate();
        state.replace(route);
        state.validate();
        check(state.usage() == usage, "Remove/reinsert changed usage");
    }
    auto bad = routed.routes.front();
    bad.states.back() = 99999;
    rejects([&] { state.replace(bad); });
    check(state.usage() == usage, "Invalid replacement changed usage");
    state.validate();
    std::ostringstream first, second;
    lab4::write_report(first, grid, routed);
    lab4::write_report(second, grid, routed);
    check(first.str() == second.str() && state.usage() == usage, "Reporting mutated routing");
}

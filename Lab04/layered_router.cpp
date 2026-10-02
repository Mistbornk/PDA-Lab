#include "pda/io.hpp"
#include "router.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <queue>
#include <tuple>
namespace lab4 {
namespace {
class Search {
    RoutingState &routing;
    const Input &input;
    const std::size_t cols, count, sink;
    std::vector<double> distance;
    std::vector<std::size_t> parent;
    using Entry = std::tuple<double, double, std::size_t>; // f, g, state; stable ties
    std::size_t cell(std::size_t state) const { return state / 2; }
    std::size_t row(std::size_t state) const { return cell(state) / cols; }
    std::size_t col(std::size_t state) const { return cell(state) % cols; }
    // Charge a cell when leaving it, so arrival AND departure layer are known.
    // A turn replaces the cell's layer cost by the average, never double-counts it.
    double cell_cost(std::size_t state, int departure) const {
        const auto &c = input.costs;
        const double m1 = c.layer1_cost[row(state)][col(state)];
        const double m2 = c.layer2_cost[row(state)][col(state)];
        const bool via = static_cast<int>(state % 2) != departure;
        return c.gamma * (via              ? m1 / 2 + m2 / 2
                          : departure == 0 ? m1
                                           : m2) +
               (via ? c.delta * c.via_cost : 0);
    }
    double step_cost(std::size_t a, std::size_t b) {
        const int layer = static_cast<int>(b % 2);
        const auto id = routing.edge(a, b);
        const auto usage = routing.usage()[id];
        const auto capacity = routing.capacity(id);
        // Difference of total overflow before/after adding one unit of usage.
        const double overflow = usage >= capacity ? input.costs.max_cellcost / 2 : 0;
        const int length = layer == 0 ? input.grid.GridHeight : input.grid.GridWidth;
        return cell_cost(a, layer) + input.costs.alpha * length + input.costs.beta * overflow;
    }
    double heuristic(std::size_t state, std::size_t destination) const {
        if (state == sink)
            return 0;
        // All remaining components are nonnegative. Each move reduces this lower
        // bound by at most its weighted wirelength, so the heuristic is consistent.
        const double dx =
            std::abs(static_cast<double>(col(state)) - static_cast<double>(col(destination)));
        const double dy =
            std::abs(static_cast<double>(row(state)) - static_cast<double>(row(destination)));
        return input.costs.alpha * (dx * input.grid.GridWidth + dy * input.grid.GridHeight);
    }

  public:
    explicit Search(RoutingState &state)
        : routing(state), input(state.problem()),
          cols(static_cast<std::size_t>(input.area.num_cols)),
          count(static_cast<std::size_t>(input.area.num_rows) * cols), sink(count * 2),
          distance(sink + 1), parent(sink + 1) {}
    Route route(NetId id) {
        const auto &net = input.nets.at(id.value);
        const auto source = routing.endpoint(net.bump1_x, net.bump1_y);
        const auto destination = routing.endpoint(net.bump2_x, net.bump2_y);
        std::fill(distance.begin(), distance.end(), std::numeric_limits<double>::infinity());
        std::fill(parent.begin(), parent.end(), sink);
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
        distance[source] = 0;
        parent[source] = source;
        open.emplace(heuristic(source, destination), 0, source);
        std::size_t expanded = 0;
        const auto relax = [&](std::size_t from, std::size_t to, double cost) {
            const double candidate = distance[from] + cost;
            if (candidate < distance[to]) {
                distance[to] = candidate;
                parent[to] = from;
                open.emplace(candidate + heuristic(to, destination), candidate, to);
            }
        };
        while (!open.empty()) {
            const auto [f, g, current] = open.top();
            open.pop();
            (void)f;
            if (g != distance[current])
                continue;
            if (current == sink)
                break;
            ++expanded;
            if (cell(current) == cell(destination)) {
                // The sink explicitly charges the final cell and any return-to-M1 via.
                relax(current, sink, cell_cost(current, 0));
            }
            const int r = static_cast<int>(row(current)), c = static_cast<int>(col(current));
            static constexpr std::array<std::pair<int, int>, 4> directions{
                {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
            for (const auto &[dr, dc] : directions) {
                const int nr = r + dr, nc = c + dc;
                if (nr < 0 || nc < 0 || nr >= input.area.num_rows || nc >= input.area.num_cols)
                    continue;
                const auto next =
                    2 * (static_cast<std::size_t>(nr) * cols + static_cast<std::size_t>(nc)) +
                    (dc != 0);
                relax(current, next, step_cost(current, next));
            }
        }
        pda::require(std::isfinite(distance[sink]), "No finite-cost layered path found");
        std::vector<std::size_t> path;
        for (auto state = parent[sink];; state = parent[state]) {
            path.push_back(state);
            if (state == source)
                break;
            pda::require(path.size() <= sink, "Cycle in layered predecessor chain");
        }
        std::reverse(path.begin(), path.end());
        return Route{id, std::move(path), distance[sink], expanded};
    }
};
} // namespace
Result solve_layered(const Input &input) {
    RoutingState state(input);
    Search search(state);
    for (std::size_t i = 0; i < input.nets.size(); ++i)
        state.replace(search.route(NetId{i}));
    return Result{state.routes(), state.metrics()};
}
void route_layered(Input input, std::ostream &output, std::ostream *statistics) {
    const auto result = solve_layered(input);
    write_report(output, input, result);
    if (statistics)
        write_stats(*statistics, result);
}
} // namespace lab4

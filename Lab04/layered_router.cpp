#include "pda/io.hpp"
#include "router.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <queue>
#include <tuple>
#include <unordered_map>
namespace lab4 {
namespace {
class Search {
    RoutingState &routing;
    const Input &input;
    const std::size_t cols, count, sink;
    using Clock = std::chrono::steady_clock;
    Clock::time_point deadline;
    const std::vector<double> *history;
    std::unordered_map<std::size_t, std::uint64_t> omitted;
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
    double step_cost(std::size_t a, std::size_t b, bool penalized = true) {
        const int layer = static_cast<int>(b % 2);
        const auto id = routing.edge(a, b);
        auto usage = routing.usage()[id];
        if (!omitted.empty()) {
            const auto found = omitted.find(id);
            if (found != omitted.end())
                usage -= found->second;
        }
        const auto capacity = routing.capacity(id);
        // Difference of total overflow before/after adding one unit of usage.
        const double overflow = usage >= capacity ? input.costs.max_cellcost / 2 : 0;
        const int length = layer == 0 ? input.grid.GridHeight : input.grid.GridWidth;
        return cell_cost(a, layer) + input.costs.alpha * length + input.costs.beta * overflow +
               (penalized && history ? (*history)[id] : 0);
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
    explicit Search(RoutingState &state, Clock::time_point until = Clock::time_point::max(),
                    const std::vector<double> *penalties = nullptr)
        : routing(state), input(state.problem()),
          cols(static_cast<std::size_t>(input.area.num_cols)),
          count(static_cast<std::size_t>(input.area.num_rows) * cols), sink(count * 2),
          deadline(until), history(penalties), distance(sink + 1), parent(sink + 1) {}
    Route route(NetId id) {
        const auto &net = input.nets.at(id.value);
        omitted.clear();
        const auto &old = routing.routes()[id.value].states;
        for (std::size_t i = 1; i < old.size(); ++i)
            ++omitted[routing.edge(old[i - 1], old[i])];
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
            if (deadline != Clock::time_point::max() && (expanded & 255u) == 0 &&
                Clock::now() >= deadline)
                throw RoutingBudgetExceeded();
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
        double original_cost = cell_cost(path.back(), 0);
        for (std::size_t i = 1; i < path.size(); ++i)
            original_cost += step_cost(path[i - 1], path[i], false);
        return Route{id, std::move(path), original_cost, expanded};
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
Result solve_negotiated(const Input &input, const RerouteOptions &options) {
    pda::input_require(options.rounds <= 1000 && options.stagnation > 0 &&
                           options.stagnation <= 1000 && pda::nonnegative(options.history) &&
                           pda::nonnegative(options.seconds) && options.seconds <= 86400,
                       "Invalid rerouting budget/penalty");
    using Clock = std::chrono::steady_clock;
    const auto began = Clock::now();
    const auto deadline = options.seconds > 0
                              ? began + std::chrono::duration_cast<Clock::duration>(
                                            std::chrono::duration<double>(options.seconds))
                              : Clock::time_point::max();
    const auto elapsed = [&] {
        return std::chrono::duration<double>(Clock::now() - began).count();
    };
    RoutingState state(input);
    std::vector<double> history(state.usage().size());
    Search search(state, deadline, &history);
    for (std::size_t i = 0; i < input.nets.size(); ++i)
        state.replace(search.route(NetId{i}));
    Result best{state.routes(), state.metrics()};
    best.iterations.push_back({0, best.metrics, elapsed(), 0});
    const double scale =
        std::max(input.costs.alpha * std::min(input.grid.GridWidth, input.grid.GridHeight),
                 input.costs.beta * (input.costs.max_cellcost / 2));
    unsigned stalled = 0;
    for (unsigned round = 1; round <= options.rounds; ++round) {
        if (Clock::now() >= deadline) {
            best.budget_exhausted = true;
            break;
        }
        std::vector<std::pair<std::uint64_t, std::size_t>> order;
        for (const auto &route : state.routes()) {
            std::uint64_t congestion = 0;
            for (std::size_t i = 1; i < route.states.size(); ++i) {
                const auto edge = state.edge(route.states[i - 1], route.states[i]);
                const auto usage = state.usage()[edge], capacity = state.capacity(edge);
                congestion += usage > capacity ? usage - capacity : 0;
            }
            // Revisit every net: lower-cost routes can exist even without overflow.
            order.emplace_back(congestion, route.net.value);
        }
        std::sort(order.begin(), order.end(), [](const auto &a, const auto &b) {
            return a.first != b.first ? a.first > b.first : a.second < b.second;
        });
        for (std::size_t i = 0; i < history.size(); ++i) {
            if (state.usage()[i] > state.capacity(i)) {
                history[i] += options.history * scale *
                              static_cast<double>(state.usage()[i] - state.capacity(i));
                pda::require(std::isfinite(history[i]), "Nonfinite congestion history");
            }
        }
        std::size_t rerouted = 0;
        for (const auto &[congestion, id] : order) {
            (void)congestion;
            try {
                // Search subtracts the old route virtually. A timeout therefore
                // leaves the existing complete solution untouched.
                auto candidate = search.route(NetId{id});
                state.replace(std::move(candidate));
                ++rerouted;
            } catch (const RoutingBudgetExceeded &) {
                best.budget_exhausted = true;
                break;
            }
        }
        const auto metrics = state.metrics();
        best.iterations.push_back({round, metrics, elapsed(), rerouted});
        if (metrics.objective < best.metrics.objective) {
            best.routes = state.routes();
            best.metrics = metrics;
            stalled = 0;
        } else
            ++stalled;
        if (best.budget_exhausted || stalled >= options.stagnation)
            break;
    }
    return best;
}
void route_layered(Input input, std::ostream &output, std::ostream *statistics) {
    const auto result = solve_layered(input);
    write_report(output, input, result);
    if (statistics)
        write_stats(*statistics, result);
}
} // namespace lab4

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
    Input &input;
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
    GCell &edge(std::size_t a, std::size_t b) {
        return input.cells[std::max(row(a), row(b))][std::max(col(a), col(b))];
    }
    double step_cost(std::size_t a, std::size_t b) {
        const int layer = static_cast<int>(b % 2);
        const auto &e = edge(a, b);
        const int usage = layer == 0 ? e.bottom_usage : e.left_usage;
        const int capacity = layer == 0 ? e.bottom_capacity : e.left_capacity;
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
        const double dx = std::abs(static_cast<double>(col(state)) - col(destination));
        const double dy = std::abs(static_cast<double>(row(state)) - row(destination));
        return input.costs.alpha * (dx * input.grid.GridWidth + dy * input.grid.GridHeight);
    }
    std::size_t state_at(int x, int y) const {
        return 2 *
               (static_cast<std::size_t>((y - input.area.Routing_Area_Y) / input.grid.GridHeight) *
                    cols +
                static_cast<std::size_t>((x - input.area.Routing_Area_X) / input.grid.GridWidth));
    }
    void emit(const std::vector<std::size_t> &path, std::ostream &output) {
        std::size_t anchor = path.front();
        int layer = 0;
        const auto segment = [&](std::size_t end) {
            const auto &a = input.cells[row(anchor)][col(anchor)];
            const auto &b = input.cells[row(end)][col(end)];
            output << 'M' << layer + 1 << ' ' << a.x << ' ' << a.y << ' ' << b.x << ' ' << b.y
                   << '\n';
        };
        for (std::size_t i = 1; i < path.size(); ++i) {
            const int next = static_cast<int>(path[i] % 2);
            if (next != layer) {
                if (i > 1)
                    segment(path[i - 1]);
                output << "via\n";
                anchor = path[i - 1];
                layer = next;
            }
            auto &e = edge(path[i - 1], path[i]);
            if (next == 0)
                ++e.bottom_usage;
            else
                ++e.left_usage;
        }
        segment(path.back());
        if (layer != 0)
            output << "via\n";
    }

  public:
    explicit Search(Input &data)
        : input(data), cols(data.area.num_cols),
          count(static_cast<std::size_t>(data.area.num_rows) * cols), sink(count * 2),
          distance(sink + 1), parent(sink + 1) {}
    void route(const Net &net, std::ostream &output, std::ostream *statistics) {
        const auto source = state_at(net.bump1_x, net.bump1_y);
        const auto destination = state_at(net.bump2_x, net.bump2_y);
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
        output << 'n' << net.idx << '\n';
        emit(path, output);
        output << ".end\n";
        if (statistics)
            *statistics << std::setprecision(17) << "{\"net\":" << net.idx
                        << ",\"incremental_cost\":" << distance[sink]
                        << ",\"expanded\":" << expanded << "}\n";
    }
};
} // namespace
void route_layered(Input input, std::ostream &output, std::ostream *statistics) {
    Search search(input);
    for (const auto &net : input.nets)
        search.route(net, output, statistics);
}
} // namespace lab4

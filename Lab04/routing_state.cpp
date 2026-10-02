#include "pda/io.hpp"
#include "router.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <unordered_map>

namespace lab4 {
RoutingState::RoutingState(const Input &input)
    : data(input), usage_(static_cast<std::size_t>(input.area.num_rows) *
                          static_cast<std::size_t>(input.area.num_cols) * 2),
      routes_(input.nets.size()) {
    for (std::size_t i = 0; i < routes_.size(); ++i)
        routes_[i].net = NetId{i};
}
std::size_t RoutingState::endpoint(int x, int y) const {
    const auto row = (static_cast<long long>(y) - data.area.Routing_Area_Y) / data.grid.GridHeight;
    const auto col = (static_cast<long long>(x) - data.area.Routing_Area_X) / data.grid.GridWidth;
    return 2 * (static_cast<std::size_t>(row) * static_cast<std::size_t>(data.area.num_cols) +
                static_cast<std::size_t>(col));
}
std::size_t RoutingState::edge(std::size_t a, std::size_t b) const {
    const auto cols = static_cast<std::size_t>(data.area.num_cols);
    return std::max(a / 2, b / 2) * 2 + (a / 2 / cols == b / 2 / cols);
}
std::uint64_t RoutingState::capacity(std::size_t id) const {
    const auto cols = static_cast<std::size_t>(data.area.num_cols);
    const auto &cell = data.cells[id / 2 / cols][id / 2 % cols];
    return static_cast<std::uint64_t>(id % 2 ? cell.left_capacity : cell.bottom_capacity);
}
void RoutingState::validate_route(const Route &route) const {
    pda::require(route.net.value < routes_.size() && !route.states.empty(),
                 "Invalid route ID/path");
    const auto &net = data.nets[route.net.value];
    pda::require(route.states.front() == endpoint(net.bump1_x, net.bump1_y) &&
                     route.states.back() / 2 == endpoint(net.bump2_x, net.bump2_y) / 2,
                 "Route endpoints do not match net");
    const auto cols = static_cast<std::size_t>(data.area.num_cols);
    for (std::size_t i = 0; i < route.states.size(); ++i) {
        const auto b = route.states[i];
        pda::require(b < usage_.size(), "Route outside grid");
        if (!i)
            continue;
        const auto a = route.states[i - 1];
        const auto ar = a / 2 / cols, ac = a / 2 % cols;
        const auto br = b / 2 / cols, bc = b / 2 % cols;
        const auto dy = std::max(ar, br) - std::min(ar, br);
        const auto dx = std::max(ac, bc) - std::min(ac, bc);
        pda::require(dx + dy == 1 && b % 2 == (dx != 0), "Nonadjacent or wrong-layer route step");
    }
}
void RoutingState::replace(Route route) {
    validate_route(route);
    auto &old = routes_[route.net.value];
    // Allocate and validate the full delta before touching shared usage or routes.
    std::unordered_map<std::size_t, std::int64_t> delta;
    for (std::size_t i = 1; i < old.states.size(); ++i)
        --delta[edge(old.states[i - 1], old.states[i])];
    for (std::size_t i = 1; i < route.states.size(); ++i)
        ++delta[edge(route.states[i - 1], route.states[i])];
    for (const auto &[id, change] : delta) {
        pda::require(change >= 0 || usage_[id] >= static_cast<std::uint64_t>(-change),
                     "Usage underflow");
        pda::require(change <= 0 || usage_[id] <= std::numeric_limits<std::uint64_t>::max() -
                                                      static_cast<std::uint64_t>(change),
                     "Usage overflow");
    }
    for (const auto &[id, change] : delta) {
        if (change < 0)
            usage_[id] -= static_cast<std::uint64_t>(-change);
        else
            usage_[id] += static_cast<std::uint64_t>(change);
    }
    old = std::move(route);
}
Route RoutingState::remove(NetId net) {
    auto &old = routes_.at(net.value);
    for (std::size_t i = 1; i < old.states.size(); ++i)
        --usage_[edge(old.states[i - 1], old.states[i])];
    Route result = std::move(old);
    old = Route{};
    old.net = net;
    return result;
}
void RoutingState::validate() const {
    std::vector<std::uint64_t> expected(usage_.size());
    for (const auto &route : routes_) {
        if (route.states.empty())
            continue;
        validate_route(route);
        for (std::size_t i = 1; i < route.states.size(); ++i)
            ++expected[edge(route.states[i - 1], route.states[i])];
    }
    pda::require(expected == usage_, "Paths and cached edge usage disagree");
}
Metrics RoutingState::metrics() const {
    Metrics result;
    const auto cols = static_cast<std::size_t>(data.area.num_cols);
    for (const auto &route : routes_) {
        for (std::size_t i = 0; i < route.states.size(); ++i) {
            const auto state = route.states[i];
            const auto next = i + 1 < route.states.size() ? route.states[i + 1] % 2 : 0;
            const auto r = state / 2 / cols, c = state / 2 % cols;
            const auto m1 = data.costs.layer1_cost[r][c], m2 = data.costs.layer2_cost[r][c];
            if (state % 2 != next) {
                ++result.vias;
                result.cell_cost += m1 / 2 + m2 / 2;
            } else
                result.cell_cost += next ? m2 : m1;
            if (i + 1 < route.states.size())
                result.wirelength += next ? data.grid.GridWidth : data.grid.GridHeight;
        }
    }
    for (std::size_t i = 0; i < usage_.size(); ++i) {
        const auto excess = usage_[i] > capacity(i) ? usage_[i] - capacity(i) : 0;
        result.overflow += excess;
        result.max_overflow = std::max(result.max_overflow, excess);
    }
    const auto &c = data.costs;
    result.objective = c.alpha * result.wirelength + c.gamma * result.cell_cost +
                       c.delta * c.via_cost * static_cast<double>(result.vias) +
                       c.beta * (c.max_cellcost / 2) * static_cast<double>(result.overflow);
    pda::require(std::isfinite(result.objective), "Nonfinite routing objective");
    return result;
}
void write_report(std::ostream &output, const Input &input, const Result &result) {
    const auto cols = static_cast<std::size_t>(input.area.num_cols);
    const auto at = [&](std::size_t state) -> const GCell & {
        return input.cells[state / 2 / cols][state / 2 % cols];
    };
    for (const auto &route : result.routes) {
        pda::require(!route.states.empty(), "Cannot emit an incomplete routing result");
        output << 'n' << input.nets.at(route.net.value).idx << '\n';
        const auto &path = route.states;
        std::size_t anchor = path.front(), layer = 0;
        const auto segment = [&](std::size_t end) {
            const auto &a = at(anchor), &b = at(end);
            output << 'M' << layer + 1 << ' ' << a.x << ' ' << a.y << ' ' << b.x << ' ' << b.y
                   << '\n';
        };
        for (std::size_t i = 1; i < path.size(); ++i) {
            const auto next = path[i] % 2;
            if (next != layer) {
                if (i > 1)
                    segment(path[i - 1]);
                output << "via\n";
                anchor = path[i - 1];
                layer = next;
            }
        }
        segment(path.back());
        if (layer != 0)
            output << "via\n";
        output << ".end\n";
    }
}
void write_stats(std::ostream &output, const Result &result) {
    for (const auto &route : result.routes)
        output << std::setprecision(17) << "{\"net\":" << route.net.value + 1
               << ",\"incremental_cost\":" << route.incremental_cost
               << ",\"expanded\":" << route.expanded << "}\n";
}
} // namespace lab4

#pragma once
#include "input.hpp"
#include <cstdint>
#include <iosfwd>
namespace lab4 {
struct NetId {
    std::size_t value;
};
struct Route {
    NetId net{};
    // state = 2 * row-major cell + arrival layer (0=M1, 1=M2).
    std::vector<std::size_t> states;
    double incremental_cost = 0;
    std::size_t expanded = 0;
};
struct Metrics {
    double objective = 0, wirelength = 0, cell_cost = 0;
    std::uint64_t vias = 0, overflow = 0, max_overflow = 0;
};
struct Result {
    std::vector<Route> routes;
    Metrics metrics;
};
// Borrows a validated, immutable Input which must outlive this session.
class RoutingState {
    const Input &data;
    std::vector<std::uint64_t> usage_;
    std::vector<Route> routes_;

  public:
    explicit RoutingState(const Input &input);
    RoutingState(Input &&) = delete;
    const Input &problem() const { return data; }
    const std::vector<Route> &routes() const { return routes_; }
    const std::vector<std::uint64_t> &usage() const { return usage_; }
    std::size_t endpoint(int x, int y) const;
    std::size_t edge(std::size_t a, std::size_t b) const;
    std::uint64_t capacity(std::size_t edge_id) const;
    void replace(Route route);
    Route remove(NetId net);
    void validate_route(const Route &route) const;
    void validate() const;
    Metrics metrics() const;
};
Result solve_legacy(const Input &input);
Result solve_layered(const Input &input);
void write_report(std::ostream &output, const Input &input, const Result &result);
void write_stats(std::ostream &output, const Result &result);
void route_legacy(Input input, std::ostream &output);
// Greedy net order is preserved. Each net minimizes incremental cost with prior
// usages fixed, on a (cell, arrival layer) graph. This is not joint net optimization.
void route_layered(Input input, std::ostream &output, std::ostream *statistics = nullptr);
} // namespace lab4

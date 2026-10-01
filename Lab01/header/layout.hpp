#pragma once
#include <cstdint>
#include <iosfwd>
#include <utility>
#include <vector>
namespace lab1 {
struct Command {
    int id, x, y, width, height;
};
struct Input {
    int width = 0, height = 0;
    std::vector<Command> commands;
};
struct NeighborCount {
    int id, solid, space;
};
struct Result {
    std::size_t tiles = 0;
    std::vector<NeighborCount> neighbors;
    std::vector<std::pair<int, int>> points;
    std::uint64_t stitch_queries = 0, candidate_visits = 0;
};
Input parse(std::istream &input);
Result solve(const Input &input, bool indexed = true);
void write_report(std::ostream &output, const Result &result);
} // namespace lab1

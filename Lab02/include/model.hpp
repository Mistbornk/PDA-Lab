#pragma once
#include <cassert>
#include <cstdint>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
namespace lab2 {
struct Block {
    std::string name;
    int x = 0, y = 0, width = 0, height = 0;
    bool rotate = false;
};
struct Terminal {
    std::string name;
    int x = 0, y = 0;
};
struct Node {
    int parent = -1, leftChild = -1, rightChild = -1;
};
struct Cost {
    int width = 0, height = 0;
    long long area = 0, wirelength = 0, cost = 0;
};
struct Problem {
    int outline_width = 0, outline_height = 0;
    std::vector<Block> blocks;
    std::vector<Terminal> terminals;
    std::unordered_map<std::string, int> pin_ids;
    std::vector<std::vector<std::string>> nets;
    std::vector<std::vector<int>> net_ids;
};
struct Placement {
    int root = -1;
    std::vector<Block> blocks;
    std::vector<Node> tree;
    // Tree links use -1 for absence. Callers check that sentinel before access;
    // keep signed ID conversion and debug bounds checking at one boundary.
    Node &node(int id) {
        assert(id >= 0 && static_cast<std::size_t>(id) < tree.size());
        return tree[static_cast<std::size_t>(id)];
    }
    const Node &node(int id) const {
        assert(id >= 0 && static_cast<std::size_t>(id) < tree.size());
        return tree[static_cast<std::size_t>(id)];
    }
    Block &block(int id) {
        assert(id >= 0 && static_cast<std::size_t>(id) < blocks.size());
        return blocks[static_cast<std::size_t>(id)];
    }
    const Block &block(int id) const {
        assert(id >= 0 && static_cast<std::size_t>(id) < blocks.size());
        return blocks[static_cast<std::size_t>(id)];
    }
};
enum class PackingMode { Dense, Skyline };
enum class SearchPolicy { Legacy, Progress, FeasibilityFirst };
struct PolicyOptions {
    double temperature = 0.05, cooling = 0.9, outline_weight = 10;
    unsigned epoch_moves = 20, reheat_epochs = 100;
};
struct EvaluationProfile {
    std::uint64_t samples = 0;
    double packing_seconds = 0, hpwl_seconds = 0;
};
struct TracePoint {
    std::uint64_t iteration = 0;
    double cpu_seconds = 0, outline_excess = 0;
    long long best_objective = -1;
};
struct SearchStats {
    double first_legal_cpu_seconds = -1, minimum_outline_excess = -1;
    std::uint64_t first_legal_iteration = 0, accepted = 0, uphill_accepted = 0;
    EvaluationProfile evaluation;
    std::vector<TracePoint> trace{};
    bool trace_truncated = false;
};
struct Options {
    double alpha = 0.5, seconds = 280.0;
    unsigned seed = 1;
    std::uint64_t iterations = 0;
    bool integer_pins = true;
    PackingMode packing = PackingMode::Skyline;
    bool undo_journal = true;
    SearchPolicy policy = SearchPolicy::Legacy;
    PolicyOptions schedule;
    bool diagnostics = false;
    std::uint64_t trace_every = 0;
};
struct Result {
    std::vector<Block> blocks;
    Cost cost;
    std::uint64_t iterations = 0;
    SearchStats stats{};
};
} // namespace lab2

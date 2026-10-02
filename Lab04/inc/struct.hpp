#pragma once
#include <utility>
#include <vector>

namespace lab4 {
struct Chip {
    int x, y;
    int width, height;
};

struct Net {
    int idx;
    int bump1_x, bump1_y;
    int bump2_x, bump2_y;
};

struct RoutingAreaInfo {
    int Routing_Area_X, Routing_Area_Y, Routing_Area_Width, Routing_Area_Height;
    int num_rows, num_cols;
};

struct GridInfo {
    int GridWidth, GridHeight;
};

struct CostInfo {
    double alpha, beta, gamma, delta;
    double via_cost;
    double max_cellcost;
    std::vector<std::vector<double>> layer1_cost;
    std::vector<std::vector<double>> layer2_cost;
};

struct GCell {
    int left_capacity, bottom_capacity;
    int left_usage, bottom_usage;
    int x, y;
};

struct Node {
    int layer = 0;
    double f, g, h;
    int parent_i, parent_j;
};

// Creating a shortcut for int, int pair type
typedef std::pair<int, int> Pair;

// Creating a shortcut for pair<int, pair<int, int>> type
typedef std::pair<double, std::pair<int, int>> pPair;

} // namespace lab4

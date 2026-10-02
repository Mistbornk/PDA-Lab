#pragma once
#include "struct.hpp"
#include <istream>
#include <limits>
#include <string>
namespace lab4 {
struct Input {
    RoutingAreaInfo area{};
    GridInfo grid{};
    std::vector<Net> nets;
    std::vector<std::vector<GCell>> cells;
    CostInfo costs{};
};
Input parse(const std::string &gmp, const std::string &gcl, const std::string &cst);
Input parse(std::istream &gmp, std::istream &gcl, std::istream &cst,
            std::size_t max_cells = std::numeric_limits<std::size_t>::max() / 2);
} // namespace lab4

#pragma once
#include "struct.hpp"
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
} // namespace lab4

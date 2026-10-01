#pragma once
#include "struct.hpp"
#include <istream>
#include <ostream>
#include <vector>

namespace lab3 {
struct Step {
    std::vector<std::string> remove;
    Cell cell{};
};
struct Input {
    double alpha{}, beta{};
    Die die{};
    std::vector<Cell> cells;
    std::vector<PlacementRow> rows;
};
Input parse_placement(std::istream &input);
std::vector<Step> parse_steps(std::istream &input);
struct Options {
    bool nearest = false;
    bool intervals = true;
    bool stats = false;
};
void legalize(Input input, const std::vector<Step> &steps, std::ostream &output, Options options);
} // namespace lab3

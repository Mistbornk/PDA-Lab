#pragma once
#include "model.hpp"
namespace lab2 {
// Precondition: placement is a valid rooted B*-tree. Packing changes x/y only.
void pack(Placement &placement, int initial_contour_width);
long long hpwl(const Problem &problem, const std::vector<Block> &blocks, bool integer_pins = true);
Cost evaluate(const Problem &problem, Placement &placement, double alpha, bool outline_penalty,
              bool integer_pins = true);
} // namespace lab2

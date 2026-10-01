#pragma once
#include "model.hpp"
#include <random>
namespace lab2 {
// Node indices are stable block IDs. Mutation requires a valid tree and distinct IDs.
void initialize_tree(Placement &placement, std::mt19937 &random);
void rotate_block(Placement &placement, int id);
void swap_nodes(Placement &placement, int first, int second);
// Random source is explicit so primitive tests do not depend on the annealer.
void move_node(Placement &placement, int from, int to, int (*random)());
// O(n) structural validator, intentionally outside the search hot loop.
bool valid_tree(const Placement &placement);
} // namespace lab2

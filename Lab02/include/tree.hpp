#pragma once
#include "model.hpp"
#include "random.hpp"
#include "undo.hpp"
#include <random>
namespace lab2 {
// Node indices are stable block IDs. Mutation requires a valid tree and distinct IDs.
void initialize_tree(Placement &placement, std::mt19937 &random);
void rotate_block(Placement &placement, int id, UndoJournal *undo = nullptr);
void swap_nodes(Placement &placement, int first, int second, UndoJournal *undo = nullptr);
// Random source is explicit so primitive tests do not depend on the annealer.
void move_node(Placement &placement, int from, int to, Random &random, UndoJournal *undo = nullptr);
// O(n) structural validator, intentionally outside the search hot loop.
bool valid_tree(const Placement &placement);
} // namespace lab2

#include "undo.hpp"
#include <algorithm>
namespace lab2 {
void UndoJournal::begin(const Placement &p) {
    if (stamps.size() != p.blocks.size())
        stamps.assign(p.blocks.size(), 0);
    if (++epoch == 0) {
        std::fill(stamps.begin(), stamps.end(), 0);
        epoch = 1;
    }
    entries.clear();
    entries.reserve(p.blocks.size());
    root = p.root;
}
void UndoJournal::remember(const Placement &p, int id) {
    if (id < 0)
        return;
    const auto index = static_cast<std::size_t>(id);
    if (stamps[index] == epoch)
        return;
    stamps[index] = epoch;
    const auto &block = p.blocks[index];
    entries.push_back({index, p.tree[index], block.width, block.height, block.rotate});
}
void UndoJournal::neighborhood(const Placement &p, int id) {
    remember(p, id);
    const auto &node = p.tree[static_cast<std::size_t>(id)];
    remember(p, node.parent);
    remember(p, node.leftChild);
    remember(p, node.rightChild);
}
void UndoJournal::rollback(Placement &p) const {
    p.root = root;
    for (const auto &entry : entries) {
        p.tree[entry.id] = entry.node;
        auto &block = p.blocks[entry.id];
        block.width = entry.width;
        block.height = entry.height;
        block.rotate = entry.rotate;
    }
}
} // namespace lab2

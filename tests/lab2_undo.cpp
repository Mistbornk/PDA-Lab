#include "packing.hpp"
#include "tree.hpp"
#include <iostream>
#include <stdexcept>
using namespace lab2;
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
int main() {
    try {
        std::mt19937 initialization(1701);
        Random random(43);
        Placement p;
        for (int i = 0; i < 40; ++i)
            p.blocks.push_back({std::to_string(i), 0, 0, 1 + i % 7, 1 + i % 5, false});
        initialize_tree(p, initialization);
        PackingWorkspace workspace;
        UndoJournal undo;
        for (int trial = 0; trial < 10000; ++trial) {
            pack(p, 1, workspace);
            const auto before = p;
            undo.begin(p);
            for (int mutation = 0; mutation <= trial % 7; ++mutation) {
                int a = random() % 40, b = (a + 1 + random() % 39) % 40;
                switch (random() % 3) {
                case 0:
                    rotate_block(p, a, &undo);
                    break;
                case 1:
                    swap_nodes(p, a, b, &undo);
                    break;
                case 2:
                    move_node(p, a, b, random, &undo);
                    break;
                }
                check(valid_tree(p), "Journal mutation broke tree");
                pack(p, 1, workspace);
            }
            if (trial % 3 == 0)
                continue; // Commit and use this structure in later trials.
            undo.rollback(p);
            pack(p, 1, workspace);
            check(p.root == before.root, "Root rollback differs");
            for (std::size_t id = 0; id < p.blocks.size(); ++id) {
                const auto &a = p.blocks[id];
                const auto &b = before.blocks[id];
                const auto &x = p.tree[id];
                const auto &y = before.tree[id];
                check(a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height &&
                          a.rotate == b.rotate && a.name == b.name,
                      "Geometry rollback differs");
                check(x.parent == y.parent && x.leftChild == y.leftChild &&
                          x.rightChild == y.rightChild,
                      "Structural rollback differs");
            }
        }
        std::cout << "10,000 multi-mutation journal transactions match full snapshots\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

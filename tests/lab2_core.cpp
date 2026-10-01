#include "packing.hpp"
#include "tree.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace lab2;
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
// Independent packing oracle: visit the tree in preorder, place atop all overlapping
// already placed rectangles. It has no coordinate contour and runs in O(n^2).
void check_packing(const Placement &p) {
    std::vector<int> pending{p.root};
    std::vector<Block> placed;
    while (!pending.empty()) {
        int id = pending.back();
        pending.pop_back();
        const auto &b = p.blocks[id];
        const auto &node = p.tree[id];
        int x = 0;
        if (node.parent != -1) {
            const auto &parent = p.blocks[node.parent];
            x = parent.x + (p.tree[node.parent].leftChild == id ? parent.width : 0);
        }
        int y = 0;
        for (const auto &old : placed)
            if (std::max(x, old.x) < std::min(x + b.width, old.x + old.width))
                y = std::max(y, old.y + old.height);
        check(b.x == x && b.y == y, "Packing differs from independent rectangle oracle");
        placed.push_back(b);
        if (node.rightChild != -1)
            pending.push_back(node.rightChild);
        if (node.leftChild != -1)
            pending.push_back(node.leftChild);
    }
}
int main() {
    try {
        std::mt19937 rng(73);
        lab2::Random mutation_random(19);
        Placement p;
        for (int i = 0; i < 20; ++i)
            p.blocks.push_back({std::to_string(i), 0, 0, 1 + i % 7, 1 + i % 5, false});
        initialize_tree(p, rng);
        for (int i = 0; i < 20000; ++i) {
            const int a = rng() % 20, b = (a + 1 + rng() % 19) % 20;
            switch (i % 3) {
            case 0:
                rotate_block(p, a);
                break;
            case 1:
                swap_nodes(p, a, b);
                break;
            case 2:
                move_node(p, a, b, mutation_random);
                break;
            }
            check(valid_tree(p), "Mutation broke root/parent/reachability invariant");
            pack(p, 1);
            check_packing(p);
        }
        auto broken = p;
        broken.tree[broken.root].leftChild = broken.root;
        check(!valid_tree(broken), "Cycle accepted");
        broken = p;
        broken.tree[broken.root].leftChild = 99;
        check(!valid_tree(broken), "Out-of-range child accepted");
        Problem problem;
        problem.blocks = {{"a", 0, 0, 3, 5, false}};
        problem.terminals = {
            {"p", std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}};
        problem.pin_ids = {{"a", 0}, {"p", 1}};
        problem.nets = {{"a", "p"}};
        problem.net_ids = {{0, 1}};
        check(hpwl(problem, problem.blocks) == 4294967294LL, "HPWL must use 64-bit differences");
        check(hpwl(problem, problem.blocks) == hpwl(problem, problem.blocks, false),
              "Pin modes differ");
        std::cout << "20,000 tree mutations and independent packing checks passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

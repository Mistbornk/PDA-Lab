#include "tree.hpp"
#include <algorithm>
#include <numeric>
using namespace std;
namespace lab2 {
void rotate_block(Placement &p, int current, UndoJournal *undo) {
    if (undo)
        undo->remember(p, current);
    swap(p.block(current).height, p.block(current).width);
    p.block(current).rotate = (p.block(current).rotate) ? false : true;
}
static void swapParent(Placement &p, int node1, int node2) {
    // swap parent
    int node1Parent = p.node(node1).parent;
    int node2Parent = p.node(node2).parent;
    if (node1Parent != -1) {
        (p.node(node1Parent).leftChild == node1) ? p.node(node1Parent).leftChild = node2
                                                 : p.node(node1Parent).rightChild = node2;
    }
    if (node2Parent != -1) {
        (p.node(node2Parent).leftChild == node2) ? p.node(node2Parent).leftChild = node1
                                                 : p.node(node2Parent).rightChild = node1;
    }
    swap(p.node(node1).parent, p.node(node2).parent);
}
static void swapChild(Placement &p, int node1, int node2) {
    // swap children
    swap(p.node(node1).leftChild, p.node(node2).leftChild);
    swap(p.node(node1).rightChild, p.node(node2).rightChild);

    if (p.node(node1).leftChild != -1)
        p.node(p.node(node1).leftChild).parent = node1;
    if (p.node(node1).rightChild != -1)
        p.node(p.node(node1).rightChild).parent = node1;

    if (p.node(node2).leftChild != -1)
        p.node(p.node(node2).leftChild).parent = node2;
    if (p.node(node2).rightChild != -1)
        p.node(p.node(node2).rightChild).parent = node2;
}
void swap_nodes(Placement &p, int node1, int node2, UndoJournal *undo) {
    if (undo) {
        undo->neighborhood(p, node1);
        undo->neighborhood(p, node2);
    }
    // swap parent
    swapParent(p, node1, node2);

    // swap child
    swapChild(p, node1, node2);

    // relationship of node1 and node2 are parent and child
    if (p.node(node1).parent == node1)
        p.node(node1).parent = node2;
    else if (p.node(node1).leftChild == node1)
        p.node(node1).leftChild = node2;
    else if (p.node(node1).rightChild == node1)
        p.node(node1).rightChild = node2;

    if (p.node(node2).parent == node2)
        p.node(node2).parent = node1;
    else if (p.node(node2).leftChild == node2)
        p.node(node2).leftChild = node1;
    else if (p.node(node2).rightChild == node2)
        p.node(node2).rightChild = node1;

    // change root
    if (p.root == node1)
        p.root = node2;
    else if (p.root == node2)
        p.root = node1;
}
void move_node(Placement &p, int from, int to, Random &random, UndoJournal *undo) {
    if (undo)
        undo->neighborhood(p, from);
    // delete the node
    if (p.node(from).leftChild == -1 && p.node(from).rightChild == -1) {
        // if no child then directly remove
        int fromParent = p.node(from).parent;
        if (fromParent != -1) {
            (p.node(fromParent).leftChild == from) ? p.node(fromParent).leftChild = -1
                                                   : p.node(fromParent).rightChild = -1;
        }
    } else if (p.node(from).leftChild != -1 && p.node(from).rightChild != -1) {
        // if has two child
        while (true) {
            bool swapLeft = false;
            if (p.node(from).leftChild != -1 && p.node(from).rightChild != -1)
                swapLeft = (random() % 2 == 0);
            else if (p.node(from).leftChild != -1)
                swapLeft = true;

            if (swapLeft) {
                swap_nodes(p, from, p.node(from).leftChild, undo);
            } else {
                swap_nodes(p, from, p.node(from).rightChild, undo);
            }
            if (p.node(from).leftChild == -1 && p.node(from).rightChild == -1)
                break;
        }
        int fromParent = p.node(from).parent;
        if (fromParent != -1) {
            (p.node(fromParent).leftChild == from) ? p.node(fromParent).leftChild = -1
                                                   : p.node(fromParent).rightChild = -1;
        }
    } else {
        // if only one child
        int fromParent = p.node(from).parent;
        int fromChild =
            (p.node(from).leftChild != -1) ? p.node(from).leftChild : p.node(from).rightChild;

        p.node(fromChild).parent = fromParent;
        if (fromParent != -1) {
            (p.node(fromParent).leftChild == from) ? p.node(fromParent).leftChild = fromChild
                                                   : p.node(fromParent).rightChild = fromChild;
        }
        p.node(fromChild).parent = fromParent;
        if (p.root == from)
            p.root = fromChild;
    }

    // insert the node
    if (undo)
        undo->neighborhood(p, to);
    int op = random() % 2;
    int toChild = (op == 0) ? p.node(to).leftChild : p.node(to).rightChild;
    switch (op) {
    case 0: {
        p.node(to).leftChild = from;
        break;
    }
    case 1: {
        p.node(to).rightChild = from;
        break;
    }
    }
    op = random() % 2;
    switch (op) {
    case 0: {
        p.node(from).leftChild = toChild;
        p.node(from).rightChild = -1;
        break;
    }
    case 1: {
        p.node(from).rightChild = toChild;
        p.node(from).leftChild = -1;
        break;
    }
    }
    p.node(from).parent = to;
    if (toChild != -1)
        p.node(toChild).parent = from;
}
void initialize_tree(Placement &p, std::mt19937 &random) {
    p.tree.assign(p.blocks.size(), Node{});
    int current = p.root = 0;
    vector<int> ids(p.blocks.size());
    iota(ids.begin(), ids.end(), 0);
    shuffle(ids.begin() + 1, ids.end(), random);
    for (size_t i = 1; i < ids.size(); ++i) {
        p.node(current).rightChild = ids[i];
        p.node(ids[i]).parent = current;
        current = ids[i];
    }
}
bool valid_tree(const Placement &p) {
    const int n = static_cast<int>(p.blocks.size());
    if (n == 0 || p.tree.size() != p.blocks.size() || p.root < 0 || p.root >= n ||
        p.node(p.root).parent != -1)
        return false;
    vector<bool> seen(static_cast<std::size_t>(n), false);
    vector<int> pending{p.root};
    int count = 0;
    while (!pending.empty()) {
        int id = pending.back();
        pending.pop_back();
        if (seen[static_cast<std::size_t>(id)])
            return false;
        seen[static_cast<std::size_t>(id)] = true;
        ++count;
        for (int child : {p.node(id).leftChild, p.node(id).rightChild}) {
            if (child == -1)
                continue;
            if (child < 0 || child >= n || p.node(child).parent != id)
                return false;
            pending.push_back(child);
        }
    }
    return count == n;
}
} // namespace lab2

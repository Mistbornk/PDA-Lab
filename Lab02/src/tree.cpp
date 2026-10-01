#include "tree.hpp"
#include <algorithm>
#include <numeric>
using namespace std;
namespace lab2 {
void rotate_block(Placement &p, int current) {
    swap(p.blocks[current].height, p.blocks[current].width);
    p.blocks[current].rotate = (p.blocks[current].rotate) ? false : true;
}
static void swapParent(Placement &p, int node1, int node2) {
    // swap parent
    int node1Parent = p.tree[node1].parent;
    int node2Parent = p.tree[node2].parent;
    if (node1Parent != -1) {
        (p.tree[node1Parent].leftChild == node1) ? p.tree[node1Parent].leftChild = node2
                                                 : p.tree[node1Parent].rightChild = node2;
    }
    if (node2Parent != -1) {
        (p.tree[node2Parent].leftChild == node2) ? p.tree[node2Parent].leftChild = node1
                                                 : p.tree[node2Parent].rightChild = node1;
    }
    swap(p.tree[node1].parent, p.tree[node2].parent);
}
static void swapChild(Placement &p, int node1, int node2) {
    // swap children
    swap(p.tree[node1].leftChild, p.tree[node2].leftChild);
    swap(p.tree[node1].rightChild, p.tree[node2].rightChild);

    if (p.tree[node1].leftChild != -1)
        p.tree[p.tree[node1].leftChild].parent = node1;
    if (p.tree[node1].rightChild != -1)
        p.tree[p.tree[node1].rightChild].parent = node1;

    if (p.tree[node2].leftChild != -1)
        p.tree[p.tree[node2].leftChild].parent = node2;
    if (p.tree[node2].rightChild != -1)
        p.tree[p.tree[node2].rightChild].parent = node2;
}
void swap_nodes(Placement &p, int node1, int node2) {
    // swap parent
    swapParent(p, node1, node2);

    // swap child
    swapChild(p, node1, node2);

    // relationship of node1 and node2 are parent and child
    if (p.tree[node1].parent == node1)
        p.tree[node1].parent = node2;
    else if (p.tree[node1].leftChild == node1)
        p.tree[node1].leftChild = node2;
    else if (p.tree[node1].rightChild == node1)
        p.tree[node1].rightChild = node2;

    if (p.tree[node2].parent == node2)
        p.tree[node2].parent = node1;
    else if (p.tree[node2].leftChild == node2)
        p.tree[node2].leftChild = node1;
    else if (p.tree[node2].rightChild == node2)
        p.tree[node2].rightChild = node1;

    // change root
    if (p.root == node1)
        p.root = node2;
    else if (p.root == node2)
        p.root = node1;
}
void move_node(Placement &p, int from, int to, int (*random)()) {
    // delete the node
    if (p.tree[from].leftChild == -1 && p.tree[from].rightChild == -1) {
        // if no child then directly remove
        int fromParent = p.tree[from].parent;
        if (fromParent != -1) {
            (p.tree[fromParent].leftChild == from) ? p.tree[fromParent].leftChild = -1
                                                   : p.tree[fromParent].rightChild = -1;
        }
    } else if (p.tree[from].leftChild != -1 && p.tree[from].rightChild != -1) {
        // if has two child
        while (true) {
            bool swapLeft = false;
            if (p.tree[from].leftChild != -1 && p.tree[from].rightChild != -1)
                swapLeft = (random() % 2 == 0);
            else if (p.tree[from].leftChild != -1)
                swapLeft = true;

            if (swapLeft) {
                swap_nodes(p, from, p.tree[from].leftChild);
            } else {
                swap_nodes(p, from, p.tree[from].rightChild);
            }
            if (p.tree[from].leftChild == -1 && p.tree[from].rightChild == -1)
                break;
        }
        int fromParent = p.tree[from].parent;
        if (fromParent != -1) {
            (p.tree[fromParent].leftChild == from) ? p.tree[fromParent].leftChild = -1
                                                   : p.tree[fromParent].rightChild = -1;
        }
    } else {
        // if only one child
        int fromParent = p.tree[from].parent;
        int fromChild =
            (p.tree[from].leftChild != -1) ? p.tree[from].leftChild : p.tree[from].rightChild;

        p.tree[fromChild].parent = fromParent;
        if (fromParent != -1) {
            (p.tree[fromParent].leftChild == from) ? p.tree[fromParent].leftChild = fromChild
                                                   : p.tree[fromParent].rightChild = fromChild;
        }
        p.tree[fromChild].parent = fromParent;
        if (p.root == from)
            p.root = fromChild;
    }

    // insert the node
    int op = random() % 2;
    int toChild = (op == 0) ? p.tree[to].leftChild : p.tree[to].rightChild;
    switch (op) {
    case 0: {
        p.tree[to].leftChild = from;
        break;
    }
    case 1: {
        p.tree[to].rightChild = from;
        break;
    }
    }
    op = random() % 2;
    switch (op) {
    case 0: {
        p.tree[from].leftChild = toChild;
        p.tree[from].rightChild = -1;
        break;
    }
    case 1: {
        p.tree[from].rightChild = toChild;
        p.tree[from].leftChild = -1;
        break;
    }
    }
    p.tree[from].parent = to;
    if (toChild != -1)
        p.tree[toChild].parent = from;
}
void initialize_tree(Placement &p, std::mt19937 &random) {
    p.tree.assign(p.blocks.size(), Node{});
    int current = p.root = 0;
    vector<int> ids(p.blocks.size());
    iota(ids.begin(), ids.end(), 0);
    shuffle(ids.begin() + 1, ids.end(), random);
    for (size_t i = 1; i < ids.size(); ++i) {
        p.tree[current].rightChild = ids[i];
        p.tree[ids[i]].parent = current;
        current = ids[i];
    }
}
bool valid_tree(const Placement &p) {
    const int n = static_cast<int>(p.blocks.size());
    if (n == 0 || p.tree.size() != p.blocks.size() || p.root < 0 || p.root >= n ||
        p.tree[p.root].parent != -1)
        return false;
    vector<bool> seen(n, false);
    vector<int> pending{p.root};
    int count = 0;
    while (!pending.empty()) {
        int id = pending.back();
        pending.pop_back();
        if (seen[id])
            return false;
        seen[id] = true;
        ++count;
        for (int child : {p.tree[id].leftChild, p.tree[id].rightChild}) {
            if (child == -1)
                continue;
            if (child < 0 || child >= n || p.tree[child].parent != id)
                return false;
            pending.push_back(child);
        }
    }
    return count == n;
}
} // namespace lab2

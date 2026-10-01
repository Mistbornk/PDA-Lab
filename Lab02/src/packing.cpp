#include "packing.hpp"
#include <algorithm>
#include <limits>
using namespace std;
namespace lab2 {
namespace {
int skyline_height(PackingWorkspace &w, int left, int right, int height) {
    auto &skyline = w.skyline;
    const auto less_x = [](const PackingWorkspace::Segment &s, int x) { return s.x < x; };
    auto first = std::lower_bound(skyline.begin(), skyline.end(), left, less_x);
    auto last = std::lower_bound(first, skyline.end(), right, less_x);
    const auto covering = first != skyline.end() && first->x == left ? first : std::prev(first);
    int y = covering->height;
    for (auto it = covering; it != last; ++it)
        y = std::max(y, it->height);
    const int end_height =
        last != skyline.end() && last->x == right ? last->height : std::prev(last)->height;
    const bool has_end = last != skyline.end() && last->x == right;
    auto position = skyline.erase(first, last);
    if (has_end)
        skyline.insert(position, {left, y + height});
    else
        skyline.insert(position, {{left, y + height}, {right, end_height}});
    return y;
}
int dense_height(PackingWorkspace &w, int left, int right, int height) {
    const auto end = static_cast<std::size_t>(right);
    if (end > w.dense.size())
        w.dense.resize(end, 0);
    int y = 0;
    for (auto x = static_cast<std::size_t>(left); x < end; ++x)
        y = std::max(y, w.dense[x]);
    for (auto x = static_cast<std::size_t>(left); x < end; ++x)
        w.dense[x] = y + height;
    return y;
}
} // namespace
void pack(Placement &p, int initial_contour_width, PackingWorkspace &w) {
    // Geometry uses half-open intervals. Iterative preorder also handles deep trees
    // without consuming one call-stack frame per macro.
    w.pending.clear();
    w.pending.reserve(p.blocks.size());
    w.pending.push_back(p.root);
    if (w.mode == PackingMode::Dense) {
        w.dense.assign(static_cast<std::size_t>(initial_contour_width), 0);
    } else {
        w.skyline.clear();
        w.skyline.reserve(2 * p.blocks.size() + 1);
        w.skyline.push_back({0, 0});
    }
    while (!w.pending.empty()) {
        const auto id = static_cast<std::size_t>(w.pending.back());
        w.pending.pop_back();
        auto &block = p.blocks[id];
        const auto &node = p.tree[id];
        block.x = 0;
        if (node.parent != -1) {
            const auto parent = static_cast<std::size_t>(node.parent);
            block.x =
                p.blocks[parent].x +
                (p.tree[parent].leftChild == static_cast<int>(id) ? p.blocks[parent].width : 0);
        }
        block.y = w.mode == PackingMode::Dense
                      ? dense_height(w, block.x, block.x + block.width, block.height)
                      : skyline_height(w, block.x, block.x + block.width, block.height);
        if (node.rightChild != -1)
            w.pending.push_back(node.rightChild);
        if (node.leftChild != -1)
            w.pending.push_back(node.leftChild);
    }
}
void pack(Placement &p, int initial_contour_width) {
    PackingWorkspace workspace;
    pack(p, initial_contour_width, workspace);
}
long long hpwl(const Problem &problem, const vector<Block> &blocks, bool integer_pins) {
    const int numBlocks = static_cast<int>(blocks.size());

    long long total = 0;
    for (std::size_t n = 0; n < problem.nets.size(); ++n) {
        int xmin = std::numeric_limits<int>::max(), ymin = xmin;
        int xmax = std::numeric_limits<int>::lowest(), ymax = xmax;
        const auto visit = [&](int id) {
            const int x = id >= numBlocks ? problem.terminals[id - numBlocks].x
                                          : blocks[id].x + blocks[id].width / 2;
            const int y = id >= numBlocks ? problem.terminals[id - numBlocks].y
                                          : blocks[id].y + blocks[id].height / 2;
            xmin = min(xmin, x);
            xmax = max(xmax, x);
            ymin = min(ymin, y);
            ymax = max(ymax, y);
        };
        if (integer_pins) {
            for (int id : problem.net_ids[n])
                visit(id);
        } else {
            for (const auto &name : problem.nets[n])
                visit(problem.pin_ids.at(name));
        }
        total += static_cast<long long>(xmax) - xmin + static_cast<long long>(ymax) - ymin;
    }
    return total;
}
Cost evaluate(const Problem &problem, Placement &placement, double alpha, bool outline_penalty,
              bool integer_pins, PackingWorkspace *workspace) {
    if (workspace)
        pack(placement, max(problem.outline_width, problem.outline_height), *workspace);
    else
        pack(placement, max(problem.outline_width, problem.outline_height));
    Cost c;
    for (const auto &b : placement.blocks) {
        c.width = max(c.width, b.x + b.width);
        c.height = max(c.height, b.y + b.height);
    }
    c.area = static_cast<long long>(c.width) * c.height;
    c.wirelength = hpwl(problem, placement.blocks, integer_pins);
    c.cost =
        outline_penalty
            ? max(0, c.width - problem.outline_width) + max(0, c.height - problem.outline_height)
            : static_cast<long long>(alpha * static_cast<double>(c.area) +
                                     (1 - alpha) * static_cast<double>(c.wirelength));
    return c;
}
} // namespace lab2

#include "packing.hpp"
#include <algorithm>
#include <limits>
using namespace std;
namespace lab2 {
namespace {
void updateContour(Placement &p, int current, vector<int> &contour, bool isLeft) {
    if (current < 0)
        return;

    int parent = p.tree[current].parent;

    // left or right child of parent
    p.blocks[current].x = p.blocks[parent].x + (isLeft ? p.blocks[parent].width : 0);

    int x_start = p.blocks[current].x;
    int x_end = x_start + p.blocks[current].width;
    int y_max = 0;
    int contour_size = contour.size();
    if (x_end > contour_size)
        contour.insert(contour.end(), x_end - contour.size(), 0);

    for (int i = x_start; i < x_end; i++) {
        if (contour[i] > y_max)
            y_max = contour[i];
    }

    p.blocks[current].y = y_max;

    y_max += p.blocks[current].height;
    for (int i = x_start; i < x_end; i++)
        contour[i] = y_max;

    // Recursively update the left and right children of the current block
    if (p.tree[current].leftChild != -1)
        updateContour(p, p.tree[current].leftChild, contour, true);
    if (p.tree[current].rightChild != -1)
        updateContour(p, p.tree[current].rightChild, contour, false);
}
} // namespace
void pack(Placement &p, int initial_contour_width) {

    vector<int> contour(max(initial_contour_width, p.blocks[p.root].width), 0);
    p.blocks[p.root].x = 0;
    p.blocks[p.root].y = 0;

    for (int i = 0; i < p.blocks[p.root].width; i++)
        contour[i] = p.blocks[p.root].height;

    if (p.tree[p.root].leftChild != -1)
        updateContour(p, p.tree[p.root].leftChild, contour, true);
    if (p.tree[p.root].rightChild != -1)
        updateContour(p, p.tree[p.root].rightChild, contour, false);
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
              bool integer_pins) {
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

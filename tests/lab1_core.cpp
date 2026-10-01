#include "Block.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
bool adjacent(const Block &a, const Block &b) {
    return ((a.x + a.width == b.x || b.x + b.width == a.x) &&
            std::max(a.y, b.y) < std::min(a.y + a.height, b.y + b.height)) ||
           ((a.y + a.height == b.y || b.y + b.height == a.y) &&
            std::max(a.x, b.x) < std::min(a.x + a.width, b.x + b.width));
}
void verify(TileList &tiles) {
    for (const auto &owner : tiles) {
        const Block &a = *owner;
        std::array<Block *, 4> expected{};
        std::set<Block *> neighbors;
        for (const auto &other : tiles) {
            Block *b = other.get();
            if (b->y == a.y + a.height && b->x < a.x + a.width && a.x + a.width <= b->x + b->width)
                expected[0] = b;
            if (b->x == a.x + a.width && b->y < a.y + a.height &&
                a.y + a.height <= b->y + b->height)
                expected[1] = b;
            if (b->y + b->height == a.y && b->x <= a.x && a.x < b->x + b->width)
                expected[2] = b;
            if (b->x + b->width == a.x && b->y <= a.y && a.y < b->y + b->height)
                expected[3] = b;
            if (adjacent(a, *b))
                neighbors.insert(b);
        }
        check(expected == std::array<Block *, 4>{a.rt, a.tr, a.lb, a.bl},
              "Stitch not equal to geometric oracle");
        const auto found = Neighbor_Finding(owner.get());
        check(found.size() == neighbors.size() &&
                  std::set<Block *>(found.begin(), found.end()) == neighbors,
              "Neighbor enumeration differs from geometry");
    }
    for (int y = 0; y < 12; ++y)
        for (int x = 0; x < 16; ++x) {
            auto *found = Point_Finding(tiles.front().get(), x, y);
            check(found && found->x <= x && x < found->x + found->width && found->y <= y &&
                      y < found->y + found->height,
                  "Point query outside returned tile");
        }
}
int main() {
    try {
        for (bool indexed : {false, true})
            for (bool spatial : {false, true})
                for (unsigned seed = 0; seed < 100; ++seed) {
                    std::mt19937 random(seed);
                    bool occupied[12][16]{};
                    TileList tiles(indexed, spatial);
                    tiles.push_back(std::make_unique<Block>(0, 0, 0, 16, 12, false));
                    for (int id = 1; id <= 100; ++id) {
                        int x = static_cast<int>(random() % 16),
                            y = static_cast<int>(random() % 12),
                            w = 1 + static_cast<int>(random() %
                                                     static_cast<unsigned>(std::min(5, 16 - x))),
                            h = 1 + static_cast<int>(random() %
                                                     static_cast<unsigned>(std::min(5, 12 - y)));
                        bool overlap = false;
                        for (int yy = y; yy < y + h; ++yy)
                            for (int xx = x; xx < x + w; ++xx)
                                overlap |= occupied[yy][xx];
                        check(tiles.overlaps_solid(x, y, w, h) == overlap,
                              "Overlap query differs from raster");
                        if (overlap)
                            continue;
                        Block_Creating(id, x, y, w, h, tiles);
                        for (int yy = y; yy < y + h; ++yy)
                            for (int xx = x; xx < x + w; ++xx)
                                occupied[yy][xx] = true;
                        verify(tiles);
                    }
                }
        std::cout << "100 seeds in all four stitch/geometry modes: all live stitches, adjacency, "
                     "and raster "
                     "points passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

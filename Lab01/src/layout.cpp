#include "layout.hpp"
#include "Block.h"
#include "pda/io.hpp"
#include <algorithm>
namespace lab1 {
Result solve(const Input &input, bool indexed, bool spatial) {
    TileList tiles(indexed, spatial);
    tiles.push_back(std::make_unique<Block>(0, 0, 0, input.width, input.height, false));
    Result result;
    for (const auto &c : input.commands) {
        if (c.id == 0) {
            const auto *block = Point_Finding(tiles.front().get(), c.x, c.y);
            pda::require(block != nullptr, "Broken corner stitch during point query");
            result.points.emplace_back(block->x, block->y);
        } else {
            Block_Creating(c.id, c.x, c.y, c.width, c.height, tiles);
        }
    }
    std::vector<Block *> solids;
    for (const auto &tile : tiles)
        if (tile->index > 0)
            solids.push_back(tile.get());
    std::sort(solids.begin(), solids.end(),
              [](const Block *a, const Block *b) { return a->index < b->index; });
    result.tiles = tiles.size();
    for (auto *block : solids) {
        int solid = 0, space = 0;
        for (const auto *neighbor : Neighbor_Finding(block)) {
            if (neighbor->isSolid)
                ++solid;
            else
                ++space;
        }
        result.neighbors.push_back({block->index, solid, space});
    }
    result.stitch_queries = tiles.stitch_queries;
    result.candidate_visits = tiles.candidate_visits;
    result.geometry_queries = tiles.geometry_queries;
    result.geometry_candidates = tiles.geometry_candidates;
    return result;
}
} // namespace lab1

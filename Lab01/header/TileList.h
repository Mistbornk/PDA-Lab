#pragma once
#include "Tile.h"
#include <array>
#include <cstdint>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>

// Tile geometry is immutable while indexed. The list owns tiles; edge buckets
// and positions contain non-owning references and are updated on every mutation.
class TileList {
    using Owners = std::list<std::unique_ptr<Block>>;
    using Bucket = std::map<std::uint64_t, Block *>;
    using Edges = std::unordered_map<int, Bucket>;
    struct Position {
        Owners::iterator iterator;
        std::uint64_t order;
    };
    Owners owners;
    std::unordered_map<Block *, Position> positions;
    // bottom, left, top, right coordinate -> tiles in original insertion order.
    std::array<Edges, 4> edges;
    std::uint64_t next_order = 0;
    bool indexed;
    struct SpatialIndex;
    std::unique_ptr<SpatialIndex> spatial;
    static std::array<int, 4> coordinates(const Block &b) {
        return {b.y, b.x, b.y + b.height, b.x + b.width};
    }

  public:
    std::uint64_t stitch_queries = 0, candidate_visits = 0;
    std::uint64_t geometry_queries = 0, geometry_candidates = 0;
    explicit TileList(bool use_index = true, bool use_spatial = true);
    ~TileList();
    bool overlaps_solid(int x, int y, int width, int height);
    Block *find_space_edge(int x, int width, int edge_y);
    auto begin() { return owners.begin(); }
    auto end() { return owners.end(); }
    auto begin() const { return owners.begin(); }
    auto end() const { return owners.end(); }
    const auto &front() const { return owners.front(); }
    auto size() const { return owners.size(); }
    bool uses_index() const { return indexed; }
    void push_back(std::unique_ptr<Block> owner);
    std::unique_ptr<Block> extract(Block *tile);
    void repair(Block *target) {
        ++stitch_queries;
        const std::array<int, 4> keys{target->y + target->height, target->x + target->width,
                                      target->y, target->x};
        for (std::size_t edge = 0; edge < edges.size(); ++edge) {
            auto found = edges[edge].find(keys[edge]);
            if (found == edges[edge].end())
                continue;
            // The old full scan selects the LAST match. Transient overlapping
            // split/merge tiles make insertion order significant during repair.
            for (auto it = found->second.rbegin(); it != found->second.rend(); ++it) {
                ++candidate_visits;
                Block *b = it->second;
                if (edge == 0 && b->x < target->x + target->width &&
                    target->x + target->width <= b->x + b->width) {
                    target->rt = b;
                    break;
                }
                if (edge == 1 && b->y < target->y + target->height &&
                    target->y + target->height <= b->y + b->height) {
                    target->tr = b;
                    break;
                }
                if (edge == 2 && b->x <= target->x && target->x < b->x + b->width) {
                    target->lb = b;
                    break;
                }
                if (edge == 3 && b->y <= target->y && target->y < b->y + b->height) {
                    target->bl = b;
                    break;
                }
            }
        }
    }
};

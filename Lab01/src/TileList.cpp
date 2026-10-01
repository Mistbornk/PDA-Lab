#include "TileList.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <limits>
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
struct TileList::SpatialIndex {
    // All int coordinates are represented exactly by double. Geometry predicates
    // below still use integer half-open bounds; the R-tree only selects candidates.
    using Point = bg::model::point<double, 2, bg::cs::cartesian>;
    using Box = bg::model::box<Point>;
    using Value = std::pair<Box, Block *>;
    bgi::rtree<Value, bgi::quadratic<16>> tiles;
    static Box box(const Block &b) {
        return {{static_cast<double>(b.x), static_cast<double>(b.y)},
                {static_cast<double>(b.x + b.width), static_cast<double>(b.y + b.height)}};
    }
};
TileList::TileList(bool use_index, bool use_spatial)
    : indexed(use_index), spatial(use_spatial ? std::make_unique<SpatialIndex>() : nullptr) {}
TileList::~TileList() = default;
void TileList::push_back(std::unique_ptr<Block> owner) {
    Block *tile = owner.get();
    if (spatial)
        spatial->tiles.insert({SpatialIndex::box(*tile), tile});
    const auto order = next_order++;
    owners.push_back(std::move(owner));
    positions.emplace(tile, Position{std::prev(owners.end()), order});
    if (indexed) {
        auto keys = coordinates(*tile);
        for (std::size_t i = 0; i < edges.size(); ++i)
            edges[i][keys[i]].emplace(order, tile);
    }
}
std::unique_ptr<Block> TileList::extract(Block *tile) {
    auto found = positions.find(tile);
    if (found == positions.end())
        return {};
    auto position = found->second;
    if (spatial)
        spatial->tiles.remove({SpatialIndex::box(*tile), tile});
    if (indexed) {
        auto keys = coordinates(*tile);
        for (std::size_t i = 0; i < edges.size(); ++i) {
            auto bucket = edges[i].find(keys[i]);
            bucket->second.erase(position.order);
            if (bucket->second.empty())
                edges[i].erase(bucket);
        }
    }
    auto owner = std::move(*position.iterator);
    owners.erase(position.iterator);
    positions.erase(found);
    return owner;
}

bool TileList::overlaps_solid(int x, int y, int width, int height) {
    ++geometry_queries;
    const auto overlaps = [&](const Block *b) {
        ++geometry_candidates;
        return b->isSolid && x < b->x + b->width && b->x < x + width && y < b->y + b->height &&
               b->y < y + height;
    };
    if (spatial) {
        const SpatialIndex::Box box{
            {static_cast<double>(x), static_cast<double>(y)},
            {static_cast<double>(x + width), static_cast<double>(y + height)}};
        for (auto it = spatial->tiles.qbegin(bgi::intersects(box)); it != spatial->tiles.qend();
             ++it)
            if (overlaps(it->second))
                return true;
    } else {
        for (const auto &b : owners)
            if (overlaps(b.get()))
                return true;
    }
    return false;
}
Block *TileList::find_space_edge(int x, int width, int edge_y) {
    ++geometry_queries;
    const auto contains = [&](const Block *b) {
        ++geometry_candidates;
        return !b->isSolid && b->y < edge_y && edge_y < b->y + b->height && b->x <= x &&
               x + width <= b->x + b->width;
    };
    if (!spatial) {
        for (const auto &b : owners)
            if (contains(b.get()))
                return b.get();
        return nullptr;
    }
    Block *first = nullptr;
    auto earliest = std::numeric_limits<std::uint64_t>::max();
    const SpatialIndex::Point point{static_cast<double>(x), static_cast<double>(edge_y)};
    for (auto it = spatial->tiles.qbegin(bgi::intersects(point)); it != spatial->tiles.qend();
         ++it) {
        Block *candidate = it->second;
        if (contains(candidate) && positions.at(candidate).order < earliest) {
            first = candidate;
            earliest = positions.at(candidate).order;
        }
    }
    return first;
}

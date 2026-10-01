#include "legalizer.hpp"
#include "pda/io.hpp"
#include <algorithm>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <unordered_map>

namespace lab3 {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using Point = bg::model::point<double, 2, bg::cs::cartesian>;
using Box = bg::model::box<Point>;
using Value = std::pair<Box, std::size_t>;
namespace {
Box box(const Cell &cell) {
    return Box(Point(cell.x, cell.y), Point(cell.x + cell.width, cell.y + cell.height));
}
bool overlaps(const Box &a, const Box &b) {
    return a.min_corner().get<0>() < b.max_corner().get<0>() &&
           b.min_corner().get<0>() < a.max_corner().get<0>() &&
           a.min_corner().get<1>() < b.max_corner().get<1>() &&
           b.min_corner().get<1>() < a.max_corner().get<1>();
}
double align_up(double x, const PlacementRow &row) {
    return row.startX + std::ceil((x - row.startX) / row.siteWidth) * row.siteWidth;
}
class Solver {
    Input data;
    Options options;
    bgi::rtree<Value, bgi::quadratic<16>> tree;
    std::unordered_map<std::string, std::size_t> ids;
    std::size_t queries = 0, rows_tested = 0;
    struct Collision {
        bool hit = false;
        double left = std::numeric_limits<double>::max(),
               right = std::numeric_limits<double>::lowest();
    };
    Collision collision(const Cell &c) {
        ++queries;
        const Box query = box(c);
        Collision result;
        for (auto it = tree.qbegin(bgi::intersects(query)); it != tree.qend(); ++it) {
            if (!overlaps(query, it->first))
                continue;
            result.hit = true;
            result.left = std::min(result.left, it->first.min_corner().get<0>());
            result.right = std::max(result.right, it->first.max_corner().get<0>());
        }
        return result;
    }
    double first_fit(Cell c, const PlacementRow &row, double right) {
        c.x = row.startX;
        c.y = row.startY;
        if (!options.intervals) {
            while (c.x + c.width <= right) {
                const auto hits = collision(c);
                if (!hits.hit)
                    return c.x;
                c.x = align_up(hits.right, row);
            }
        } else {
            // One spatial query per row instead of one per blocked candidate.
            ++queries;
            const Box strip(Point(row.startX, c.y), Point(right, c.y + c.height));
            std::vector<std::pair<double, double>> intervals;
            for (auto it = tree.qbegin(bgi::intersects(strip)); it != tree.qend(); ++it) {
                if (overlaps(strip, it->first))
                    intervals.emplace_back(it->first.min_corner().get<0>(),
                                           it->first.max_corner().get<0>());
            }
            std::sort(intervals.begin(), intervals.end());
            for (const auto &interval : intervals) {
                if (interval.second <= c.x)
                    continue;
                if (c.x + c.width <= interval.first)
                    break;
                c.x = align_up(interval.second, row);
            }
            if (c.x + c.width <= right)
                return c.x;
        }
        return std::numeric_limits<double>::infinity();
    }
    double near_fit(Cell c, const PlacementRow &row, double right) {
        double left =
            std::min(align_up(c.x, row),
                     row.startX + std::floor((right - c.width - row.startX) / row.siteWidth) *
                                      row.siteWidth);
        double next = std::max(align_up(c.x, row), row.startX);
        while (left >= row.startX || next + c.width <= right) {
            double best = std::numeric_limits<double>::infinity();
            if (left >= row.startX) {
                auto candidate = c;
                candidate.x = left;
                candidate.y = row.startY;
                const auto hits = collision(candidate);
                if (!hits.hit)
                    best = left;
                else
                    left =
                        row.startX +
                        std::floor(((hits.left < left ? hits.left : left - c.width) - row.startX) /
                                   row.siteWidth) *
                            row.siteWidth;
            }
            if (next + c.width <= right) {
                auto candidate = c;
                candidate.x = next;
                candidate.y = row.startY;
                const auto hits = collision(candidate);
                if (!hits.hit) {
                    if (std::abs(best - c.x) > std::abs(next - c.x))
                        best = next;
                } else
                    next = align_up(hits.right, row);
            }
            if (std::isfinite(best))
                return best;
        }
        return std::numeric_limits<double>::infinity();
    }
    void insert(Cell c) {
        const auto id = data.cells.size();
        pda::require(ids.emplace(c.name, id).second, "Duplicate live cell: " + c.name);
        tree.insert({box(c), id});
        data.cells.push_back(std::move(c));
    }

  public:
    Solver(Input input, Options opts) : data(std::move(input)), options(opts) {
        ids.reserve(data.cells.size());
        for (std::size_t i = 0; i < data.cells.size(); ++i) {
            ids.emplace(data.cells[i].name, i);
            tree.insert({box(data.cells[i]), i});
        }
    }
    void run(const std::vector<Step> &steps, std::ostream &output) {
        output << std::setprecision(std::numeric_limits<double>::max_digits10);
        for (const auto &step : steps) {
            for (const auto &name : step.remove) {
                const auto it = ids.find(name);
                pda::require(it != ids.end(), "Unknown banked cell: " + name);
                const auto id = it->second;
                pda::require(!data.cells[id].fixed, "Cannot bank a FIX cell: " + name);
                pda::require(tree.remove({box(data.cells[id]), id}) == 1,
                             "Missing cell in spatial index");
                ids.erase(it);
            }
            Cell cell = step.cell;
            // Preserve the original stateful tie ordering on course cases.
            std::sort(data.rows.begin(), data.rows.end(), [&](const auto &a, const auto &b) {
                return std::abs(cell.y - a.startY) < std::abs(cell.y - b.startY);
            });
            bool placed = false;
            for (const auto &row : data.rows) {
                if (row.startY + cell.height > data.die.uRY)
                    continue;
                const double right =
                    std::min(data.die.uRX, row.startX + row.siteWidth * row.NumOfSites);
                if (cell.width > right - row.startX)
                    continue;
                ++rows_tested;
                const double x =
                    options.nearest ? near_fit(cell, row, right) : first_fit(cell, row, right);
                if (!std::isfinite(x))
                    continue;
                cell.x = x;
                cell.y = row.startY;
                insert(cell);
                output << cell.x << ' ' << cell.y << "\n0\n";
                placed = true;
                break;
            }
            pda::require(placed, "Could not place cell: " + cell.name);
        }
        if (options.stats)
            std::cerr << "{\"spatial_queries\":" << queries << ",\"rows_tested\":" << rows_tested
                      << "}\n";
    }
};
} // namespace
void legalize(Input input, const std::vector<Step> &steps, std::ostream &output, Options options) {
    Solver(std::move(input), options).run(steps, output);
}
} // namespace lab3

#include "legalizer.hpp"
#include "pda/io.hpp"
#include <algorithm>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <iomanip>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace lab3 {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using Point = bg::model::point<double, 2, bg::cs::cartesian>;
using Box = bg::model::box<Point>;
using Value = std::pair<Box, CellId>;
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
} // namespace
struct Legalizer::Impl {
    Input data;
    Options options;
    bgi::rtree<Value, bgi::quadratic<16>> tree;
    std::unordered_map<std::string, CellId> ids;
    std::vector<std::size_t> row_order;
    std::unordered_set<std::size_t> excluded;
    bool poisoned = false;
    Stats counters;
    struct Collision {
        bool hit = false;
        double left = std::numeric_limits<double>::max(),
               right = std::numeric_limits<double>::lowest();
    };
    Collision collision(const Cell &c) {
        ++counters.spatial_queries;
        const Box query = box(c);
        Collision result;
        for (auto it = tree.qbegin(bgi::intersects(query)); it != tree.qend(); ++it) {
            if (excluded.count(it->second.value) || !overlaps(query, it->first))
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
            ++counters.spatial_queries;
            const Box strip(Point(row.startX, c.y), Point(right, c.y + c.height));
            std::vector<std::pair<double, double>> intervals;
            for (auto it = tree.qbegin(bgi::intersects(strip)); it != tree.qend(); ++it) {
                if (!excluded.count(it->second.value) && overlaps(strip, it->first))
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
    Impl(Input input, Options opts) : data(std::move(input)), options(opts) {
        ids.reserve(data.cells.size());
        row_order.resize(data.rows.size());
        std::iota(row_order.begin(), row_order.end(), 0);
        for (std::size_t i = 0; i < data.cells.size(); ++i) {
            pda::require(ids.emplace(data.cells[i].name, CellId{i}).second, "Duplicate cell");
            tree.insert({box(data.cells[i]), CellId{i}});
        }
    }
    StepResult apply(const Step &step) {
        pda::require(!poisoned, "Discard legalizer after an internal commit failure");
        excluded.clear();
        pda::require(!step.remove.empty(), "Empty banking list");
        for (const auto &name : step.remove) {
            const auto it = ids.find(name);
            pda::require(it != ids.end(), "Unknown banked cell: " + name);
            pda::require(!data.cells[it->second.value].fixed, "Cannot bank a FIX cell: " + name);
            pda::require(excluded.insert(it->second.value).second, "Duplicate banked cell");
        }
        const auto existing = ids.find(step.cell.name);
        pda::require(existing == ids.end() || excluded.count(existing->second.value),
                     "Duplicate live cell: " + step.cell.name);
        pda::require(!step.cell.name.empty() && std::isfinite(step.cell.x) &&
                         std::isfinite(step.cell.y) && pda::positive(step.cell.width) &&
                         pda::positive(step.cell.height) && !step.cell.fixed,
                     "Invalid merged cell");
        auto order = row_order;
        // Preserve legacy successful-step tie order, without changing it on failure.
        std::sort(order.begin(), order.end(), [&](auto a, auto b) {
            return std::abs(step.cell.y - data.rows[a].startY) <
                   std::abs(step.cell.y - data.rows[b].startY);
        });
        StepResult result;
        result.placed = step.cell;
        bool placed = false;
        for (const auto index : order) {
            const auto &row = data.rows[index];
            if (row.startY + step.cell.height > data.die.uRY)
                continue;
            const double right =
                std::min(data.die.uRX, row.startX + row.siteWidth * row.NumOfSites);
            if (step.cell.width > right - row.startX)
                continue;
            ++counters.rows_tested;
            const double x = options.nearest ? near_fit(step.cell, row, right)
                                             : first_fit(step.cell, row, right);
            if (!std::isfinite(x))
                continue;
            result.placed.x = x;
            result.placed.y = row.startY;
            placed = true;
            break;
        }
        if (!placed)
            throw NoLegalPlacement(step.cell.name);
        // No expected failure remains. Allocation/internal failures poison this
        // instance; semantic failures above have not changed cells or indexes.
        poisoned = true;
        for (const auto index : excluded) {
            const auto &cell = data.cells[index];
            pda::require(tree.remove({box(cell), CellId{index}}) == 1,
                         "Missing cell in spatial index");
            ids.erase(cell.name);
        }
        const CellId id{data.cells.size()};
        data.cells.push_back(result.placed);
        ids.emplace(result.placed.name, id);
        tree.insert({box(result.placed), id});
        row_order = std::move(order);
        excluded.clear();
        ++counters.committed_steps;
        poisoned = false;
        return result;
    }
    void validate() const {
        pda::require(!poisoned && ids.size() == tree.size(), "Invalid legalizer index size");
        for (const auto &entry : ids) {
            const auto &c = data.cells.at(entry.second.value);
            pda::require(c.name == entry.first && c.x >= data.die.lLX && c.y >= data.die.lLY &&
                             c.x + c.width <= data.die.uRX && c.y + c.height <= data.die.uRY,
                         "Invalid live cell");
            bool on_site = false;
            for (const auto &row : data.rows) {
                const double site = (c.x - row.startX) / row.siteWidth;
                if (c.y == row.startY && site >= 0 && site < row.NumOfSites &&
                    std::abs(site - std::round(site)) < 1e-7)
                    on_site = true;
            }
            pda::require(on_site, "Cell is not on a site");
            std::size_t self = 0;
            const auto bounds = box(c);
            for (auto it = tree.qbegin(bgi::intersects(bounds)); it != tree.qend(); ++it) {
                if (it->second == entry.second) {
                    ++self;
                    pda::require(bg::equals(it->first, bounds), "Stale cell geometry");
                } else
                    pda::require(!overlaps(bounds, it->first), "Overlapping live cells");
            }
            pda::require(self == 1, "Missing/duplicate cell index");
        }
    }
};
Legalizer::Legalizer(Input input, Options options)
    : impl(std::make_unique<Impl>(std::move(input), options)) {}
Legalizer::~Legalizer() = default;
Legalizer::Legalizer(Legalizer &&) noexcept = default;
Legalizer &Legalizer::operator=(Legalizer &&) noexcept = default;
StepResult Legalizer::apply(const Step &step) { return impl->apply(step); }
const Stats &Legalizer::stats() const { return impl->counters; }
std::vector<Cell> Legalizer::cells() const {
    std::vector<Cell> result;
    for (std::size_t i = 0; i < impl->data.cells.size(); ++i) {
        const auto &cell = impl->data.cells[i];
        const auto it = impl->ids.find(cell.name);
        if (it != impl->ids.end() && it->second.value == i)
            result.push_back(cell);
    }
    return result;
}
void Legalizer::validate() const { impl->validate(); }
void write_step(std::ostream &output, const StepResult &result) {
    output << std::setprecision(std::numeric_limits<double>::max_digits10) << result.placed.x << ' '
           << result.placed.y << '\n'
           << result.moved.size() << '\n';
    for (const auto &cell : result.moved)
        output << cell.name << ' ' << cell.x << ' ' << cell.y << '\n';
}
void write_stats(std::ostream &output, const Stats &stats) {
    output << "{\"spatial_queries\":" << stats.spatial_queries
           << ",\"rows_tested\":" << stats.rows_tested
           << ",\"committed_steps\":" << stats.committed_steps << "}\n";
}
void legalize(Input input, const std::vector<Step> &steps, std::ostream &output, Options options) {
    Legalizer solver(std::move(input), options);
    for (const auto &step : steps)
        write_step(output, solver.apply(step));
}
} // namespace lab3

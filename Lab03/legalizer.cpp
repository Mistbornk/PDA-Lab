#include "legalizer.hpp"
#include "pda/io.hpp"
#include <algorithm>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <numeric>
#include <optional>
#include <tuple>
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
double displacement(const Cell &cell) {
    return std::abs(cell.x - cell.opt_x) + std::abs(cell.y - cell.opt_y);
}
} // namespace
struct Legalizer::Impl {
    Input data;
    Options options;
    bgi::rtree<Value, bgi::quadratic<16>> tree;
    std::unordered_map<std::string, CellId> ids;
    std::vector<std::size_t> row_order;
    std::unordered_set<std::size_t> excluded;
    std::vector<std::uint64_t> exclusion_epoch;
    std::uint64_t epoch = 0;
    bool is_excluded(std::size_t id) const { return exclusion_epoch[id] == epoch; }
    bool poisoned = false;
    Stats counters;
    std::vector<Cell> staged;
    std::unordered_map<std::string, std::size_t> scored_ids;
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
            if (is_excluded(it->second.value) || !overlaps(query, it->first))
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
                if (!is_excluded(it->second.value) && overlaps(strip, it->first))
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
    std::optional<Cell> closest(Cell target, const std::optional<Cell> &center = std::nullopt) {
        std::vector<std::size_t> order(data.rows.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](auto a, auto b) {
            return std::make_pair(std::abs(target.y - data.rows[a].startY), data.rows[a].startY) <
                   std::make_pair(std::abs(target.y - data.rows[b].startY), data.rows[b].startY);
        });
        std::optional<Cell> best;
        double best_distance = std::numeric_limits<double>::infinity();
        for (const auto index : order) {
            const auto &row = data.rows[index];
            const double dy = std::abs(row.startY - target.y);
            if (dy > best_distance)
                break;
            if (row.startY + target.height > data.die.uRY)
                continue;
            const double radius_x = center
                                        ? options.repair_radius - std::abs(row.startY - center->y)
                                        : std::numeric_limits<double>::infinity();
            if (radius_x < 0)
                continue;
            ++counters.rows_tested;
            const double right =
                std::min(data.die.uRX, row.startX + row.siteWidth * row.NumOfSites);
            const Box strip(Point(row.startX, row.startY),
                            Point(right, row.startY + target.height));
            std::vector<std::pair<double, double>> intervals;
            ++counters.spatial_queries;
            for (auto it = tree.qbegin(bgi::intersects(strip)); it != tree.qend(); ++it)
                if (!is_excluded(it->second.value) && overlaps(strip, it->first))
                    intervals.emplace_back(it->first.min_corner().get<0>(),
                                           it->first.max_corner().get<0>());
            for (const auto &cell : staged)
                if (overlaps(strip, box(cell)))
                    intervals.emplace_back(cell.x, cell.x + cell.width);
            std::sort(intervals.begin(), intervals.end());
            const auto consider = [&](double left, double end) {
                double lower = left, upper = end - target.width;
                if (center) {
                    lower = std::max(lower, center->x - radius_x);
                    upper = std::min(upper, center->x + radius_x);
                }
                const double lo = std::ceil((lower - row.startX) / row.siteWidth);
                const double hi = std::floor((upper - row.startX) / row.siteWidth);
                if (lo > hi)
                    return;
                for (const auto site : {std::floor((target.x - row.startX) / row.siteWidth),
                                        std::ceil((target.x - row.startX) / row.siteWidth)}) {
                    Cell candidate = target;
                    candidate.x = row.startX + std::clamp(site, lo, hi) * row.siteWidth;
                    candidate.y = row.startY;
                    if (candidate.x < left || candidate.x + candidate.width > end)
                        continue;
                    const double distance = dy + std::abs(candidate.x - target.x);
                    if (!best || std::make_tuple(distance, candidate.y, candidate.x) <
                                     std::make_tuple(best_distance, best->y, best->x)) {
                        best = candidate;
                        best_distance = distance;
                    }
                }
            };
            double left = row.startX;
            for (const auto &[begin, end] : intervals) {
                if (begin > left)
                    consider(left, std::min(begin, right));
                left = std::max(left, end);
                if (left >= right)
                    break;
            }
            if (left < right)
                consider(left, right);
        }
        return best;
    }
    std::optional<StepResult> repair(const Cell &target, std::optional<StepResult> best) {
        const auto score = [&](const StepResult &trial) {
            double delta = scored_ids.count(trial.placed.name) ? 0 : displacement(trial.placed);
            for (const auto &cell : trial.moved) {
                const auto id = ids.at(cell.name).value;
                const auto scored = scored_ids.find(cell.name);
                if (scored == scored_ids.end() || scored->second == id)
                    delta += displacement(cell) - displacement(data.cells[id]);
            }
            return data.alpha * static_cast<double>(trial.moved.size()) + data.beta * delta;
        };
        double best_score = best ? score(*best) : std::numeric_limits<double>::infinity();
        std::vector<std::pair<double, double>>
            positions; // y,x; generated independently of index order
        auto order = row_order;
        std::sort(order.begin(), order.end(), [&](auto a, auto b) {
            return std::make_pair(std::abs(target.y - data.rows[a].startY), data.rows[a].startY) <
                   std::make_pair(std::abs(target.y - data.rows[b].startY), data.rows[b].startY);
        });
        std::size_t considered_rows = 0;
        for (auto index : order) {
            const auto &row = data.rows[index];
            if (std::abs(row.startY - target.y) > options.repair_radius)
                break;
            if (considered_rows++ >= options.repair_candidates)
                break;
            if (row.startY + target.height > data.die.uRY)
                continue;
            const double right =
                std::min(data.die.uRX, row.startX + row.siteWidth * row.NumOfSites);
            const double last = std::floor((right - target.width - row.startX) / row.siteWidth);
            if (last < 0)
                continue;
            const double middle =
                std::clamp(std::round((target.x - row.startX) / row.siteWidth), 0., last);
            for (std::size_t offset = 0; offset < options.repair_candidates; ++offset) {
                for (double direction : {-1., 1.}) {
                    const double site = middle + direction * static_cast<double>(offset);
                    const double x = row.startX + site * row.siteWidth;
                    if (site >= 0 && site <= last &&
                        std::abs(x - target.x) + std::abs(row.startY - target.y) <=
                            options.repair_radius)
                        positions.emplace_back(row.startY, x);
                }
            }
        }
        std::sort(positions.begin(), positions.end(), [&](auto a, auto b) {
            return std::make_tuple(std::abs(a.first - target.y) + std::abs(a.second - target.x),
                                   a) <
                   std::make_tuple(std::abs(b.first - target.y) + std::abs(b.second - target.x), b);
        });
        positions.erase(std::unique(positions.begin(), positions.end()), positions.end());
        if (positions.size() > options.repair_candidates)
            positions.resize(options.repair_candidates);
        for (const auto &[y, x] : positions) {
            ++counters.repair_attempts;
            Cell inserted = target;
            inserted.x = x;
            inserted.y = y;
            const auto bounds = box(inserted);
            std::vector<std::size_t> blockers;
            bool fixed = false;
            ++counters.spatial_queries;
            for (auto it = tree.qbegin(bgi::intersects(bounds)); it != tree.qend(); ++it) {
                if (is_excluded(it->second.value) || !overlaps(bounds, it->first))
                    continue;
                blockers.push_back(it->second.value);
                fixed = fixed || data.cells[it->second.value].fixed;
                if (fixed || blockers.size() > options.repair_cells)
                    break;
            }
            if (fixed || blockers.size() > options.repair_cells || blockers.empty())
                continue;
            // Every direct blocker must move. Even returning each blocker to its
            // original point cannot beat this optimistic immediate score bound.
            double lower_bound =
                data.alpha * static_cast<double>(blockers.size()) +
                data.beta * (scored_ids.count(inserted.name) ? 0 : displacement(inserted));
            for (auto id : blockers) {
                const auto scored = scored_ids.find(data.cells[id].name);
                if (scored == scored_ids.end() || scored->second == id)
                    lower_bound -= data.beta * displacement(data.cells[id]);
            }
            if (best && lower_bound >= best_score)
                continue;
            std::sort(blockers.begin(), blockers.end());
            for (auto id : blockers)
                exclusion_epoch[id] = epoch;
            for (unsigned permutation = 0; permutation < (blockers.size() > 1 ? 2u : 1u);
                 ++permutation) {
                if (permutation)
                    std::reverse(blockers.begin(), blockers.end());
                staged = {inserted};
                StepResult trial{inserted, {}};
                bool success = true;
                for (auto id : blockers) {
                    const auto &old = data.cells[id];
                    Cell wanted = old;
                    wanted.x = old.opt_x;
                    wanted.y = old.opt_y;
                    auto placed = closest(wanted, old);
                    if (!placed) {
                        success = false;
                        break;
                    }
                    staged.push_back(*placed);
                    if (placed->x != old.x || placed->y != old.y)
                        trial.moved.push_back(*placed);
                }
                if (success && score(trial) < best_score) {
                    best_score = score(trial);
                    best = std::move(trial);
                }
            }
            for (auto id : blockers)
                exclusion_epoch[id] = 0;
            staged.clear();
        }
        return best;
    }
    void account(const Cell &cell, std::size_t id, bool moved) {
        counters.maximum_displacement = std::max(counters.maximum_displacement, displacement(cell));
        counters.moved_cells += moved;
        const auto found = scored_ids.find(cell.name);
        // The pinned evaluator retains the first recorded object for each name,
        // even after banking. Reusing a name must not replace that score entry.
        if (found != scored_ids.end() && found->second != id)
            return;
        if (found != scored_ids.end())
            counters.total_distance -= displacement(data.cells[found->second]);
        counters.total_distance += displacement(cell);
        scored_ids[cell.name] = id;
    }
    Impl(Input input, Options opts) : data(std::move(input)), options(opts) {
        pda::input_require(options.repair_cells <= 8 && options.repair_candidates > 0 &&
                               options.repair_candidates <= 256 &&
                               pda::nonnegative(options.repair_radius),
                           "Invalid local repair limits");
        ids.reserve(data.cells.size());
        exclusion_epoch.resize(data.cells.size());
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
        staged.clear();
        if (++epoch == 0) {
            std::fill(exclusion_epoch.begin(), exclusion_epoch.end(), 0);
            epoch = 1;
        }
        pda::require(!step.remove.empty(), "Empty banking list");
        for (const auto &name : step.remove) {
            const auto it = ids.find(name);
            pda::require(it != ids.end(), "Unknown banked cell: " + name);
            pda::require(!data.cells[it->second.value].fixed, "Cannot bank a FIX cell: " + name);
            pda::require(excluded.insert(it->second.value).second, "Duplicate banked cell");
            exclusion_epoch[it->second.value] = epoch;
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
        result.placed.opt_x = step.cell.x;
        result.placed.opt_y = step.cell.y;
        bool placed = false;
        if (options.strategy == Strategy::Legacy) {
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
        } else {
            std::optional<StepResult> selected;
            if (auto cell = closest(result.placed))
                selected = StepResult{*cell, {}};
            if (options.strategy == Strategy::Repair)
                selected = repair(result.placed, std::move(selected));
            if (selected) {
                result = std::move(*selected);
                placed = true;
            }
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
        for (const auto &moved : result.moved) {
            const auto id = ids.at(moved.name);
            pda::require(!data.cells[id.value].fixed &&
                             tree.remove({box(data.cells[id.value]), id}) == 1,
                         "Invalid repair commit");
            account(moved, id.value, true);
            data.cells[id.value] = moved;
            tree.insert({box(moved), id});
        }
        const CellId id{data.cells.size()};
        account(result.placed, id.value, false);
        data.cells.push_back(result.placed);
        exclusion_epoch.push_back(0);
        ids.emplace(result.placed.name, id);
        tree.insert({box(result.placed), id});
        row_order = std::move(order);
        excluded.clear();
        ++counters.committed_steps;
        counters.objective = data.alpha * static_cast<double>(counters.moved_cells) +
                             data.beta * counters.total_distance;
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
           << ",\"committed_steps\":" << stats.committed_steps
           << ",\"repair_attempts\":" << stats.repair_attempts
           << ",\"move_times\":" << stats.moved_cells
           << ",\"total_distance\":" << std::setprecision(17) << stats.total_distance
           << ",\"maximum_displacement_seen\":" << stats.maximum_displacement
           << ",\"objective\":" << stats.objective << "}\n";
}
void legalize(Input input, const std::vector<Step> &steps, std::ostream &output, Options options) {
    Legalizer solver(std::move(input), options);
    for (const auto &step : steps)
        write_step(output, solver.apply(step));
}
} // namespace lab3

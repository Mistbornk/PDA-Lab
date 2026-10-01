#include "input.hpp"
#include "pda/io.hpp"
#include <algorithm>
#include <limits>
#include <sstream>
namespace lab4 {
Input parse(const std::string &gmp, const std::string &gcl, const std::string &cst) {
    Input result;
    auto map = pda::input_file(gmp);
    auto &a = result.area;
    auto &g = result.grid;
    pda::expect(map, ".ra");
    pda::read(map, a.Routing_Area_X, a.Routing_Area_Y, a.Routing_Area_Width, a.Routing_Area_Height);
    pda::expect(map, ".g");
    pda::read(map, g.GridWidth, g.GridHeight);
    pda::require(a.Routing_Area_Width > 0 && a.Routing_Area_Height > 0 && g.GridWidth > 0 &&
                     g.GridHeight > 0 && a.Routing_Area_Width % g.GridWidth == 0 &&
                     a.Routing_Area_Height % g.GridHeight == 0,
                 "Invalid routing area or grid");
    pda::require(static_cast<long long>(a.Routing_Area_X) + a.Routing_Area_Width <=
                         std::numeric_limits<int>::max() &&
                     static_cast<long long>(a.Routing_Area_Y) + a.Routing_Area_Height <=
                         std::numeric_limits<int>::max(),
                 "Routing coordinates overflow");
    a.num_rows = a.Routing_Area_Height / g.GridHeight;
    a.num_cols = a.Routing_Area_Width / g.GridWidth;
    // Read two chip sections; preserve sequential indices required by the course.
    pda::expect(map, ".c");
    for (int side = 0; side < 2; ++side) {
        Chip chip{};
        pda::read(map, chip.x, chip.y, chip.width, chip.height);
        pda::require(chip.width > 0 && chip.height > 0 && chip.x >= 0 && chip.y >= 0 &&
                         static_cast<long long>(chip.x) + chip.width <= a.Routing_Area_Width &&
                         static_cast<long long>(chip.y) + chip.height <= a.Routing_Area_Height,
                     "Invalid chip geometry");
        pda::expect(map, ".b");
        std::string token;
        std::size_t count = 0;
        bool separator = false;
        while (map >> token) {
            if (token == ".c") {
                separator = true;
                break;
            }
            std::size_t used = 0;
            const int index = std::stoi(token, &used);
            pda::require(used == token.size() && index > 0 &&
                             static_cast<std::size_t>(index) == count + 1,
                         "Nonsequential bump index");
            int x, y;
            pda::read(map, x, y);
            pda::require(x >= 0 && y >= 0 && x <= chip.width && y <= chip.height,
                         "Bump outside chip");
            const long long realx = static_cast<long long>(a.Routing_Area_X) + chip.x + x;
            const long long realy = static_cast<long long>(a.Routing_Area_Y) + chip.y + y;
            pda::require(
                realx >= a.Routing_Area_X && realy >= a.Routing_Area_Y &&
                    realx < static_cast<long long>(a.Routing_Area_X) + a.Routing_Area_Width &&
                    realy < static_cast<long long>(a.Routing_Area_Y) + a.Routing_Area_Height,
                "Bump outside routing grid");
            if (side == 0)
                result.nets.push_back(
                    Net{index, static_cast<int>(realx), static_cast<int>(realy), 0, 0});
            else {
                pda::require(count < result.nets.size(), "Mismatched bump count");
                result.nets[count].bump2_x = static_cast<int>(realx);
                result.nets[count].bump2_y = static_cast<int>(realy);
            }
            ++count;
        }
        pda::require(side == 0 ? separator : !separator && count == result.nets.size(),
                     "Missing chip or mismatched bump count");
    }
    auto capacity = pda::input_file(gcl);
    pda::expect(capacity, ".ec");
    result.cells.resize(static_cast<std::size_t>(a.num_rows));
    for (int y = 0; y < a.num_rows; ++y) {
        auto &row = result.cells[static_cast<std::size_t>(y)];
        row.resize(static_cast<std::size_t>(a.num_cols));
        for (int x = 0; x < a.num_cols; ++x) {
            auto &cell = row[static_cast<std::size_t>(x)];
            pda::read(capacity, cell.left_capacity, cell.bottom_capacity);
            pda::require(cell.left_capacity >= 0 && cell.bottom_capacity >= 0,
                         "Negative edge capacity");
            cell.left_usage = cell.bottom_usage = 0;
            cell.x = a.Routing_Area_X + x * g.GridWidth;
            cell.y = a.Routing_Area_Y + y * g.GridHeight;
        }
    }
    pda::end(capacity);
    auto costs = pda::input_file(cst);
    auto &c = result.costs;
    pda::expect(costs, ".alpha");
    pda::read(costs, c.alpha);
    pda::expect(costs, ".beta");
    pda::read(costs, c.beta);
    pda::expect(costs, ".gamma");
    pda::read(costs, c.gamma);
    pda::expect(costs, ".delta");
    pda::read(costs, c.delta);
    pda::expect(costs, ".v");
    pda::read(costs, c.via_cost);
    pda::require(pda::nonnegative(c.alpha) && pda::nonnegative(c.beta) &&
                     pda::nonnegative(c.gamma) && pda::nonnegative(c.delta) &&
                     pda::nonnegative(c.via_cost),
                 "Invalid cost weights");
    for (auto *layer : {&c.layer1_cost, &c.layer2_cost}) {
        pda::expect(costs, ".l");
        layer->resize(static_cast<std::size_t>(a.num_rows));
        for (auto &row : *layer) {
            row.resize(static_cast<std::size_t>(a.num_cols));
            for (auto &value : row) {
                pda::read(costs, value);
                pda::require(pda::nonnegative(value), "Invalid cell cost");
                c.max_cellcost = std::max(c.max_cellcost, value);
            }
        }
    }
    pda::end(costs);
    return result;
}
} // namespace lab4

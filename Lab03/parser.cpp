#include "legalizer.hpp"
#include "pda/io.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace lab3 {
Input parse_placement(std::istream &input) {
    Input result;
    pda::expect(input, "Alpha");
    pda::read(input, result.alpha);
    pda::expect(input, "Beta");
    pda::read(input, result.beta);
    pda::require(pda::nonnegative(result.alpha) && pda::nonnegative(result.beta),
                 "Invalid weights");
    pda::expect(input, "DieSize");
    auto &d = result.die;
    pda::read(input, d.lLX, d.lLY, d.uRX, d.uRY);
    pda::require(std::isfinite(d.lLX) && std::isfinite(d.lLY) && pda::positive(d.uRX - d.lLX) &&
                     pda::positive(d.uRY - d.lLY),
                 "Invalid die");
    std::unordered_set<std::string> names;
    std::string token;
    while (input >> token) {
        if (token == "PlacementRows") {
            PlacementRow row{};
            pda::read(input, row.startX, row.startY, row.siteWidth, row.siteHeight, row.NumOfSites);
            pda::require(std::isfinite(row.startX) && std::isfinite(row.startY) &&
                             pda::positive(row.siteWidth) && pda::positive(row.siteHeight) &&
                             pda::positive(row.NumOfSites) &&
                             row.NumOfSites == std::floor(row.NumOfSites),
                         "Invalid placement row");
            pda::require(row.startX >= d.lLX && row.startY >= d.lLY &&
                             row.startX + row.siteWidth * row.NumOfSites <= d.uRX &&
                             row.startY + row.siteHeight <= d.uRY,
                         "Placement row outside die");
            result.rows.push_back(row);
        } else {
            Cell cell{};
            cell.name = token;
            std::string fixed;
            pda::read(input, cell.x, cell.y, cell.width, cell.height, fixed);
            pda::require(names.insert(token).second && (fixed == "FIX" || fixed == "NOTFIX"),
                         "Duplicate cell or invalid FIX attribute");
            pda::require(pda::positive(cell.width) && pda::positive(cell.height) &&
                             std::isfinite(cell.x) && std::isfinite(cell.y),
                         "Invalid cell geometry");
            pda::require(cell.x >= d.lLX && cell.y >= d.lLY && cell.x + cell.width <= d.uRX &&
                             cell.y + cell.height <= d.uRY,
                         "Cell outside die");
            cell.fixed = fixed == "FIX";
            cell.opt_x = cell.x;
            cell.opt_y = cell.y;
            result.cells.push_back(cell);
        }
    }
    pda::require(!result.rows.empty(), "No placement rows");
    // The course contract requires uniform, continuous rows; enforce rather than
    // silently placing a multi-height cell over holes or a different site grid.
    auto rows = result.rows;
    std::sort(rows.begin(), rows.end(),
              [](const auto &a, const auto &b) { return a.startY < b.startY; });
    for (std::size_t i = 1; i < rows.size(); ++i) {
        pda::require(rows[i].startX == rows[0].startX && rows[i].siteWidth == rows[0].siteWidth &&
                         rows[i].siteHeight == rows[0].siteHeight &&
                         rows[i].NumOfSites == rows[0].NumOfSites &&
                         rows[i].startY == rows[i - 1].startY + rows[i - 1].siteHeight,
                     "Nonuniform or discontinuous rows are unsupported");
    }
    return result;
}
std::vector<Step> parse_steps(std::istream &input) {
    std::vector<Step> steps;
    std::string line;
    while (std::getline(input, line)) {
        if (line.find_first_not_of(" \t\r") == std::string::npos)
            continue;
        std::istringstream row(line);
        pda::expect(row, "Banking_Cell:");
        Step step;
        std::unordered_set<std::string> names;
        std::string name;
        while (true) {
            pda::read(row, name);
            if (name == "-->")
                break;
            pda::require(names.insert(name).second, "Duplicate banked cell");
            step.remove.push_back(name);
        }
        pda::require(!step.remove.empty(), "Empty banking list");
        auto &c = step.cell;
        pda::read(row, c.name, c.x, c.y, c.width, c.height);
        pda::end(row);
        pda::require(std::isfinite(c.x) && std::isfinite(c.y) && pda::positive(c.width) &&
                         pda::positive(c.height),
                     "Invalid merged cell");
        c.opt_x = c.x;
        c.opt_y = c.y;
        c.fixed = false;
        steps.push_back(std::move(step));
    }
    return steps;
}
} // namespace lab3

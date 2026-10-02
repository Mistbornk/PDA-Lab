#pragma once
#include <cfloat>
#include <map>
#include <string>
#include <vector>

namespace lab3 {
struct Cell {
    std::string name;
    double x, y;
    double opt_x, opt_y;
    double width, height;
    bool fixed;
    // Overload operator==
    bool operator==(const Cell &other) const { return this->name == other.name; }
};

struct PlacementRow {
    double startX, startY;
    double siteWidth, siteHeight;
    double NumOfSites;
};

struct Die {
    double lLX, lLY;
    double uRX, uRY;
};

} // namespace lab3

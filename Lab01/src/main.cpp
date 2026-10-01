#include "Block.h"
#include "pda/io.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace {
struct Command {
    int id, x, y, width, height;
};
std::vector<Command> parse(std::istream &input, int &outline_x, int &outline_y) {
    pda::read(input, outline_x, outline_y);
    pda::require(outline_x > 0 && outline_y > 0 && outline_x <= 200000 && outline_y <= 200000,
                 "Outline must be within [1,200000]");
    std::string line;
    std::getline(input, line);
    pda::require(line.find_first_not_of(" \t\r") == std::string::npos, "Unexpected outline fields");
    std::unordered_set<int> ids;
    std::vector<Command> commands;
    while (std::getline(input, line)) {
        if (line.find_first_not_of(" \t\r") == std::string::npos)
            continue;
        std::istringstream row(line);
        std::string tag;
        pda::read(row, tag);
        Command c{};
        if (tag == "P") {
            pda::read(row, c.x, c.y);
        } else {
            std::size_t used = 0;
            c.id = std::stoi(tag, &used);
            pda::require(used == tag.size() && c.id > 0 && c.id < 2147483647 &&
                             ids.insert(c.id).second,
                         "Invalid or duplicate block ID");
            pda::read(row, c.x, c.y, c.width, c.height);
            pda::require(c.width > 0 && c.height > 0 && c.width <= outline_x &&
                             c.height <= outline_y,
                         "Invalid block dimensions");
            pda::require(c.x <= outline_x - c.width && c.y <= outline_y - c.height,
                         "Block outside outline");
        }
        pda::end(row);
        pda::require(c.x >= 0 && c.x < outline_x && c.y >= 0 && c.y < outline_y,
                     "Coordinate outside outline");
        commands.push_back(c);
    }
    return commands;
}
} // namespace

int main(int argc, char *argv[]) {
    try {
        pda::require(argc >= 3,
                     "Usage: Lab1 input.txt output.txt [--stitches indexed|scan] [--stats]");
        bool indexed = true, stats = false;
        for (int i = 3; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--stats") {
                stats = true;
                continue;
            }
            pda::require(option == "--stitches" && i + 1 < argc, "Invalid stitch option");
            const std::string value = argv[++i];
            pda::require(value == "indexed" || value == "scan", "Invalid stitch mode");
            indexed = value == "indexed";
        }
        auto input = pda::input_file(argv[1]);
        int outline_x = 0, outline_y = 0;
        const auto commands = parse(input, outline_x, outline_y);
        TileList tiles(indexed);
        tiles.push_back(std::make_unique<Block>(0, 0, 0, outline_x, outline_y, false));
        std::vector<std::pair<int, int>> points;
        for (const auto &c : commands) {
            if (c.id == 0) {
                const auto *block = Point_Finding(tiles.front().get(), c.x, c.y);
                pda::require(block != nullptr, "Broken corner stitch during point query");
                points.emplace_back(block->x, block->y);
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
        auto output = pda::output_file(argv[2]);
        output << tiles.size() << '\n';
        for (auto *block : solids) {
            int solid = 0, space = 0;
            for (const auto *neighbor : Neighbor_Finding(block)) {
                if (neighbor->isSolid)
                    ++solid;
                else
                    ++space;
            }
            output << block->index << ' ' << solid << ' ' << space << '\n';
        }
        for (const auto &point : points)
            output << point.first << ' ' << point.second << '\n';
        output.flush();
        if (stats)
            std::cerr << "{\"stitch_queries\":" << tiles.stitch_queries
                      << ",\"candidate_visits\":" << tiles.candidate_visits << "}\n";
    } catch (const std::exception &error) {
        std::cerr << "Lab1: " << error.what() << '\n';
        return 1;
    }
}

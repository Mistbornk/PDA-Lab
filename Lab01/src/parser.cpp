#include "layout.hpp"
#include "pda/io.hpp"
#include <sstream>
#include <unordered_set>
namespace lab1 {
Input parse(std::istream &input) {
    Input result;
    auto &outline_x = result.width;
    auto &outline_y = result.height;
    pda::read(input, outline_x, outline_y);
    pda::input_require(outline_x > 0 && outline_y > 0 && outline_x <= 200000 && outline_y <= 200000,
                       "Outline must be within [1,200000]");
    std::string line;
    std::getline(input, line);
    pda::input_require(line.find_first_not_of(" \t\r") == std::string::npos,
                       "Unexpected outline fields");
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
            c.id = pda::integer_token(tag);
            pda::input_require(c.id > 0 && c.id < 2147483647 && ids.insert(c.id).second,
                               "Invalid or duplicate block ID");
            pda::read(row, c.x, c.y, c.width, c.height);
            pda::input_require(c.width > 0 && c.height > 0 && c.width <= outline_x &&
                                   c.height <= outline_y,
                               "Invalid block dimensions");
            pda::input_require(c.x <= outline_x - c.width && c.y <= outline_y - c.height,
                               "Block outside outline");
        }
        pda::end(row);
        pda::input_require(c.x >= 0 && c.x < outline_x && c.y >= 0 && c.y < outline_y,
                           "Coordinate outside outline");
        commands.push_back(c);
    }
    result.commands = std::move(commands);
    return result;
}
} // namespace lab1

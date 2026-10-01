#include "annealer.hpp"
#include "pda/io.hpp"
namespace lab2 {
void write_report(const std::string &path, const Result &result, double elapsed_seconds) {
    auto output = pda::output_file(path);
    const auto &c = result.cost;
    output << c.cost << '\n'
           << c.wirelength << '\n'
           << c.area << '\n'
           << c.width << ' ' << c.height << '\n'
           << elapsed_seconds << '\n';
    for (const auto &b : result.blocks)
        output << b.name << ' ' << b.x << ' ' << b.y << ' ' << b.x + b.width << ' '
               << b.y + b.height << '\n';
    output.flush();
}
} // namespace lab2

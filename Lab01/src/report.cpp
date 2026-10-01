#include "layout.hpp"
#include <ostream>
namespace lab1 {
void write_report(std::ostream &output, const Result &result) {
    output << result.tiles << '\n';
    for (const auto &n : result.neighbors)
        output << n.id << ' ' << n.solid << ' ' << n.space << '\n';
    for (const auto &p : result.points)
        output << p.first << ' ' << p.second << '\n';
    output.flush();
}
} // namespace lab1

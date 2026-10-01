#pragma once
#include "input.hpp"
#include <iosfwd>
namespace lab4 {
void route_legacy(Input input, std::ostream &output);
// Greedy net order is preserved. Each net minimizes incremental cost with prior
// usages fixed, on a (cell, arrival layer) graph. This is not joint net optimization.
void route_layered(Input input, std::ostream &output, std::ostream *statistics = nullptr);
} // namespace lab4

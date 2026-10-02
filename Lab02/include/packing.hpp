#pragma once
#include "model.hpp"
namespace lab2 {
struct PackingWorkspace {
    struct Segment {
        int x, height;
    };
    PackingMode mode;
    std::vector<Segment> skyline;
    std::vector<int> dense, pending;
    explicit PackingWorkspace(PackingMode selected = PackingMode::Skyline) : mode(selected) {}
};
// Precondition: placement is a valid rooted B*-tree. Packing changes x/y only.
void pack(Placement &placement, int initial_contour_width);
void pack(Placement &placement, int initial_contour_width, PackingWorkspace &workspace);
long long hpwl(const Problem &problem, const std::vector<Block> &blocks, bool integer_pins = true);
Cost evaluate(const Problem &problem, Placement &placement, double alpha, bool outline_penalty,
              bool integer_pins = true, PackingWorkspace *workspace = nullptr,
              EvaluationProfile *profile = nullptr);
} // namespace lab2

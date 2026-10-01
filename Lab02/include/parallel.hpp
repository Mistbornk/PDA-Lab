#pragma once
#include "annealer.hpp"
namespace lab2 {
struct MultiOptions {
    unsigned restarts = 1;
    unsigned threads = 1;
};
struct Attempt {
    unsigned seed = 0;
    bool legal = false;
    std::uint64_t iterations = 0;
    double cpu_seconds = 0;
    Cost cost;
};
struct MultiResult {
    Result best;
    unsigned winning_seed = 0;
    unsigned threads = 1;
    std::vector<Attempt> attempts;
    double cpu_seconds = 0;
    std::uint64_t total_iterations = 0;
};
// Runs independent trajectories at seeds base, base+1, ... . Fixed-work results
// are independent of scheduling; ties select the earliest restart.
MultiResult solve_multi(const Problem &problem, const Options &options, const MultiOptions &multi);
} // namespace lab2

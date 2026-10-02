#include "annealer.hpp"
#include "packing.hpp"
#include "pda/io.hpp"
#include "tree.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
namespace lab2 {
Result solve_progress(const Problem &problem, const Options &options, double start) {
    const auto &policy = options.schedule;
    pda::input_require(pda::positive(policy.temperature) && policy.temperature <= 1e6 &&
                           pda::positive(policy.cooling) && policy.cooling < 1 &&
                           pda::positive(policy.outline_weight) && policy.outline_weight <= 1e6 &&
                           policy.epoch_moves > 0 && policy.epoch_moves <= 1000 &&
                           policy.reheat_epochs > 0 && policy.reheat_epochs <= 10000,
                       "Invalid annealing policy parameters");
    pda::input_require(!problem.blocks.empty() && problem.blocks.size() < 500 &&
                           (options.iterations || pda::positive(options.seconds)) &&
                           problem.outline_width > 0 && problem.outline_height > 0 &&
                           options.alpha >= 0 && options.alpha <= 1,
                       "Invalid floorplanning problem/options");
    Placement placement{-1, problem.blocks, {}};
    PackingWorkspace workspace(options.packing);
    UndoJournal journal;
    Random random(options.seed);
    std::mt19937 shuffle(options.seed);
    initialize_tree(placement, shuffle);
    SearchStats stats;
    std::uint64_t iterations = 0;
    const auto evaluate_current = [&] {
        return evaluate(problem, placement, options.alpha, false, options.integer_pins, &workspace,
                        options.diagnostics && iterations % 256 == 0 ? &stats.evaluation : nullptr);
    };
    Cost current = evaluate_current();
    const double scale = std::max(1., static_cast<double>(current.cost));
    const int count = static_cast<int>(placement.blocks.size());
    const auto epoch_size =
        static_cast<std::uint64_t>(policy.epoch_moves) * placement.blocks.size();
    std::optional<Result> best;
    const auto capture = [&](const Cost &cost) {
        if (cost.width <= problem.outline_width && cost.height <= problem.outline_height &&
            (!best || cost.cost < best->cost.cost))
            best = Result{placement.blocks, cost, iterations};
        if (options.diagnostics)
            observe(stats, problem, cost, iterations, start, best ? best->cost.cost : -1,
                    options.trace_every);
    };
    capture(current);
    bool refining = best.has_value();
    const auto score = [&](const Cost &cost) {
        const double excess = outline_excess(problem, cost);
        if (options.policy == SearchPolicy::FeasibilityFirst && !refining)
            return excess + 0.01 * (static_cast<double>(cost.width) / problem.outline_width +
                                    static_cast<double>(cost.height) / problem.outline_height);
        return static_cast<double>(cost.cost) / scale + policy.outline_weight * excess;
    };
    std::uint64_t phase_start = 0;
    while (options.iterations ? iterations < options.iterations
                              : search_seconds() - start < options.seconds) {
        const auto epoch = ((iterations - phase_start) / epoch_size) % policy.reheat_epochs;
        const double temperature = std::max(
            1e-7, policy.temperature * std::pow(policy.cooling, static_cast<double>(epoch)));
        std::optional<Placement> snapshot;
        if (options.undo_journal)
            journal.begin(placement);
        else
            snapshot = placement;
        const int operation = count == 1 ? 0 : random() % 3;
        const int a = random() % count;
        auto *undo = options.undo_journal ? &journal : nullptr;
        if (operation == 0)
            rotate_block(placement, a, undo);
        else {
            int b = random() % count;
            while (a == b)
                b = random() % count;
            if (operation == 1)
                move_node(placement, a, b, random, undo);
            else
                swap_nodes(placement, a, b, undo);
        }
        ++iterations;
        const Cost candidate = evaluate_current();
        const double delta = score(candidate) - score(current);
        const bool legal =
            candidate.width <= problem.outline_width && candidate.height <= problem.outline_height;
        // A feasible-first refinement stays feasible. The progress ablation can
        // still cross infeasible states and retains all best legal candidates.
        const bool permitted =
            options.policy != SearchPolicy::FeasibilityFirst || !refining || legal;
        const double draw = static_cast<double>(random()) / Random::maximum;
        const bool accept =
            permitted &&
            ((options.policy == SearchPolicy::FeasibilityFirst && !refining && legal) ||
             delta <= 0 || std::exp(-delta / temperature) > draw);
        capture(candidate);
        if (accept) {
            current = candidate;
            if (options.diagnostics) {
                ++stats.accepted;
                stats.uphill_accepted += delta > 0;
            }
        } else if (options.undo_journal)
            journal.rollback(placement);
        else
            placement = std::move(*snapshot);
        if (!refining && legal && options.policy == SearchPolicy::FeasibilityFirst) {
            refining = true;
            phase_start = iterations;
        }
    }
    if (!best)
        throw NoLegalPlacement(iterations, std::move(stats));
    if (options.diagnostics && options.trace_every)
        observe(stats, problem, best->cost, iterations, start, best->cost.cost, 1);
    best->iterations = iterations;
    best->stats = std::move(stats);
    return std::move(*best);
}
} // namespace lab2

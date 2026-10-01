#include "annealer.hpp"
#include "packing.hpp"
#include "pda/io.hpp"
#include "timing.hpp"
#include "tree.hpp"
#include <cmath>
#include <random>
using namespace std;
namespace lab2 {
namespace {
class Annealer {
    const Problem &problem;
    const Options &options;
    Placement placement;
    PackingWorkspace packing;
    UndoJournal journal;
    int numBlocks, outline_width, outline_height;
    vector<Block> bestblocks;
    Cost bestcost;
    double start;
    Random random;
    mt19937 g;
    uint64_t iteration_limit, iterations = 0;
    double time_limit;
    Cost CalCost(bool penalty) {
        return evaluate(problem, placement, options.alpha, penalty, options.integer_pins, &packing);
    }
    bool isAccept(const double &T, const double &diff) {
        double RandNum = static_cast<double>(random()) / Random::maximum;
        return T > 0 && exp(-diff / T) > RandNum;
    }
    Cost Perturb(bool findOutline) {
        int op = numBlocks == 1 ? 0 : random() % 3;
        switch (op) {
        case 0: {
            int randID = random() % numBlocks;
            rotate_block(placement, randID, options.undo_journal ? &journal : nullptr);
            break;
        }
        case 1: {
            int randID1 = random() % numBlocks;
            int randID2 = random() % numBlocks;
            while (randID1 == randID2)
                randID2 = random() % numBlocks;
            move_node(placement, randID1, randID2, random,
                      options.undo_journal ? &journal : nullptr);
            break;
        }
        case 2: {
            int randID1 = random() % numBlocks;
            int randID2 = random() % numBlocks;
            while (randID1 == randID2)
                randID2 = random() % numBlocks;
            swap_nodes(placement, randID1, randID2, options.undo_journal ? &journal : nullptr);
            break;
        }
        }
        Cost current_cost = CalCost(findOutline);
        return current_cost;
    }
    bool checkTime(int &seconds_to_fit_outline, int &max_seconds_to_fit_outline,
                   bool in_fixed_outline) {
        return seconds_to_fit_outline >= max_seconds_to_fit_outline && !in_fixed_outline;
    }
    void SimulatedAnneling() {
        initialize_tree(placement, g);
        bool random_expanded = true;
        bool in_fixed_outline = false;
        bestcost = CalCost(random_expanded);

        bestblocks = placement.blocks;

        double P = 0.95;
        double r = 0.9;
        int N = 20 * numBlocks;
        double T0 = -static_cast<double>(bestcost.cost) * numBlocks / log(P);

        double T = T0;
        int total_move = 0;
        int uphill = 0;
        Cost old_cost = bestcost;

        double init_time = search_seconds();
        double seconds_to_fit_outline_time = init_time;
        double round_count_start = search_seconds();
        int max_seconds_to_fit_outline = numBlocks / 2;

        int seconds_to_fit_outline = 0;
        int i = 1;

        total_move = 0;
        uphill = 0;

        while (iteration_limit ? iterations < iteration_limit
                               : search_seconds() - start < time_limit) {
            total_move = 0;
            uphill = 0;

            while (uphill <= N && total_move <= 2 * N &&
                   (iteration_limit ? iterations < iteration_limit
                                    : search_seconds() - start < time_limit)) {
                vector<Block> tempblocks;
                vector<Node> tempbstartree;
                if (options.undo_journal)
                    journal.begin(placement);
                else {
                    tempblocks = placement.blocks;
                    tempbstartree = placement.tree;
                }
                int prev_root_block = placement.root;

                Cost cur_cost = old_cost;

                int count_time = 0;

                if (random_expanded) {
                    count_time = static_cast<int>(search_seconds() - round_count_start);
                    if (!iteration_limit && count_time > 1.5) {
                        i = i % 4 + 1;
                        round_count_start = search_seconds();
                    }
                    int tmp = iteration_limit ? 1 : i;
                    while (tmp--)
                        cur_cost = Perturb(random_expanded);
                } else {
                    cur_cost = Perturb(random_expanded);
                }

                total_move++;
                ++iterations;

                double delta_cost =
                    static_cast<double>(cur_cost.cost) - static_cast<double>(old_cost.cost);
                bool in_outline_after_perturb = false;
                bool acceptPoor = isAccept(T, delta_cost);
                if (delta_cost <= 0 || old_cost.cost == 0 || ((acceptPoor) && random_expanded)) {

                    if (cur_cost.width <= outline_width && cur_cost.height <= outline_height) {

                        in_outline_after_perturb = true;

                        if (in_fixed_outline && cur_cost.cost <= bestcost.cost &&
                            cur_cost.cost != 0) {
                            bestcost = cur_cost;
                            bestblocks = placement.blocks;
                        } else {
                            in_fixed_outline = true;
                            random_expanded = false;

                            bestcost = cur_cost;
                            bestblocks = placement.blocks;

                            Cost tmp_cost = CalCost(random_expanded);
                            bestcost.cost = tmp_cost.cost;
                            cur_cost.cost = tmp_cost.cost;
                        }
                    }

                    if (cur_cost.cost <= bestcost.cost && random_expanded) {

                        bestcost = cur_cost;
                        bestblocks = placement.blocks;
                    }

                    if (random_expanded || (!random_expanded && in_outline_after_perturb) ||
                        cur_cost.cost != 0) {

                        old_cost = cur_cost;
                    }
                    if (delta_cost > 0)
                        uphill++;
                } else {
                    if (options.undo_journal)
                        journal.rollback(placement);
                    else {
                        placement.root = prev_root_block;
                        placement.blocks = std::move(tempblocks);
                        placement.tree = std::move(tempbstartree);
                    }
                }
            }

            T *= r;

            seconds_to_fit_outline =
                static_cast<int>(search_seconds() - seconds_to_fit_outline_time);

            if (!iteration_limit &&
                checkTime(seconds_to_fit_outline, max_seconds_to_fit_outline, in_fixed_outline)) {
                seconds_to_fit_outline = 0;
                seconds_to_fit_outline_time = search_seconds();
                T = T0;
            }
        }
    }

  public:
    Annealer(const Problem &input, const Options &settings, double began)
        : problem(input), options(settings), placement{-1, input.blocks, {}},
          packing(settings.packing), numBlocks(static_cast<int>(input.blocks.size())),
          outline_width(input.outline_width), outline_height(input.outline_height), start(began),
          random(settings.seed), g(settings.seed), iteration_limit(settings.iterations),
          time_limit(settings.seconds) {}
    Result run() {
        SimulatedAnneling();
        if (bestcost.width > outline_width || bestcost.height > outline_height)
            throw NoLegalPlacement(iterations);
        bestcost.cost =
            static_cast<long long>(options.alpha * static_cast<double>(bestcost.area) +
                                   (1 - options.alpha) * static_cast<double>(bestcost.wirelength));
        return {std::move(bestblocks), bestcost, iterations};
    }
};
} // namespace
Result solve(const Problem &problem, const Options &options, double start) {
    return Annealer(problem, options, start).run();
}
} // namespace lab2

#include "annealer.hpp"
#include "parallel.hpp"
#include "parser.hpp"
#include "pda/io.hpp"
#include <cmath>
#include <iostream>
#include <limits>
using namespace std;
int main(int argc, char *argv[]) {
    bool print_stats = false;
    try {
        pda::require(argc >= 5,
                     "Usage: Lab2 alpha input.block input.nets output.rpt [--seed N] "
                     "[--iterations N | --seconds S] [--hpwl ids|strings] [--packing "
                     "dense|skyline] [--rollback snapshot|journal] [--restarts N --threads N] "
                     "[--policy legacy|progress|feasibility] [--trace-every N] [--stats]");
        lab2::Options options;
        lab2::MultiOptions multi;
        const auto start = lab2::search_seconds();
        options.seed = static_cast<unsigned>(time(nullptr));
        bool seconds_set = false, policy_parameters = false;
        for (int i = 5; i < argc; ++i) {
            const string option = argv[i];
            if (option == "--stats") {
                print_stats = true;
                continue;
            }
            pda::require(i + 1 < argc, "Missing option value");
            const string value = argv[++i];
            std::size_t used = 0;
            if (option == "--policy") {
                pda::input_require(value == "legacy" || value == "progress" ||
                                       value == "feasibility",
                                   "Invalid search policy");
                options.policy = value == "legacy"     ? lab2::SearchPolicy::Legacy
                                 : value == "progress" ? lab2::SearchPolicy::Progress
                                                       : lab2::SearchPolicy::FeasibilityFirst;
                continue;
            }
            if (option == "--temperature" || option == "--cooling" ||
                option == "--outline-weight") {
                policy_parameters = true;
                const double number = stod(value, &used);
                pda::input_require(used == value.size() && pda::positive(number),
                                   "Invalid policy parameter");
                if (option == "--temperature")
                    options.schedule.temperature = number;
                else if (option == "--cooling")
                    options.schedule.cooling = number;
                else
                    options.schedule.outline_weight = number;
                continue;
            }
            if (option == "--epoch-moves" || option == "--reheat-epochs") {
                policy_parameters = true;
                const int count = pda::integer_token(value);
                pda::input_require(count > 0 && count <= 10000, "Invalid policy epoch count");
                if (option == "--epoch-moves")
                    options.schedule.epoch_moves = static_cast<unsigned>(count);
                else
                    options.schedule.reheat_epochs = static_cast<unsigned>(count);
                continue;
            }
            if (option == "--rollback") {
                pda::require(value == "snapshot" || value == "journal", "Invalid rollback mode");
                options.undo_journal = value == "journal";
                continue;
            }
            if (option == "--packing") {
                pda::require(value == "dense" || value == "skyline", "Invalid packing mode");
                options.packing =
                    value == "dense" ? lab2::PackingMode::Dense : lab2::PackingMode::Skyline;
                continue;
            }
            if (option == "--hpwl") {
                pda::require(value == "ids" || value == "strings", "Invalid HPWL mode");
                options.integer_pins = value == "ids";
                continue;
            }
            if (option == "--seconds") {
                options.seconds = stod(value, &used);
                seconds_set = true;
                pda::require(pda::positive(options.seconds), "Invalid time budget");
            } else if (option == "--iterations" || option == "--seed" || option == "--restarts" ||
                       option == "--threads" || option == "--trace-every") {
                pda::require(!value.empty() && value.find('-') == string::npos,
                             "Expected unsigned integer");
                auto n = stoull(value, &used);
                if (option == "--trace-every") {
                    pda::input_require(n > 0, "Trace interval must be positive");
                    options.trace_every = n;
                    print_stats = true;
                } else if (option == "--iterations") {
                    options.iterations = n;
                    pda::require(n > 0, "Iterations must be positive");
                } else if (option == "--restarts" || option == "--threads" ||
                           option == "--trace-every") {
                    pda::require(n >= 1 && n <= 1024, "Restarts and threads must be in [1,1024]");
                    (option == "--restarts" ? multi.restarts : multi.threads) =
                        static_cast<unsigned>(n);
                } else {
                    pda::require(n <= std::numeric_limits<unsigned>::max(), "Seed out of range");
                    options.seed = static_cast<unsigned>(n);
                }
            } else
                throw std::runtime_error("Unknown option: " + option);
            pda::require(used == value.size(), "Invalid numeric option");
        }
        pda::require(!(seconds_set && options.iterations), "Choose seconds OR iterations");
        std::size_t used = 0;
        options.alpha = stod(argv[1], &used);
        pda::require(used == string(argv[1]).size() && std::isfinite(options.alpha) &&
                         options.alpha >= 0 && options.alpha <= 1,
                     "alpha must be in [0,1]");

        pda::input_require(!policy_parameters || options.policy != lab2::SearchPolicy::Legacy,
                           "Policy parameters require progress or feasibility mode");
        options.diagnostics = print_stats;
        auto problem = lab2::parse(argv[2], argv[3]);
        if (multi.restarts == 1) {
            auto result = lab2::solve(problem, options, start);
            lab2::write_report(argv[4], result, lab2::search_seconds() - start);
            if (print_stats) {
                cerr << "{\"seed\":" << options.seed << ",\"iterations\":" << result.iterations
                     << ",\"threads\":1,\"restarts\":1,\"search\":";
                lab2::write_search_stats(cerr, result.stats);
                cerr << "}\n";
            }
        } else {
            const auto result = lab2::solve_multi(problem, options, multi);
            lab2::write_report(argv[4], result.best, result.cpu_seconds);
            if (print_stats) {
                cerr << "{\"seed\":" << result.winning_seed
                     << ",\"iterations\":" << result.best.iterations
                     << ",\"total_iterations\":" << result.total_iterations
                     << ",\"threads\":" << result.threads
                     << ",\"restarts\":" << result.attempts.size() << ",\"attempts\":[";
                for (std::size_t i = 0; i < result.attempts.size(); ++i) {
                    const auto &a = result.attempts[i];
                    if (i)
                        cerr << ',';
                    cerr << "{\"seed\":" << a.seed << ",\"legal\":" << (a.legal ? "true" : "false")
                         << ",\"iterations\":" << a.iterations
                         << ",\"cpu_seconds\":" << a.cpu_seconds;
                    if (a.legal)
                        cerr << ",\"objective\":" << a.cost.cost;
                    if (options.diagnostics) {
                        cerr << ",\"search\":";
                        lab2::write_search_stats(cerr, a.stats);
                    }
                    cerr << '}';
                }
                cerr << "]}\n";
            }
        }
    } catch (const lab2::NoLegalPlacement &error) {
        if (print_stats) {
            cerr << "{\"legal\":false,\"iterations\":" << error.iterations << ",\"search\":";
            lab2::write_search_stats(cerr, error.stats);
            cerr << "}\n";
        }
        cerr << "Lab2: " << error.what() << '\n';
        return 3;
    } catch (const exception &error) {
        cerr << "Lab2: " << error.what() << '\n';
        return 1;
    }
}

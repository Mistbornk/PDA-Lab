#include "annealer.hpp"
#include "parallel.hpp"
#include "parser.hpp"
#include "pda/io.hpp"
#include <cmath>
#include <iostream>
#include <limits>
using namespace std;
int main(int argc, char *argv[]) {
    try {
        pda::require(
            argc >= 5,
            "Usage: Lab2 alpha input.block input.nets output.rpt [--seed N] "
            "[--iterations N | --seconds S] [--hpwl ids|strings] [--packing "
            "dense|skyline] [--rollback snapshot|journal] [--restarts N --threads N] [--stats]");
        lab2::Options options;
        lab2::MultiOptions multi;
        bool print_stats = false;
        const auto start = lab2::search_seconds();
        options.seed = static_cast<unsigned>(time(nullptr));
        bool seconds_set = false;
        for (int i = 5; i < argc; ++i) {
            const string option = argv[i];
            if (option == "--stats") {
                print_stats = true;
                continue;
            }
            pda::require(i + 1 < argc, "Missing option value");
            const string value = argv[++i];
            std::size_t used = 0;
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
                       option == "--threads") {
                pda::require(!value.empty() && value[0] != '-', "Expected unsigned integer");
                auto n = stoull(value, &used);
                if (option == "--iterations") {
                    options.iterations = n;
                    pda::require(n > 0, "Iterations must be positive");
                } else if (option == "--restarts" || option == "--threads") {
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

        auto problem = lab2::parse(argv[2], argv[3]);
        if (multi.restarts == 1) {
            auto result = lab2::solve(problem, options, start);
            lab2::write_report(argv[4], result, lab2::search_seconds() - start);
            if (print_stats)
                cerr << "{\"seed\":" << options.seed << ",\"iterations\":" << result.iterations
                     << ",\"threads\":1,\"restarts\":1}\n";
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
                    cerr << '}';
                }
                cerr << "]}\n";
            }
        }
    } catch (const exception &error) {
        cerr << "Lab2: " << error.what() << '\n';
        return 1;
    }
}

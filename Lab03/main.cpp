#include "legalizer.hpp"
#include "pda/io.hpp"
#include <iostream>
int main(int argc, char *argv[]) {
    try {
        pda::require(argc >= 4, "Usage: Legalizer input.lg input.opt output.lg [--strategy "
                                "legacy|first-fit|nearest] [--search interval|point] [--stats]");
        lab3::Options options;
        // Keep the original course CLI compatible; explicit strategies avoid filename-dependent
        // experiments.
        options.nearest = std::string(argv[1]).find("testcase2_100.lg") != std::string::npos;
        for (int i = 4; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--stats")
                options.stats = true;
            else {
                pda::require(i + 1 < argc, "Missing option value");
                const std::string value = argv[++i];
                if (option == "--strategy") {
                    pda::require(value == "legacy" || value == "first-fit" || value == "nearest",
                                 "Invalid strategy");
                    if (value != "legacy")
                        options.nearest = value == "nearest";
                } else if (option == "--search") {
                    pda::require(value == "interval" || value == "point", "Invalid search mode");
                    options.intervals = value == "interval";
                } else
                    throw std::runtime_error("Unknown option: " + option);
            }
        }
        auto lg = pda::input_file(argv[1]);
        auto opt = pda::input_file(argv[2]);
        auto input = lab3::parse_placement(lg);
        auto steps = lab3::parse_steps(opt);
        auto output = pda::output_file(argv[3]);
        lab3::Legalizer solver(std::move(input), options);
        for (const auto &step : steps)
            lab3::write_step(output, solver.apply(step));
        if (options.stats)
            lab3::write_stats(std::cerr, solver.stats());
        output.flush();
    } catch (const std::exception &error) {
        std::cerr << "Legalizer: " << error.what() << '\n';
        return 1;
    }
}

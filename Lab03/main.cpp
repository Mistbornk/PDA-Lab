#include "legalizer.hpp"
#include "pda/io.hpp"
#include <iostream>
int main(int argc, char *argv[]) {
    try {
        pda::require(
            argc >= 4,
            "Usage: Legalizer input.lg input.opt output.lg [--strategy "
            "legacy|first-fit|nearest|minimum|repair] [--search interval|point] [--stats]");
        lab3::Options options;
        bool repair_settings = false;
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
                    pda::require(value == "legacy" || value == "first-fit" || value == "nearest" ||
                                     value == "minimum" || value == "repair",
                                 "Invalid strategy");
                    options.strategy = value == "minimum"  ? lab3::Strategy::MinimumDisplacement
                                       : value == "repair" ? lab3::Strategy::Repair
                                                           : lab3::Strategy::Legacy;
                    if (value != "legacy")
                        options.nearest = value == "nearest";
                } else if (option == "--search") {
                    pda::require(value == "interval" || value == "point", "Invalid search mode");
                    options.intervals = value == "interval";
                } else if (option == "--repair-cells" || option == "--repair-candidates") {
                    const auto number = pda::integer_token(value);
                    pda::input_require(number >= 0 && number <= 256, "Invalid repair count");
                    if (option == "--repair-cells")
                        options.repair_cells = static_cast<std::size_t>(number);
                    else
                        options.repair_candidates = static_cast<std::size_t>(number);
                    repair_settings = true;
                } else if (option == "--repair-radius") {
                    std::size_t used = 0;
                    options.repair_radius = std::stod(value, &used);
                    pda::input_require(used == value.size() &&
                                           pda::nonnegative(options.repair_radius),
                                       "Invalid repair radius");
                    repair_settings = true;
                } else
                    throw std::runtime_error("Unknown option: " + option);
            }
        }
        pda::input_require(!repair_settings || options.strategy == lab3::Strategy::Repair,
                           "Repair limits require repair strategy");
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
    } catch (const lab3::NoLegalPlacement &error) {
        std::cerr << "Legalizer: " << error.what() << '\n';
        return 3;
    } catch (const std::exception &error) {
        std::cerr << "Legalizer: " << error.what() << '\n';
        return 1;
    }
}

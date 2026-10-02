#include "pda/io.hpp"
#include "router.hpp"
#include <iostream>
int main(int argc, char *argv[]) {
    try {
        pda::input_require(argc >= 5,
                           "Usage: D2DGRter input.gmp input.gcl input.cst output.lg "
                           "[--router legacy|layered|negotiated] [--rounds N] [--history W] "
                           "[--stagnation N] [--seconds S] [--stats]");
        std::string mode = "legacy";
        bool stats = false, tuning = false, timed = false;
        lab4::RerouteOptions options;
        for (int i = 5; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--stats") {
                stats = true;
                continue;
            }
            pda::input_require(i + 1 < argc, "Missing router option value");
            const std::string value = argv[++i];
            if (option == "--router") {
                pda::input_require(value == "legacy" || value == "layered" || value == "negotiated",
                                   "Invalid router mode");
                mode = value;
            } else if (option == "--rounds" || option == "--stagnation") {
                const int count = pda::integer_token(value);
                pda::input_require(count >= 0 && count <= 1000, "Invalid iteration count");
                if (option == "--rounds")
                    options.rounds = static_cast<unsigned>(count);
                else
                    options.stagnation = static_cast<unsigned>(count);
                tuning = true;
            } else if (option == "--seconds" || option == "--history") {
                std::size_t used = 0;
                const double number = std::stod(value, &used);
                pda::input_require(used == value.size() && pda::nonnegative(number),
                                   "Invalid numeric option");
                if (option == "--seconds") {
                    options.seconds = number;
                    timed = true;
                } else {
                    options.history = number;
                    tuning = true;
                }
            } else
                throw pda::InputError("Unknown router option: " + option);
        }
        pda::input_require(!tuning || mode == "negotiated",
                           "Rerouting settings require negotiated mode");
        pda::input_require(!timed || mode != "legacy",
                           "Time budget requires layered or negotiated mode");
        auto input = lab4::parse(argv[1], argv[2], argv[3]);
        lab4::Result result;
        if (mode == "legacy")
            result = lab4::solve_legacy(input);
        else if (mode == "layered" && !timed)
            result = lab4::solve_layered(input);
        else {
            if (mode == "layered") {
                options.rounds = 0;
                options.history = 0;
            }
            result = lab4::solve_negotiated(input, options);
        }
        auto output = pda::output_file(argv[4]);
        lab4::write_report(output, input, result);
        if (stats && mode != "legacy")
            lab4::write_stats(std::cerr, result);
        output.flush();
    } catch (const lab4::RoutingBudgetExceeded &error) {
        std::cerr << "D2DGRter: " << error.what() << '\n';
        return 3;
    } catch (const pda::InputError &error) {
        std::cerr << "D2DGRter: " << error.what() << '\n';
        return 2;
    } catch (const std::exception &error) {
        std::cerr << "D2DGRter: " << error.what() << '\n';
        return 1;
    }
}

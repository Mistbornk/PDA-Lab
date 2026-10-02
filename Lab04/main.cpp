#include "pda/io.hpp"
#include "router.hpp"
#include <iostream>
int main(int argc, char *argv[]) {
    try {
        pda::require(argc >= 5, "Usage: D2DGRter input.gmp input.gcl input.cst output.lg [--router "
                                "legacy|layered] [--stats]");
        bool layered = false, stats = false;
        for (int i = 5; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--stats") {
                stats = true;
                continue;
            }
            pda::require(option == "--router" && i + 1 < argc, "Invalid router option");
            const std::string mode = argv[++i];
            pda::require(mode == "legacy" || mode == "layered", "Invalid router mode");
            layered = mode == "layered";
        }
        auto input = lab4::parse(argv[1], argv[2], argv[3]);
        const auto result = layered ? lab4::solve_layered(input) : lab4::solve_legacy(input);
        auto output = pda::output_file(argv[4]);
        lab4::write_report(output, input, result);
        if (stats && layered)
            lab4::write_stats(std::cerr, result);
        output.flush();
    } catch (const std::exception &error) {
        std::cerr << "D2DGRter: " << error.what() << '\n';
        return 1;
    }
}

#include "layout.hpp"
#include "pda/io.hpp"
#include <iostream>
int main(int argc, char *argv[]) {
    try {
        pda::require(argc >= 3,
                     "Usage: Lab1 input.txt output.txt [--stitches indexed|scan] [--stats]");
        bool indexed = true, stats = false;
        for (int i = 3; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--stats") {
                stats = true;
                continue;
            }
            pda::require(option == "--stitches" && i + 1 < argc, "Invalid stitch option");
            const std::string value = argv[++i];
            pda::require(value == "indexed" || value == "scan", "Invalid stitch mode");
            indexed = value == "indexed";
        }
        auto input = pda::input_file(argv[1]);
        auto problem = lab1::parse(input);
        auto result = lab1::solve(problem, indexed);
        auto output = pda::output_file(argv[2]);
        lab1::write_report(output, result);
        if (stats)
            std::cerr << "{\"stitch_queries\":" << result.stitch_queries
                      << ",\"candidate_visits\":" << result.candidate_visits << "}\n";
    } catch (const std::exception &error) {
        std::cerr << "Lab1: " << error.what() << '\n';
        return 1;
    }
}

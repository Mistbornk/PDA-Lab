#include "layout.hpp"
#include "pda/io.hpp"
#include <iostream>
int main(int argc, char *argv[]) {
    try {
        pda::require(argc >= 3, "Usage: Lab1 input.txt output.txt [--stitches indexed|scan] "
                                "[--geometry spatial|scan] [--stats]");
        bool indexed = true, spatial = true, stats = false;
        for (int i = 3; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--stats") {
                stats = true;
                continue;
            }
            pda::require(i + 1 < argc, "Missing option value");
            const std::string value = argv[++i];
            if (option == "--stitches") {
                pda::require(value == "indexed" || value == "scan", "Invalid stitch mode");
                indexed = value == "indexed";
            } else {
                pda::require(option == "--geometry" && (value == "spatial" || value == "scan"),
                             "Invalid geometry option");
                spatial = value == "spatial";
            }
        }
        auto input = pda::input_file(argv[1]);
        auto problem = lab1::parse(input);
        auto result = lab1::solve(problem, indexed, spatial);
        auto output = pda::output_file(argv[2]);
        lab1::write_report(output, result);
        if (stats)
            std::cerr << "{\"stitch_queries\":" << result.stitch_queries
                      << ",\"candidate_visits\":" << result.candidate_visits
                      << ",\"geometry_queries\":" << result.geometry_queries
                      << ",\"geometry_candidates\":" << result.geometry_candidates << "}\n";
    } catch (const std::exception &error) {
        std::cerr << "Lab1: " << error.what() << '\n';
        return 1;
    }
}

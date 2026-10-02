#include "parser.hpp"
#include "pda/io.hpp"
#include <algorithm>
#include <limits>
using namespace std;
namespace lab2 {
Problem parse(std::istream &blocks, std::istream &nets) {
    Problem problem;
    int numBlocks = 0, numTerminals = 0, numNets = 0;
    {
        auto &input = blocks;
        pda::expect(input, "Outline:");
        pda::read(input, problem.outline_width, problem.outline_height);
        pda::expect(input, "NumBlocks:");
        pda::read(input, numBlocks);
        pda::expect(input, "NumTerminals:");
        pda::read(input, numTerminals);
        pda::input_require(problem.outline_width > 0 && problem.outline_height > 0 &&
                               numBlocks > 0 && numBlocks < 500 && numTerminals >= 0 &&
                               numTerminals < 500,
                           "Invalid outline or counts");
        long long coordinate_bound = 0;
        for (int i = 0; i < numBlocks; ++i) {
            Block b;
            pda::read(input, b.name, b.width, b.height);
            pda::input_require(b.width > 0 && b.height > 0 &&
                                   problem.pin_ids.emplace(b.name, i).second,
                               "Invalid or duplicate macro");
            coordinate_bound += max(b.width, b.height);
            problem.blocks.push_back(b);
        }
        pda::input_require(coordinate_bound < std::numeric_limits<int>::max() / 4,
                           "Macro dimensions exceed supported coordinate range");
        for (int i = 0; i < numTerminals; ++i) {
            Terminal t;
            string tag;
            pda::read(input, t.name, tag, t.x, t.y);
            pda::input_require(tag == "terminal" &&
                                   problem.pin_ids.emplace(t.name, numBlocks + i).second,
                               "Invalid or duplicate terminal");
            problem.terminals.push_back(t);
        }
        pda::end(input);
    }
    {
        auto &input = nets;
        pda::expect(input, "NumNets:");
        pda::read(input, numNets);
        pda::input_require(numNets >= 0 && numNets < 500, "Invalid net count");
        problem.nets.resize(static_cast<std::size_t>(numNets));
        problem.net_ids.resize(static_cast<std::size_t>(numNets));
        for (auto &net : problem.nets) {
            int pins = 0;
            pda::expect(input, "NetDegree:");
            pda::read(input, pins);
            pda::input_require(pins > 0 && pins <= numBlocks + numTerminals, "Invalid net degree");
            for (int j = 0; j < pins; ++j) {
                string name;
                pda::read(input, name);
                pda::input_require(problem.pin_ids.count(name) != 0, "Unknown pin: " + name);
                net.push_back(name);
            }
        }
        pda::end(input);
        for (std::size_t i = 0; i < problem.nets.size(); ++i) {
            problem.net_ids[i].reserve(problem.nets[i].size());
            for (const auto &name : problem.nets[i])
                problem.net_ids[i].push_back(problem.pin_ids.at(name));
        }
    }
    return problem;
}
Problem parse(const string &block_path, const string &net_path) {
    auto blocks = pda::input_file(block_path);
    auto nets = pda::input_file(net_path);
    return parse(blocks, nets);
}
} // namespace lab2

#pragma once
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

namespace pda {
inline void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
inline std::ifstream input_file(const std::string &path) {
    std::ifstream input(path);
    require(input.is_open(), "Cannot open input: " + path);
    return input;
}
inline std::ofstream output_file(const std::string &path) {
    std::ofstream output(path);
    require(output.is_open(), "Cannot open output: " + path);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    return output;
}
template <class... Values> void read(std::istream &input, Values &...values) {
    require(static_cast<bool>((input >> ... >> values)), "Malformed or truncated input");
}
inline void expect(std::istream &input, const std::string &expected) {
    std::string token;
    read(input, token);
    require(token == expected, "Expected token: " + expected + ", got: " + token);
}
inline void end(std::istream &input) {
    std::string token;
    require(!(input >> token), "Unexpected trailing input: " + token);
}
inline bool positive(double value) { return std::isfinite(value) && value > 0; }
inline bool nonnegative(double value) { return std::isfinite(value) && value >= 0; }
} // namespace pda

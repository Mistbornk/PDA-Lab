#include "pda/io.hpp"
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#if PDA_FUZZ_LAB == 1
#include "layout.hpp"
#elif PDA_FUZZ_LAB == 2
#include "parser.hpp"
#elif PDA_FUZZ_LAB == 3
#include "legalizer.hpp"
#else
#include "input.hpp"
#endif
// Seeds use a separator between the independently parsed input files. Random
// mutations exercise both malformed syntax and successful structured parses.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *bytes, std::size_t size) {
    if (size > 16384)
        return 0;
    const std::string text(reinterpret_cast<const char *>(bytes), size);
    const std::string delimiter = "\n---\n";
    const auto first = text.find(delimiter);
    const auto second =
        first == std::string::npos ? first : text.find(delimiter, first + delimiter.size());
    std::istringstream a(text.substr(0, first));
    std::istringstream b(
        first == std::string::npos
            ? ""
            : text.substr(first + delimiter.size(), second == std::string::npos
                                                        ? second
                                                        : second - first - delimiter.size()));
    std::istringstream c(second == std::string::npos ? "" : text.substr(second + delimiter.size()));
    try {
#if PDA_FUZZ_LAB == 1
        (void)lab1::parse(a);
#elif PDA_FUZZ_LAB == 2
        (void)lab2::parse(a, b);
#elif PDA_FUZZ_LAB == 3
        (void)lab3::parse_placement(a);
        (void)lab3::parse_steps(b);
#else
        (void)lab4::parse(a, b, c, 4096);
#endif
    } catch (const pda::InputError &) {
        // Only expected input rejection is caught. Allocation errors, internal
        // failures, sanitizer reports and signals remain visible to the fuzzer.
    }
    return 0;
}

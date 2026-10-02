#pragma once
#include "struct.hpp"
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace lab3 {
struct Step {
    std::vector<std::string> remove;
    Cell cell{};
};
struct Input {
    double alpha{}, beta{};
    Die die{};
    std::vector<Cell> cells;
    std::vector<PlacementRow> rows;
};
Input parse_placement(std::istream &input);
std::vector<Step> parse_steps(std::istream &input);
struct Options {
    bool nearest = false;
    bool intervals = true;
    bool stats = false;
};
struct CellId {
    std::size_t value;
    bool operator==(CellId other) const { return value == other.value; }
};
struct Stats {
    std::size_t spatial_queries = 0, rows_tested = 0, committed_steps = 0;
};
struct StepResult {
    Cell placed{};
    std::vector<Cell> moved;
};
class NoLegalPlacement : public std::runtime_error {
  public:
    explicit NoLegalPlacement(const std::string &name)
        : std::runtime_error("Could not place cell: " + name) {}
};
// Input follows parse_placement's contract. Expected failures leave the placement
// unchanged. After an allocation/internal error during commit, discard the session.
class Legalizer {
    struct Impl;
    std::unique_ptr<Impl> impl;

  public:
    explicit Legalizer(Input input, Options options);
    ~Legalizer();
    Legalizer(Legalizer &&) noexcept;
    Legalizer &operator=(Legalizer &&) noexcept;
    StepResult apply(const Step &step);
    const Stats &stats() const;
    std::vector<Cell> cells() const;
    void validate() const;
};
void write_step(std::ostream &output, const StepResult &result);
void write_stats(std::ostream &output, const Stats &stats);
void legalize(Input input, const std::vector<Step> &steps, std::ostream &output, Options options);
} // namespace lab3

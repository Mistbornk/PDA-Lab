#pragma once
#include "model.hpp"
namespace lab2 {
// Transaction over structural state only. Derived x/y coordinates must be repacked
// after rollback. Block names never change and are never copied into the journal.
class UndoJournal {
    struct Entry {
        std::size_t id;
        Node node;
        int width, height;
        bool rotate;
    };
    std::vector<std::uint64_t> stamps;
    std::vector<Entry> entries;
    std::uint64_t epoch = 0;
    int root = -1;

  public:
    void begin(const Placement &placement);
    void remember(const Placement &placement, int id);
    void neighborhood(const Placement &placement, int id);
    void rollback(Placement &placement) const;
};
} // namespace lab2

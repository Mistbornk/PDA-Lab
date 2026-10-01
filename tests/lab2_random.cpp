#include "annealer.hpp"
#include "random.hpp"
#include <cstdlib>
#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
bool equal(const lab2::Result &a, const lab2::Result &b) {
    if (a.cost.cost != b.cost.cost || a.iterations != b.iterations ||
        a.blocks.size() != b.blocks.size())
        return false;
    for (std::size_t i = 0; i < a.blocks.size(); ++i) {
        const auto &x = a.blocks[i];
        const auto &y = b.blocks[i];
        if (x.x != y.x || x.y != y.y || x.width != y.width || x.height != y.height)
            return false;
    }
    return true;
}
int main() {
    try {
#ifdef __GLIBC__
        // libc is a test-only external sequence oracle, never used by the solver.
        for (unsigned seed : {0U, 1U, 7U, 19U, 2147483647U, 2147483648U, 4294967295U}) {
            std::srand(seed);
            lab2::Random local(seed);
            for (int i = 0; i < 100000; ++i)
                check(local() == std::rand(), "Local generator changed legacy sequence");
        }
#endif
        lab2::Problem problem;
        problem.outline_width = 100;
        problem.outline_height = 100;
        for (int i = 0; i < 12; ++i)
            problem.blocks.push_back({std::to_string(i), 0, 0, 2 + i % 4, 3 + i % 7, false});
        std::vector<lab2::Result> reference;
        for (unsigned seed = 1; seed <= 8; ++seed) {
            lab2::Options options;
            options.seed = seed;
            options.iterations = 3000;
            reference.push_back(lab2::solve(problem, options));
        }
        std::vector<std::future<lab2::Result>> futures;
        for (unsigned seed = 1; seed <= 8; ++seed)
            futures.push_back(std::async(std::launch::async, [&, seed] {
                lab2::Options options;
                options.seed = seed;
                options.iterations = 3000;
                return lab2::solve(problem, options);
            }));
        for (std::size_t i = 0; i < futures.size(); ++i)
            check(equal(reference[i], futures[i].get()),
                  "Concurrent solver differs from serial result");
        std::cout << "700,000 libc sequence checks and 8 concurrent searches passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

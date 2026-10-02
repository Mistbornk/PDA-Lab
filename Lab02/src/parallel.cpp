#include "parallel.hpp"
#include "pda/io.hpp"
#include <algorithm>
#include <atomic>
#include <exception>
#include <future>
#include <limits>
#include <optional>
namespace lab2 {
MultiResult solve_multi(const Problem &problem, const Options &options, const MultiOptions &multi) {
    pda::require(multi.restarts >= 1 && multi.restarts <= 1024 && multi.threads >= 1 &&
                     multi.threads <= 1024,
                 "Restarts and threads must be in [1,1024]");
    pda::require(static_cast<std::uint64_t>(options.seed) + multi.restarts - 1 <=
                     std::numeric_limits<unsigned>::max(),
                 "Restart seed range overflows");
    MultiResult result;
    result.threads = std::min(multi.threads, multi.restarts);
    result.attempts.resize(multi.restarts);
    std::vector<std::optional<Result>> solutions(multi.restarts);
    std::vector<std::exception_ptr> errors(multi.restarts);
    std::atomic<unsigned> next{0};
    const auto worker = [&] {
        while (true) {
            const auto id = next.fetch_add(1, std::memory_order_relaxed);
            if (id >= multi.restarts)
                return;
            auto settings = options;
            settings.seed += id;
            auto &attempt = result.attempts[id];
            attempt.seed = settings.seed;
            const double began = search_seconds();
            try {
                solutions[id] = solve(problem, settings, began);
                attempt.legal = true;
                attempt.iterations = solutions[id]->iterations;
                attempt.cost = solutions[id]->cost;
                attempt.stats = solutions[id]->stats;
            } catch (const NoLegalPlacement &failure) {
                attempt.iterations = failure.iterations;
                attempt.stats = failure.stats;
            } catch (...) {
                errors[id] = std::current_exception();
            }
            attempt.cpu_seconds = search_seconds() - began;
        }
    };
    // Future destruction joins launched tasks, even if launching a later worker
    // throws. Only task claiming is shared mutable state; result slots are disjoint.
    std::vector<std::future<void>> workers;
    workers.reserve(result.threads);
    if (result.threads == 1)
        worker();
    else {
        for (unsigned i = 0; i < result.threads; ++i)
            workers.push_back(std::async(std::launch::async, worker));
        for (auto &thread : workers)
            thread.get();
    }
    std::optional<std::size_t> best;
    for (std::size_t i = 0; i < result.attempts.size(); ++i) {
        if (errors[i])
            std::rethrow_exception(errors[i]);
        const auto &attempt = result.attempts[i];
        result.total_iterations += attempt.iterations;
        result.cpu_seconds += attempt.cpu_seconds;
        if (attempt.legal && (!best || attempt.cost.cost < result.attempts[*best].cost.cost))
            best = i;
    }
    if (!best)
        throw NoLegalPlacement(result.total_iterations);
    result.winning_seed = result.attempts[*best].seed;
    result.best = std::move(*solutions[*best]);
    return result;
}
} // namespace lab2

#include "loomlog/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <iostream>
#include <vector>

long long benchmark_thread_pool(
    std::size_t task_count,
    std::size_t thread_count,
    std::size_t& completed_count
) {
    loom::ThreadPool pool(thread_count);
    std::atomic<std::size_t> completed{0};
    std::vector<std::future<void>> futures;
    futures.reserve(task_count);

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < task_count; ++i) {
        futures.push_back(pool.submit([&completed] {
            completed.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& future : futures) {
        future.get();
    }

    pool.shutdown();

    const auto end = std::chrono::steady_clock::now();
    completed_count = completed.load();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
}

long long benchmark_async(
    std::size_t task_count,
    std::size_t& completed_count
) {
    std::atomic<std::size_t> completed{0};
    std::vector<std::future<void>> futures;
    futures.reserve(task_count);

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < task_count; ++i) {
        futures.push_back(std::async(
            std::launch::async,
            [&completed] {
                completed.fetch_add(1, std::memory_order_relaxed);
            }
        ));
    }

    for (auto& future : futures) {
        future.get();
    }

    const auto end = std::chrono::steady_clock::now();
    completed_count = completed.load();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
}

int main() {
    constexpr std::size_t task_count = 100'000;
    constexpr std::size_t thread_count = 4;

    std::size_t pool_completed = 0;
    const auto pool_elapsed = benchmark_thread_pool(
        task_count,
        thread_count,
        pool_completed
    );

    std::size_t async_completed = 0;
    const auto async_elapsed = benchmark_async(
        task_count,
        async_completed
    );

    std::cout << "thread_pool\n";
    std::cout << "  thread_count: " << thread_count << '\n';
    std::cout << "  task_count: " << task_count << '\n';
    std::cout << "  completed: " << pool_completed << '\n';
    std::cout << "  elapsed_ms: " << pool_elapsed << '\n';

    std::cout << "std::async\n";
    std::cout << "  task_count: " << task_count << '\n';
    std::cout << "  completed: " << async_completed << '\n';
    std::cout << "  elapsed_ms: " << async_elapsed << '\n';

    return pool_completed == task_count && async_completed == task_count
        ? 0
        : 1;
}

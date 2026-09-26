#include "loomlog/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>
#include <vector>

TEST(ThreadPoolTest, SubmitReturnsValue) {
    loom::ThreadPool pool(2);

    auto future = pool.submit([] {
        return 41 + 1;
    });

    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTest, ManyTasksComplete) {
    loom::ThreadPool pool(4);
    std::atomic<int> completed{0};
    std::vector<std::future<void>> futures;
    futures.reserve(1000);

    for (int i = 0; i < 1000; ++i) {
        futures.push_back(pool.submit([&completed] {
            completed.fetch_add(1);
        }));
    }

    for (auto& future : futures) {
        future.get();
    }

    pool.shutdown();

    EXPECT_EQ(completed.load(), 1000);
}

TEST(ThreadPoolTest, ShutdownIsIdempotent) {
    loom::ThreadPool pool(2);

    pool.shutdown();
    pool.shutdown();
    pool.shutdown();
}

TEST(ThreadPoolTest, SubmitAfterShutdownThrows) {
    loom::ThreadPool pool(2);
    pool.shutdown();

    EXPECT_THROW(pool.submit([] {}), std::runtime_error);
}

TEST(ThreadPoolTest, DestructorJoins) {
    std::atomic<bool> completed{false};

    {
        loom::ThreadPool pool(2);
        pool.submit([&completed] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            completed.store(true);
        });
    }

    EXPECT_TRUE(completed.load());
}

TEST(ThreadPoolTest, ExceptionPropagatesToFuture) {
    loom::ThreadPool pool(2);

    auto future = pool.submit([] {
        throw std::runtime_error("task failed");
    });

    EXPECT_THROW(future.get(), std::runtime_error);
}

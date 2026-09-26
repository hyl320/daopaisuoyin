#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace loom {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t nthreads = 0);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool();

    void shutdown();

    template <class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    std::size_t nthreads_{0};
};

}  // namespace loom

template <class F, class... Args>
auto loom::ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
{
    using R = std::invoke_result_t<
        std::decay_t<F>,
        std::decay_t<Args>...
    >;

    auto task = std::make_shared<std::packaged_task<R()>>(
        [fn = std::decay_t<F>(std::forward<F>(f)),
         tup = std::make_tuple(
             std::decay_t<Args>(std::forward<Args>(args))...
         )]() mutable -> R {
            return std::apply(std::move(fn), std::move(tup));
        }
    );

    std::future<R> future = task->get_future();

    {
        std::lock_guard<std::mutex> lock(mu_);

        if (stop_.load()) {
            throw std::runtime_error("ThreadPool is shutdown");
        }

        tasks_.emplace([task] {
            (*task)();
        });
    }

    cv_.notify_one();
    return future;
}

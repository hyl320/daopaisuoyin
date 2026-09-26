#include "loomlog/thread_pool.hpp"

#include <algorithm>

namespace loom {

ThreadPool::ThreadPool(std::size_t nthreads) {
    if (nthreads == 0) {
        nthreads_ = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    } else {
        nthreads_ = nthreads;
    }

    workers_.reserve(nthreads_);
    for (std::size_t i = 0; i < nthreads_; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mu_);
            cv_.wait(lock, [this] {
                return stop_.load() || !tasks_.empty();
            });

            if (stop_.load() && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mu_);
        stop_.store(true);
    }

    cv_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
}

}  // namespace loom

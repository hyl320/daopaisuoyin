#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace loom {

enum class Level {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
};

struct LoggerConfig {
    std::string path = "logs/app.log";
    Level min_level = Level::Info;
    std::size_t queue_limit = 8192;
    std::size_t batch_size = 256;
    std::chrono::milliseconds flush_interval{16};
    std::size_t rotate_bytes = 32 * 1024 * 1024;
    std::size_t rotate_keep = 3;
};

class Logger {
public:
    explicit Logger(LoggerConfig config = {});

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    ~Logger();

    void log(Level level, std::string_view message);

    template <class... Args>
    void error(std::format_string<Args...> fmt, Args&&... args);

    template <class... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args);

    template <class... Args>
    void info(std::format_string<Args...> fmt, Args&&... args);

    template <class... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args);

    void shutdown();

    std::uint64_t dropped() const noexcept;

private:
    void worker_loop();
    void flush_batch(std::deque<std::string>& batch);
    void rotate_if_needed();

    LoggerConfig config_;
    std::deque<std::string> queue_;
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::thread worker_;
    std::ofstream file_;
    std::atomic<bool> stopping_{false};
    std::atomic<std::uint64_t> dropped_{0};
    std::size_t current_size_{0};
};

}  // namespace loom

template <class... Args>
void loom::Logger::error(std::format_string<Args...> fmt, Args&&... args) {
    log(Level::Error, std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void loom::Logger::warn(std::format_string<Args...> fmt, Args&&... args) {
    log(Level::Warn, std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void loom::Logger::info(std::format_string<Args...> fmt, Args&&... args) {
    log(Level::Info, std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void loom::Logger::debug(std::format_string<Args...> fmt, Args&&... args) {
    log(Level::Debug, std::format(fmt, std::forward<Args>(args)...));
}

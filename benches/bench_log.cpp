#include "loomlog/logger.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>

int main() {
    const auto dir = std::filesystem::temp_directory_path() / "loomlog_bench";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    loom::LoggerConfig config;
    config.path = (dir / "bench.log").string();
    config.min_level = loom::Level::Info;
    config.queue_limit = 8192;
    config.batch_size = 256;
    config.flush_interval = std::chrono::milliseconds(16);
    config.rotate_bytes = 64 * 1024 * 1024;

    constexpr int n = 1'000'000;

    loom::Logger logger(config);

    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < n; ++i) {
        logger.info("hello {}", i);
    }

    const auto submit_done = std::chrono::steady_clock::now();

    logger.shutdown();

    const auto all_done = std::chrono::steady_clock::now();

    const auto submit_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(submit_done - start).count();

    const auto total_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(all_done - start).count();

    std::cout << "messages: " << n << '\n';
    std::cout << "submit ms: " << submit_ms << '\n';
    std::cout << "total ms: " << total_ms << '\n';
    std::cout << "dropped: " << logger.dropped() << '\n';
    std::cout << "log path: " << config.path << '\n';
}
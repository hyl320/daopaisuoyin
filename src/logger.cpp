#include "loomlog/logger.hpp"

#include <filesystem>
#include <string>

namespace loom {

namespace {

std::filesystem::path rotated_path(const std::filesystem::path& path, std::size_t index) {
    return std::filesystem::path(path.string() + "." + std::to_string(index));
}

bool should_wait_for_space(Level level) {
    return level == Level::Error;
}

}  // namespace

Logger::Logger(LoggerConfig config)
    : config_(std::move(config)){
        worker_ = std::thread(&Logger::worker_loop, this);}

Logger::~Logger() {
    shutdown();
}

void Logger::log(Level level, std::string_view message) {
    if (static_cast<int>(level) > static_cast<int>(config_.min_level)) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(mu_);

        if (stopping_.load()) {
            dropped_.fetch_add(1);
            return;
        }

        if (queue_.size() >= config_.queue_limit) {
            if (!should_wait_for_space(level)) {
                dropped_.fetch_add(1);
                return;
            }

            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
            cv_.wait_until(lock, deadline, [this] {
                return stopping_.load() || queue_.size() < config_.queue_limit;
            });

            if (stopping_.load() || queue_.size() >= config_.queue_limit) {
                dropped_.fetch_add(1);
                return;
            }
        }

        queue_.emplace_back(message);
    }

    cv_.notify_one();
}

void Logger::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mu_);
        stopping_.store(true);
    }

    cv_.notify_all();

    if (worker_.joinable()) {
        worker_.join();
    }

    if (file_.is_open()) {
        file_.close();
    }
}

std::uint64_t Logger::dropped() const noexcept {
    return dropped_.load();
}

void Logger::worker_loop() {
    while (true) {
        std::deque<std::string> batch;

        {
            std::unique_lock<std::mutex> lock(mu_);
            cv_.wait(lock, [this] {
                return stopping_.load() || !queue_.empty();
            });

            if (stopping_.load() && queue_.empty()) {
                break;
            }

            const auto deadline = std::chrono::steady_clock::now() + config_.flush_interval;

            if (!stopping_.load() && queue_.size() < config_.batch_size) {
                cv_.wait_until(lock, deadline, [this] {
                    return stopping_.load() || queue_.size() >= config_.batch_size;
                });
            }
            while (!queue_.empty() && batch.size() < config_.batch_size) {
                batch.push_back(std::move(queue_.front()));
                queue_.pop_front();
            }
        }

        cv_.notify_all();
        flush_batch(batch);
    }
}

void Logger::flush_batch(std::deque<std::string>& batch) {
    if (batch.empty()) {
        return;
    }

    if (!file_.is_open()) {
        const std::filesystem::path path(config_.path);
        const auto parent = path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        file_.open(config_.path, std::ios::app);
        std::error_code ec;
        current_size_ = std::filesystem::exists(path, ec)
            ? static_cast<std::size_t>(std::filesystem::file_size(path, ec))
            : 0;
    }

    for (const auto& message : batch) {
        file_ << message << '\n';
        current_size_ += message.size() + 1;
        rotate_if_needed();
    }

    file_.flush();
}

void Logger::rotate_if_needed() {
    if (config_.rotate_bytes == 0 || current_size_ < config_.rotate_bytes) {
        return;
    }

    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }

    const std::filesystem::path path(config_.path);
    std::error_code ec;

    if (config_.rotate_keep == 0) {
        std::filesystem::remove(path, ec);
    } else {
        std::filesystem::remove(rotated_path(path, config_.rotate_keep), ec);

        for (std::size_t index = config_.rotate_keep; index > 1; --index) {
            const auto from = rotated_path(path, index - 1);
            const auto to = rotated_path(path, index);

            if (std::filesystem::exists(from, ec)) {
                ec.clear();
                std::filesystem::rename(from, to, ec);
            }
        }

        if (std::filesystem::exists(path, ec)) {
            ec.clear();
            std::filesystem::rename(path, rotated_path(path, 1), ec);
        }
    }

    file_.open(config_.path, std::ios::trunc);
    current_size_ = 0;
}

}  // namespace loom

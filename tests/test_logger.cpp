#include "loomlog/logger.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::filesystem::path clean_test_dir(std::string_view name) {
    const auto dir = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

}  // namespace

TEST(LoggerTest, LevelFiltering) {
    const auto dir = clean_test_dir("loomlog_logger_test_level");
    const auto path = dir / "app.log";

    loom::LoggerConfig config;
    config.path = path.string();
    config.min_level = loom::Level::Info;
    config.batch_size = 1;
    config.flush_interval = std::chrono::milliseconds(1);

    loom::Logger logger(config);
    logger.debug("hidden");
    logger.info("visible");
    logger.shutdown();

    const auto content = read_file(path);
    EXPECT_EQ(content.find("hidden"), std::string::npos);
    EXPECT_NE(content.find("visible"), std::string::npos);
}

TEST(LoggerTest, FormatsMessageContent) {
    const auto dir = clean_test_dir("loomlog_logger_test_format");
    const auto path = dir / "app.log";

    loom::LoggerConfig config;
    config.path = path.string();
    config.batch_size = 1;
    config.flush_interval = std::chrono::milliseconds(1);

    loom::Logger logger(config);
    logger.info("hello {}", 42);
    logger.shutdown();

    const auto content = read_file(path);
    EXPECT_NE(content.find("hello 42"), std::string::npos);
}

TEST(LoggerTest, DestructorFlushesQueuedMessages) {
    const auto dir = clean_test_dir("loomlog_logger_test_destructor");
    const auto path = dir / "app.log";

    loom::LoggerConfig config;
    config.path = path.string();
    config.batch_size = 64;
    config.flush_interval = std::chrono::milliseconds(100);

    {
        loom::Logger logger(config);
        logger.info("last message");
    }

    const auto content = read_file(path);
    EXPECT_NE(content.find("last message"), std::string::npos);
}

TEST(LoggerTest, RotatesFileWhenSizeLimitIsReached) {
    const auto dir = clean_test_dir("loomlog_logger_test_rotate");
    const auto path = dir / "app.log";

    loom::LoggerConfig config;
    config.path = path.string();
    config.batch_size = 1;
    config.flush_interval = std::chrono::milliseconds(1);
    config.rotate_bytes = 64;
    config.rotate_keep = 2;

    loom::Logger logger(config);
    for (int i = 0; i < 10; ++i) {
        logger.info("rotating message {}", i);
    }
    logger.shutdown();

    const auto rotated = std::filesystem::path(path.string() + ".1");
    EXPECT_TRUE(std::filesystem::exists(rotated));
    EXPECT_NE(read_file(rotated).find("rotating message"), std::string::npos);
}

TEST(LoggerTest, CountsDroppedMessagesWhenQueueLimitIsZero) {
    const auto dir = clean_test_dir("loomlog_logger_test_dropped");
    const auto path = dir / "app.log";

    loom::LoggerConfig config;
    config.path = path.string();
    config.min_level = loom::Level::Info;
    config.queue_limit = 0;

    loom::Logger logger(config);
    logger.debug("filtered");
    logger.info("dropped");
    logger.shutdown();

    EXPECT_EQ(logger.dropped(), 1);
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(LoggerTest, CountsDroppedErrorWhenNoQueueCapacityExists) {
    const auto dir = clean_test_dir("loomlog_logger_test_dropped_error");
    const auto path = dir / "app.log";

    loom::LoggerConfig config;
    config.path = path.string();
    config.min_level = loom::Level::Debug;
    config.queue_limit = 0;

    loom::Logger logger(config);
    logger.error("important but impossible to enqueue");
    logger.shutdown();

    EXPECT_EQ(logger.dropped(), 1);
    EXPECT_FALSE(std::filesystem::exists(path));
}

#include "loomlog/index_store.hpp"
#include "loomlog/indexer.hpp"
#include "loomlog/logger.hpp"
#include "loomlog/query_engine.hpp"
#include "loomlog/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <future>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::size_t parse_size(std::string_view value, std::string_view option) {
    try {
        std::size_t parsed = 0;
        const auto number = std::stoull(std::string(value), &parsed);
        if (parsed != value.size() || number == 0) {
            throw std::invalid_argument("invalid value");
        }
        return static_cast<std::size_t>(number);
    } catch (const std::exception&) {
        throw std::invalid_argument(
            "invalid value for " + std::string(option) + ": " + std::string(value)
        );
    }
}

int run_bench_pool(int argc, char* argv[]) {
    std::size_t thread_count = 4;
    std::size_t task_count = 100'000;

    for (int i = 2; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if ((option == "--threads" || option == "--jobs") && i + 1 < argc) {
            const auto value = parse_size(argv[++i], option);
            if (option == "--threads") {
                thread_count = value;
            } else {
                task_count = value;
            }
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + std::string(option));
        }
    }

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

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();

    std::cout << "command: bench-pool\n";
    std::cout << "threads: " << thread_count << '\n';
    std::cout << "jobs: " << task_count << '\n';
    std::cout << "completed: " << completed.load() << '\n';
    std::cout << "elapsed_ms: " << elapsed << '\n';

    return completed.load() == task_count ? 0 : 1;
}

int run_bench_log(int argc, char* argv[]) {
    std::size_t message_count = 100'000;
    std::string path =
        (std::filesystem::temp_directory_path() / "loomlog_cli_bench.log").string();

    for (int i = 2; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if ((option == "--messages" || option == "--file") && i + 1 < argc) {
            const std::string value(argv[++i]);
            if (option == "--messages") {
                message_count = parse_size(value, option);
            } else {
                path = value;
            }
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + std::string(option));
        }
    }

    std::filesystem::remove(path);

    loom::LoggerConfig config;
    config.path = path;
    config.queue_limit = message_count + 1;
    config.batch_size = 256;
    config.rotate_bytes = 0;

    loom::Logger logger(config);
    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < message_count; ++i) {
        logger.info("INFO message {}", i);
    }

    const auto submit_done = std::chrono::steady_clock::now();
    logger.shutdown();
    const auto finished = std::chrono::steady_clock::now();

    const auto submit_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        submit_done - start
    ).count();
    const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        finished - start
    ).count();

    std::cout << "command: bench-log\n";
    std::cout << "messages: " << message_count << '\n';
    std::cout << "submit_ms: " << submit_ms << '\n';
    std::cout << "total_ms: " << total_ms << '\n';
    std::cout << "dropped: " << logger.dropped() << '\n';
    std::cout << "file: " << path << '\n';
    return 0;
}

int run_grep(int argc, char* argv[]) {
    std::string path;
    std::string keyword;
    std::string level;

    for (int i = 2; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if ((option == "--file" || option == "--keyword" || option == "--level")
            && i + 1 < argc) {
            const std::string value(argv[++i]);
            if (option == "--file") {
                path = value;
            } else if (option == "--keyword") {
                keyword = value;
            } else {
                level = value;
            }
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + std::string(option));
        }
    }

    if (path.empty()) {
        throw std::invalid_argument("grep requires --file");
    }

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("cannot open file: " + path);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!keyword.empty() && line.find(keyword) == std::string::npos) {
            continue;
        }
        if (!level.empty() && line.find(level) == std::string::npos) {
            continue;
        }
        std::cout << line << '\n';
    }

    return 0;
}

int run_index(int argc, char* argv[]) {
    //有没有漏参数
    if (argc < 3) {
        throw std::invalid_argument("index requires a directory");
    }
    //是否是已近存在的目录
    const std::filesystem::path root(argv[2]);
    if (!std::filesystem::is_directory(root)) {
        throw std::invalid_argument("index requires an existing directory: " + root.string());
    }
    
    //默认配置是否是线程数和输出文件
    std::size_t thread_count = 0;
    std::filesystem::path output = "index.bin";
    
    //如果当前参数是 --threads 或 --output，并且后面还有值，就读取下一个参数作为它的值。
    for (int i = 3; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if ((option == "--threads" || option == "--output") && i + 1 < argc) {
            const std::string value(argv[++i]);
            if (option == "--threads") {
                thread_count = parse_size(value, option);
            } else {
                output = value;
            }
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + std::string(option));
        }
    }

    loom::LoggerConfig logger_config;
    logger_config.path = "loomsearch.log";
    loom::Logger logger(logger_config);

    loom::IndexBuildOptions options;
    options.root = root;
    options.thread_count = thread_count;
    options.logger = &logger;

    // CLI 是索引构建的调度入口：并发处理由 Indexer 内部交给 ThreadPool。
    // 1. 并发扫描 + 构建内存索引
    const auto result = loom::Indexer{}.build(options);
    //2. 把索引落盘
    loom::IndexStore{}.save(result.index, output);
    // 3. 优雅关闭日志线程
    logger.shutdown();

    std::cout << "command: index\n";
    std::cout << "files_total: " << result.stats.files_total << '\n';
    std::cout << "files_success: " << result.stats.files_success << '\n';
    std::cout << "files_failed: " << result.stats.files_failed << '\n';
    std::cout << "bytes_total: " << result.stats.bytes_total << '\n';
    std::cout << "keywords_total: " << result.stats.keywords_total << '\n';
    std::cout << "elapsed_ms: " << result.stats.elapsed_ms << '\n';
    std::cout << "output: " << output.lexically_normal().string() << '\n';

    return result.stats.files_failed == 0 ? 0 : 1;
}

int run_query(int argc, char* argv[]) {
    if (argc < 4) {
        throw std::invalid_argument("query requires an index file and keywords");
    }

    const std::filesystem::path index_path(argv[2]);
    std::ostringstream query_text;
    for (int i = 3; i < argc; ++i) {
        if (i > 3) {
            query_text << ' ';
        }
        query_text << argv[i];
    }

    const auto index = loom::IndexStore{}.load(index_path);
    const auto matches = loom::QueryEngine{}.query(index, query_text.str());

    // 输出保持简单稳定，便于脚本消费和测试断言。
    std::cout << "matches: " << matches.size() << '\n';
    for (const auto& match : matches) {
        std::cout << match << '\n';
    }

    return 0;
}

void print_usage() {
    std::cout << "usage:\n";
    std::cout << "  loomsearch index <directory> [--threads N] [--output PATH]\n";
    std::cout << "  loomsearch query <index-file> <keywords...>\n";
    std::cout << "  loomsearch bench-pool [--threads N] [--jobs N]\n";
    std::cout << "  loomsearch bench-log [--messages N] [--file PATH]\n";
    std::cout << "  loomsearch grep --file PATH [--keyword TEXT] [--level LEVEL]\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 2 || std::string_view(argv[1]) == "--help") {
            print_usage();
            return argc < 2 ? 1 : 0;
        }

        if (std::string_view(argv[1]) == "bench-pool") {
            return run_bench_pool(argc, argv);
        }
        if (std::string_view(argv[1]) == "bench-log") {
            return run_bench_log(argc, argv);
        }
        if (std::string_view(argv[1]) == "grep") {
            return run_grep(argc, argv);
        }
        if (std::string_view(argv[1]) == "index") {
            return run_index(argc, argv);
        }
        if (std::string_view(argv[1]) == "query") {
            return run_query(argc, argv);
        }

        throw std::invalid_argument("unknown command: " + std::string(argv[1]));
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        print_usage();
        return 2;
    }
}

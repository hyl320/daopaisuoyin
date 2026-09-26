#pragma once

#include "loomlog/inverted_index.hpp"
#include "loomlog/logger.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace loom {

struct IndexBuildOptions {
    std::filesystem::path root;
    std::size_t thread_count{0};
    Logger* logger{nullptr};
};

struct IndexBuildStats {
    std::size_t files_total{0};
    std::size_t files_success{0};
    std::size_t files_failed{0};
    std::size_t bytes_total{0};
    std::size_t keywords_total{0};
    std::size_t elapsed_ms{0};
};

struct IndexBuildResult {
    InvertedIndex index;
    IndexBuildStats stats;
};

class Indexer {
public:
    IndexBuildResult build(const IndexBuildOptions& options) const;

    IndexBuildResult build_files(
        std::vector<std::filesystem::path> files,
        std::size_t thread_count = 0,
        Logger* logger = nullptr
    ) const;
};

}  // namespace loom

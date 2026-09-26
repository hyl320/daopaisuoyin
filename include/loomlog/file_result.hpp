#pragma once

#include <cstddef>
#include <string>
#include <unordered_set>

namespace loom {

struct FileResult {
    std::string path;
    std::unordered_set<std::string> words;
    std::size_t bytes{0};
    bool success{false};
    std::string error;
};

}  // namespace loom

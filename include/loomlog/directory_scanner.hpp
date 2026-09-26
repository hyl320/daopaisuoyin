#pragma once

#include <filesystem>
#include <vector>

namespace loom {

class DirectoryScanner {
public:
    std::vector<std::filesystem::path> scan(const std::filesystem::path& root) const;
};

}  // namespace loom

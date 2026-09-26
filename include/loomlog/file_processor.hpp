#pragma once

#include "loomlog/file_result.hpp"

#include <filesystem>

namespace loom {

class FileProcessor {
public:
    FileResult process(const std::filesystem::path& path) const;
};

}  // namespace loom

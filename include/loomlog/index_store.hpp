#pragma once

#include "loomlog/inverted_index.hpp"

#include <filesystem>

namespace loom {

class IndexStore {
public:
    void save(const InvertedIndex& index, const std::filesystem::path& path) const;
    InvertedIndex load(const std::filesystem::path& path) const;
};

}  // namespace loom

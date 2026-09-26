#pragma once

#include "loomlog/inverted_index.hpp"

#include <string_view>
#include <vector>

namespace loom {

class QueryEngine {
public:
    std::vector<std::string> query(const InvertedIndex& index, std::string_view text) const;
};

}  // namespace loom

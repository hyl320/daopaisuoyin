#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace loom {

std::unordered_set<std::string> tokenize_words(std::string_view text);

std::vector<std::string> normalize_query_terms(std::string_view text);

}  // namespace loom

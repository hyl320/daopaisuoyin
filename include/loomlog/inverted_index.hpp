#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace loom {

using IndexSnapshot = std::vector<std::pair<std::string, std::vector<std::string>>>;

class InvertedIndex {
public:
    void add_document(const std::string& path, const std::unordered_set<std::string>& words);

    std::vector<std::string> find(const std::string& word) const;
    IndexSnapshot snapshot() const;

    std::size_t keyword_count() const noexcept;
    std::size_t document_count() const noexcept;

private:
    std::unordered_map<std::string, std::vector<std::string>> index_;
    std::unordered_set<std::string> documents_;
};

}  // namespace loom

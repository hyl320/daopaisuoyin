#include "loomlog/inverted_index.hpp"

#include <algorithm>

namespace loom {

void InvertedIndex::add_document(
    const std::string& path,
    const std::unordered_set<std::string>& words
) {
    if (path.empty()) {
        return;
    }

    documents_.insert(path);

    // v1 的倒排索引只在主调度线程合并，类内部不加锁，边界更清楚。
    for (const auto& word : words) {
        if (word.empty()) {
            continue;
        }

        auto& docs = index_[word];
        //这会带来额外开销。对于出现频率很高的词，随着文档越来越多，反复排序整个列表可能达到接近：
        if (std::ranges::find(docs, path) == docs.end()) {
            docs.push_back(path);
            std::ranges::sort(docs);
        }
    }
}

std::vector<std::string> InvertedIndex::find(const std::string& word) const {
    const auto it = index_.find(word);
    if (it == index_.end()) {
        return {};
    }
    return it->second;
}

IndexSnapshot InvertedIndex::snapshot() const {
    IndexSnapshot result;
    result.reserve(index_.size());

    for (const auto& [word, docs] : index_) {
        auto sorted_docs = docs;
        std::ranges::sort(sorted_docs);
        result.emplace_back(word, std::move(sorted_docs));
    }

    std::ranges::sort(result, [](const auto& left, const auto& right) {
        return left.first < right.first;
    });

    return result;
}

std::size_t InvertedIndex::keyword_count() const noexcept {
    return index_.size();
}

std::size_t InvertedIndex::document_count() const noexcept {
    return documents_.size();
}

}  // namespace loom

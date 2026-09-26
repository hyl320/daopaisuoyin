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

    for (const auto& word : words) {
        if (word.empty()) {
            continue;
        }

        auto& posting = index_[word];
        // O(1) 去重；首次出现才追加，构建期不再排序
        if (posting.seen.insert(path).second) {
            posting.docs.push_back(path);
        }
    }
}

std::vector<std::string> InvertedIndex::find(const std::string& word) const {
    const auto it = index_.find(word);
    if (it == index_.end()) {
        return {};
    }

    auto docs = it->second.docs;   // 按值拷贝，原来就会拷贝
    std::ranges::sort(docs);       // 查询时才排序，且只排被查的词
    return docs;
}

IndexSnapshot InvertedIndex::snapshot() const {
    IndexSnapshot result;
    result.reserve(index_.size());

    for (const auto& [word, posting] : index_) {
        auto docs = posting.docs;   // 拷贝一份
        std::ranges::sort(docs);    // 输出时排序（原来也是这样做的）
        result.emplace_back(word, std::move(docs));
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

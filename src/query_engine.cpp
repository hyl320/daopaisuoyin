#include "loomlog/query_engine.hpp"

#include "loomlog/text_analyzer.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

namespace loom {

std::vector<std::string> QueryEngine::query(
    const InvertedIndex& index,
    std::string_view text
) const {
    const auto terms = normalize_query_terms(text);
    if (terms.empty()) {
        return {};
    }

    auto matches = index.find(terms.front());

    // 多关键词采用 AND 语义：每追加一个词，就和当前命中文档列表求交集。
    for (std::size_t i = 1; i < terms.size(); ++i) {
        const auto docs = index.find(terms[i]);
        std::vector<std::string> intersection;
        std::set_intersection(
            matches.begin(),
            matches.end(),
            docs.begin(),
            docs.end(),
            std::back_inserter(intersection)
        );
        matches = std::move(intersection);

        if (matches.empty()) {
            break;
        }
    }

    return matches;
}

}  // namespace loom

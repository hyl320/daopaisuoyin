#include "loomlog/text_analyzer.hpp"

#include <gtest/gtest.h>
#include <string>
#include <unordered_set>
#include <vector>

TEST(TextAnalyzerTest, TokenizesAsciiWordsAsLowercaseUniqueTerms) {
    const auto words = loom::tokenize_words("Thread_pool THREAD logger.md 42");
    const std::unordered_set<std::string> expected{
        "thread",
        "pool",
        "logger",
        "md",
        "42",
    };

    EXPECT_EQ(words, expected);
}

TEST(TextAnalyzerTest, NormalizesQueryTermsInInputOrderWithoutDuplicates) {
    const auto terms = loom::normalize_query_terms("Thread pool THREAD");
    const std::vector<std::string> expected{"thread", "pool"};

    EXPECT_EQ(terms, expected);
}

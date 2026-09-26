#include "loomlog/inverted_index.hpp"

#include <gtest/gtest.h>
#include <string>
#include <unordered_set>
#include <vector>

TEST(InvertedIndexTest, AddsDocumentsForEachWord) {
    loom::InvertedIndex index;

    index.add_document("a.md", {"thread", "pool"});
    index.add_document("b.md", {"pool", "logger"});

    EXPECT_EQ(index.find("thread"), std::vector<std::string>{"a.md"});
    EXPECT_EQ(index.find("logger"), std::vector<std::string>{"b.md"});
    EXPECT_EQ(index.find("pool"), (std::vector<std::string>{"a.md", "b.md"}));
}

TEST(InvertedIndexTest, DoesNotDuplicateSameDocumentForSameWord) {
    loom::InvertedIndex index;

    index.add_document("a.md", {"thread"});
    index.add_document("a.md", {"thread"});

    EXPECT_EQ(index.find("thread"), std::vector<std::string>{"a.md"});
    EXPECT_EQ(index.document_count(), 1u);
}

TEST(InvertedIndexTest, SnapshotIsSortedByKeywordAndPath) {
    loom::InvertedIndex index;

    index.add_document("c.cpp", {"pool"});
    index.add_document("a.md", {"thread", "pool"});

    const auto snapshot = index.snapshot();

    ASSERT_EQ(snapshot.size(), 2u);
    EXPECT_EQ(snapshot[0].first, "pool");
    EXPECT_EQ(snapshot[0].second, (std::vector<std::string>{"a.md", "c.cpp"}));
    EXPECT_EQ(snapshot[1].first, "thread");
}

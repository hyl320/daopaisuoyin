#include "loomlog/query_engine.hpp"

#include <gtest/gtest.h>
#include <vector>

TEST(QueryEngineTest, SingleKeywordReturnsMatchingDocuments) {
    loom::InvertedIndex index;
    index.add_document("a.md", {"thread", "pool"});
    index.add_document("b.md", {"logger"});

    EXPECT_EQ(loom::QueryEngine{}.query(index, "THREAD"), std::vector<std::string>{"a.md"});
}

TEST(QueryEngineTest, MultipleKeywordsUseAndSemantics) {
    loom::InvertedIndex index;
    index.add_document("a.md", {"thread", "pool"});
    index.add_document("b.md", {"thread"});
    index.add_document("c.md", {"pool"});

    EXPECT_EQ(loom::QueryEngine{}.query(index, "thread pool"), std::vector<std::string>{"a.md"});
}

TEST(QueryEngineTest, EmptyQueryReturnsNoMatches) {
    loom::InvertedIndex index;
    index.add_document("a.md", {"thread"});

    EXPECT_TRUE(loom::QueryEngine{}.query(index, "!!!").empty());
}

TEST(QueryEngineTest, MissingTermMakesAndQueryEmpty) {
    loom::InvertedIndex index;
    index.add_document("a.md", {"thread", "pool"});

    EXPECT_TRUE(loom::QueryEngine{}.query(index, "thread logger").empty());
}

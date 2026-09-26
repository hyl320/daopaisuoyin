#include "loomlog/directory_scanner.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(DirectoryScannerTest, RecursivelyFindsSupportedExtensions) {
    const auto root = std::filesystem::temp_directory_path() / "loomsearch_scanner_supported";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "nested");
    std::ofstream(root / "a.txt") << "a";
    std::ofstream(root / "b.md") << "b";
    std::ofstream(root / "nested" / "c.cpp") << "c";
    std::ofstream(root / "nested" / "d.hpp") << "d";
    std::ofstream(root / "image.png") << "skip";
    std::ofstream(root / "data.bin") << "skip";

    const auto files = loom::DirectoryScanner{}.scan(root);

    std::vector<std::string> names;
    for (const auto& file : files) {
        names.push_back(file.filename().string());
    }
    std::ranges::sort(names);

    const std::vector<std::string> expected{"a.txt", "b.md", "c.cpp", "d.hpp"};
    EXPECT_EQ(names, expected);
}

TEST(DirectoryScannerTest, ExtensionMatchingIsCaseInsensitive) {
    const auto root = std::filesystem::temp_directory_path() / "loomsearch_scanner_case";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    std::ofstream(root / "UPPER.MD") << "a";
    std::ofstream(root / "HEADER.HPP") << "b";
    std::ofstream(root / "skip.PNG") << "skip";

    const auto files = loom::DirectoryScanner{}.scan(root);

    std::vector<std::string> names;
    for (const auto& file : files) {
        names.push_back(file.filename().string());
    }
    std::ranges::sort(names);

    const std::vector<std::string> expected{"HEADER.HPP", "UPPER.MD"};
    EXPECT_EQ(names, expected);
}

TEST(DirectoryScannerTest, MissingRootReturnsEmptyList) {
    const auto root = std::filesystem::temp_directory_path() / "loomsearch_scanner_missing";
    std::filesystem::remove_all(root);

    EXPECT_TRUE(loom::DirectoryScanner{}.scan(root).empty());
}

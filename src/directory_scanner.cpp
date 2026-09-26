#include "loomlog/directory_scanner.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace loom {
namespace {

std::string lowercase_extension(const std::filesystem::path& path) {
    auto ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return ext;
}

bool is_supported_file(const std::filesystem::path& path) {
    static constexpr std::array extensions{".txt", ".md", ".cpp", ".hpp"};
    const auto ext = lowercase_extension(path);
    return std::ranges::find(extensions, ext) != extensions.end();
}

}  // namespace

//它返回一个 std::vector<std::filesystem::path>，也就是一组文件路径。执行流程
std::vector<std::filesystem::path> DirectoryScanner::scan(
    const std::filesystem::path& root
) const {
    std::vector<std::filesystem::path> files;
//如果 root 不存在，或者不是目录，直接返回空数组：
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
        return files;
    }
//遇到没有权限访问的目录时跳过，而不是中断程序。
    const auto options = std::filesystem::directory_options::skip_permission_denied;
//然后创建递归目录迭代器 it 从 root 开始递归遍历，end 表示遍历结束。
    std::filesystem::recursive_directory_iterator it(root, options, ec);
    const std::filesystem::recursive_directory_iterator end;

    for (; it != end && !ec; it.increment(ec)) {
//果当前项不是普通文件，就跳过：
        if (!it->is_regular_file(ec)) {
            ec.clear();
            continue;
        }

        // 只把第一版支持的文本/源码扩展名交给索引器，其他文件直接跳过。
        if (is_supported_file(it->path())) {
            files.push_back(it->path().lexically_normal());
        }
        ec.clear();
    }

    std::ranges::sort(files, [](const auto& left, const auto& right) {
        return left.string() < right.string();
    });

    return files;
}

}  // namespace loom

# LoomSearch

LoomSearch 是一个基于 C++20 线程池的并行文件索引器：递归扫描文件，并发读取和分词，构建倒排索引，然后从索引文件中查询关键词。

## 支持范围

第一版只索引这些文件：

```text
.txt
.md
.cpp
.hpp
```

暂不做 GUI、HTTP 服务、模糊搜索、复杂排名、数据库、增量索引和文件监听。

## 构建

```powershell
cmake -S . -B build -DLOOMLOG_BUILD_CLI=ON
cmake --build build
```

生成的命令行程序位于：

```text
build\loomsearch.exe
```

## 使用

建立索引：

```powershell
.\build\loomsearch.exe index .\documents --threads 8 --output .\index.bin
```

查询索引：

```powershell
.\build\loomsearch.exe query .\index.bin "thread pool"
```

输出示例：

```text
matches: 3
documents/thread_pool.txt
documents/logger.md
documents/design.md
```

多个关键词使用 AND 语义。例如 `thread pool` 只返回同时包含 `thread` 和 `pool` 的文件。

## 调试命令

旧命令仍然保留，用于观察线程池和日志组件：

```powershell
.\build\loomsearch.exe bench-pool --threads 8 --jobs 100000
.\build\loomsearch.exe bench-log --messages 100000 --file .\bench.log
.\build\loomsearch.exe grep --file .\loomsearch.log --keyword processed
```

## 架构

```text
index:
  DirectoryScanner -> ThreadPool -> FileProcessor -> FileResult
                                      |
                                      v
                             InvertedIndex -> IndexStore

query:
  IndexStore -> QueryEngine -> matches
```

worker 线程只负责读取和解析文件，不直接修改共享索引。主线程统一合并 `FileResult`，减少锁竞争，也让并发边界更容易解释和测试。

## 验证

运行全部测试：

```powershell
ctest --test-dir build --output-on-failure
```

只跑 CLI 往返测试：

```powershell
.\build\test_cli.exe
```

手动端到端验证：

```powershell
New-Item -ItemType Directory -Force -Path .\out\loomsearch-demo
Set-Content -Path .\out\loomsearch-demo\thread_pool.txt -Value 'thread pool worker'
Set-Content -Path .\out\loomsearch-demo\logger.md -Value 'thread logger'
.\build\loomsearch.exe index .\out\loomsearch-demo --threads 2 --output .\out\loomsearch-demo\index.bin
.\build\loomsearch.exe query .\out\loomsearch-demo\index.bin "thread pool"
```

期望输出包含：

```text
matches: 1
thread_pool.txt
```

## Benchmark

本机轻量数据，时间为一次运行结果，用于观察量级，不作为跨机器结论：

```text
处理器标识: Intel64 Family 6 Model 154 Stepping 3, GenuineIntel
线程池: 100000 个任务，4 线程，1759 ms
std::async: 100000 个任务，17988 ms
异步日志: 1000000 条消息，submit 1142 ms，total 1142 ms，dropped 0
```

## 设计取舍

- worker 返回 `FileResult`，主线程合并索引，避免多个线程同时写 `unordered_map`。
- 索引格式先用文本格式，方便查看、调试和测试。
- 查询先支持单关键词和多关键词 AND，不引入排名系统。
- Logger 使用独立后台线程，不放进文件处理线程池，避免监控日志被慢任务拖住。

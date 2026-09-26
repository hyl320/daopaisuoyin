# loomlog 完整执行计划（可迁移、可复用）

把这份文件整份带走。换电脑、换 Cursor、换对话，把 **第 0 节提示词** 贴给下一个 AI，再把本文件丢进仓库根目录即可接着做。

- 项目名：`loomlog`
- 定位：C++20 **线程池 + 异步日志库 + 命令行压测/检索**
- 目的：简历投 C++ 系统 / 智驾 / 游戏工具 / 基础架构
- 不做：AI、Qt、Web、无锁队列第一版、spdlog 套壳
- 语言标准：C++20
- 构建：CMake 3.20+
- 测试：GoogleTest
- 质量：ASan 必过，有锁路径再跑 TSan

---

## 0. 给下一个 AI 的提示词（整段复制）

```text
你在帮我实现仓库里的 loomlog。先读 ./loomlog-plan.md，严格按它做，不要加计划外功能。

当前进度：只做线程池。范围锁定为：
1. ThreadPool::submit 返回 std::future
2. 优雅退出（shutdown / 析构）：不再接新任务，排空队列，join 所有 worker
3. GoogleTest 至少一个用例：提交 N 个任务，结果正确，退出不卡死
4. ASan 跑测试必须干净
不要写 Logger，不要写 CLI，不要无锁队列。

约束：
- C++20，CMake，GoogleTest（FetchContent）
- 队列先用 mutex + condition_variable + std::queue
- shutdown 后再 submit 抛 std::runtime_error
- shutdown 可重复调用，必须幂等
- 禁止在头文件 using namespace
- 每个可运行目标结束后给出：怎么编译、怎么带 ASan 跑测试

先给目录和 CMake，再实现头文件/源文件/测试。不要一次写完整个四週计划。
```

进度往前走时，把「当前进度」那几行改成你做到哪。

---

## 1. 一句话目标和非目标

**目标：** 业务线程丢任务、丢日志都不阻塞；后台线程跑活、批量写盘；命令行能压测、能按级别/关键字扫日志。做出 **可复现的数字** 和 **一个你修过的并发 bug 故事**。

**非目标（写进仓库也不许加）：**

- 任何 LLM / Copilot 包装 / 自动审查 / 自动生成测试
- Qt、Web、登录、监控大盘
- 无锁队列、无锁 ring buffer（测量后再说，第一版禁止）
- 网络收集、分布式、syslog
- 再包一层 spdlog / Boost.Asio / Folly
- 通用线程池框架（优先级、任务窃取、CPU 亲和）——那是另一份简历

线程池单独拿出来当项目名 **没有竞争力**。它只是第 1 周的砖。竞争力来自：优雅退出 + ASan/TSan + 异步日志数字 + 那个 bug。

---

## 2. 四周进度（唯一时间表）

| 周   | 状态门禁（没过不准进入下一周）                      | 你要得到的数字                                      |
| --- | ------------------------------------ | -------------------------------------------- |
| 1   | `submit` + 优雅退出 + gtest + **ASan 绿** | 100 万空任务耗时 vs 裸 `std::thread` / `std::async` |
| 2   | 异步日志 + 级别 + 按大小滚动 + gtest + ASan 绿   | 同步写文件 vs 异步：主线程 P99、落盘 MB/s                  |
| 3   | CLI `bench` / `grep` + **TSan 绿**    | 一张 README 表                                  |
| 4   | README、DESIGN.md、90 秒终端录屏、简历四条定稿     | 可以投递                                         |

卡住就停在当周，不要用新功能掩盖红的 sanitizer。

---

## 3. 仓库目录（一次定死）

在空目录初始化：

```text
loomlog/
  loomlog-plan.md          # 本文件，跟着仓库走
  CMakeLists.txt
  CMakePresets.json
  .gitignore
  README.md                # 第 4 周写完，第 1 周先占位
  DESIGN.md                # 第 1 周起每做一个取舍就记一行
  include/loomlog/
    thread_pool.hpp
    logger.hpp             # 第 2 周再创建
  src/
    thread_pool.cpp
    logger.cpp             # 第 2 周
    cli.cpp                # 第 3 周
  tests/
    test_thread_pool.cpp
    test_logger.cpp        # 第 2 周
  benches/
    bench_pool.cpp         # 第 1 周末
    bench_log.cpp          # 第 2 周末
  examples/
    hello.cpp              # 第 2 周：池 + 日志各打一行
  .github/workflows/
    ci.yml                 # 第 1 周末：Debug + ASan 跑测试
```

`.gitignore` 至少：`build/` `cmake-build-*/` `.cache/` `.vscode/` `.idea/` `*.log`

---

## 4. 环境（换机器先跑这段）

依赖：

- Linux 或 macOS（Windows 也能做，但 sanitizer 以 Linux 为准，简历写 Linux）
- CMake >= 3.20
- Clang 或 GCC，支持 C++20
- Ninja（可选）

```bash
# Debian/Ubuntu
sudo apt update
sudo apt install -y cmake g++ clang ninja-build git

# 确认
cmake --version
c++ --version
```

GoogleTest **不要** 系统包，用 CMake `FetchContent`，保证换机器可复现。

建议编译器：日常 GCC 或 Clang 均可；**ASan/TSan 用 Clang 更省事**。

---

## 5. CMake 骨架（第 1 天就写上）

`CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.20)
project(loomlog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LOOMLOG_BUILD_TESTS "Build tests" ON)
option(LOOMLOG_BUILD_BENCH "Build benches" ON)
option(LOOMLOG_BUILD_CLI "Build CLI" OFF) # 第 3 周改 ON

add_library(loomlog
  src/thread_pool.cpp
  # src/logger.cpp   # 第 2 周取消注释
)
add_library(loomlog::loomlog ALIAS loomlog)
target_include_directories(loomlog PUBLIC include)

if(LOOMLOG_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
  add_executable(test_thread_pool tests/test_thread_pool.cpp)
  target_link_libraries(test_thread_pool PRIVATE loomlog GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(test_thread_pool)
endif()

if(LOOMLOG_BUILD_BENCH)
  add_executable(bench_pool benches/bench_pool.cpp)
  target_link_libraries(bench_pool PRIVATE loomlog)
endif()
```

带 ASan 的用法（不要写死进默认 flags，用命令行开）：

```bash
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"

cmake --build build-asan -j
cd build-asan && ctest --output-on-failure
```

TSan **不能和 ASan 同时开**，第 3 周另配 `build-tsan`。

`CMakePresets.json` 可选。没有 preset 也不挡第 1 周。

---

## 6. 第 1 周：线程池（今天开始，只做这些）

### 6.1 对外 API（锁死，不要「顺便」加功能）

`include/loomlog/thread_pool.hpp`：

```cpp
#pragma once

#include <cstddef>
#include <functional>
#include <future>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace loom {

class ThreadPool {
public:
    // nthreads == 0 表示 std::max(1u, hardware_concurrency())
    explicit ThreadPool(std::size_t nthreads = 0);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool();

    // 停止接新任务，排空已排队任务，join workers。可重复调用。
    void shutdown();

    std::size_t thread_count() const noexcept;
    bool accepting() const noexcept; // shutdown 后为 false

    template <class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    std::size_t nthreads_{0};
};

} // namespace loom

// 模板放头文件底部
template <class F, class... Args>
auto loom::ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
{
    using R = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
    auto task = std::make_shared<std::packaged_task<R()>>(
        [fn = std::decay_t<F>(std::forward<F>(f)),
         tup = std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...)]() mutable -> R {
            return std::apply(std::move(fn), std::move(tup));
        });
    std::future<R> fut = task->get_future();
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (stop_) {
            throw std::runtime_error("ThreadPool is shutdown");
        }
        tasks_.emplace([task] { (*task)(); });
    }
    cv_.notify_one();
    return fut;
}
```

实现要点（写在 `src/thread_pool.cpp`，不要把 worker 逻辑全塞进头文件）：

1. 构造：创建 `nthreads` 个 `std::thread(&ThreadPool::worker_loop, this)`
2. `worker_loop`：加锁，等 `!tasks_.empty() || stop_`；如果 `stop_ && tasks_.empty()` 则 return；否则取出任务，解锁后执行
3. **先解锁再跑任务**，否则池内任务再 `submit` 会死锁
4. `shutdown`：`stop_ = true`；`cv_.notify_all()`；join 还没 join 的线程；清空 workers 或用 `joinable()` 判断
5. 析构调用 `shutdown()`
6. 任务抛异常：`packaged_task` 会把异常放进 future，worker **不要** 让异常逃出 `worker_loop`（`packaged_task::operator()` 已消化）

第 1 周 **不要**：

- 有界队列、拒绝策略
- 优先级
- `std::jthread` 也可以用，但要能讲和 `std::thread` + 自己 join 的区别；不会讲就先 `std::thread`
- 无锁

### 6.2 测试名单（`tests/test_thread_pool.cpp`）

最少这 6 个。第 1 天可以只写前 2 个，周内补齐。

| 用例名                           | 断言                                            |
| ----------------------------- | --------------------------------------------- |
| `SubmitReturnsValue`          | `submit([]{return 41+1;}).get() == 42`        |
| `ManyTasksComplete`           | 1000 个任务把 `atomic<int>` 加到 1000，然后 `shutdown` |
| `ShutdownIsIdempotent`        | 连续 `shutdown()` 三次不卡死                         |
| `SubmitAfterShutdownThrows`   | shutdown 后再 submit，抛 `std::runtime_error`     |
| `DestructorJoins`             | 作用域结束前所有任务完成（用 promise/atomic 观察）             |
| `ExceptionPropagatesToFuture` | 任务里 `throw`，`future.get()` 抛同一类型              |

卡死判定：测试加 10s 超时（gtest `TEST` 里自己 `wait_for`，或 CI 里 `timeout 30 ctest`）。

### 6.3 第 1 天最小路径（严格按这个顺序）

```bash
git init loomlog && cd loomlog
# 拷贝本计划为 loomlog-plan.md
# 写 CMakeLists.txt、.gitignore
# 写 thread_pool.hpp / thread_pool.cpp
# 写 tests/test_thread_pool.cpp 里的 SubmitReturnsValue + ManyTasksComplete
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan -j
cd build-asan && ctest --output-on-failure
```

ASan 红了就修，不要开始写日志。

### 6.4 第 1 周末 bench（可以很土）

`benches/bench_pool.cpp`：

- 提交 1,000,000 个「空任务」（`atomic` fetch_add 1）
- 计时 `submit` 全部 + `shutdown` 结束
- 对比 1：每次 `std::thread` 一个任务再 join（可以只跑 10 万，否则太慢）
- 对比 2：`std::async(std::launch::async, ...)` 同样次数

把结果记进 `DESIGN.md`，不要编数字。机器写清楚（CPU 型号、核数）。

---

## 7. 第 2 周：异步日志

**只有第 1 周 ASan 绿了才开始。**

### 7.1 行为锁死

- 级别：`Error=0, Warn=1, Info=2, Debug=3`，低于阈值的调用直接 return
- 调用线程：格式化成字符串（`std::format`）后入队，**不 fwrite**
- 后台 **单独一条线程**（不要复用 ThreadPool，见 10.1）
- 批量：满 256 条 **或** 等了 16ms 就刷盘
- 文件滚动：单文件超过 32MB 则 `app.log` → `app.log.1`，只保留 N=3 个备份
- 队列上限 8192：满了丢 Debug/Info，Warn/Error 仍尝试入队；若队列仍满，Error 允许短暂阻塞最多 50ms，超时则丢并 `dropped` 计数 +1
- 进程退出：Logger 析构必须排空并 close 文件

### 7.2 API

```cpp
namespace loom {
enum class Level { Error, Warn, Info, Debug };

struct LoggerConfig {
    std::string path = "logs/app.log";
    Level min_level = Level::Info;
    std::size_t queue_limit = 8192;
    std::size_t batch_size = 256;
    std::chrono::milliseconds flush_interval{16};
    std::size_t rotate_bytes = 32 * 1024 * 1024;
    std::size_t rotate_keep = 3;
};

class Logger {
public:
    static Logger& instance();
    void start(LoggerConfig cfg);
    void shutdown();

    void log(Level lv, std::string_view msg);

    template <class... Args>
    void info(std::format_string<Args...> fmt, Args&&... args);
    // error / warn / debug 同样

    std::uint64_t dropped() const noexcept;
};
}
```

第一版可以先不用单例，构造函数注入 config，单例留给 CLI。库代码更好测。

行格式：

```text
2026-09-08T18:00:00.123Z INFO thread=1234 hello 42
```

时间用 UTC。时区争议会浪费一天，不要争。

### 7.3 测试

- 级别过滤：min=Info 时 Debug 不出现在文件
- 内容：`info("hello {}", 42)` 文件里有 `hello 42`
- 析构后文件完整（最后一条在）
- 滚动：把 `rotate_bytes` 调成 4KB，写很多条，出现 `app.log.1`
- `dropped`：把 `queue_limit=8`，狂写 Debug，dropped > 0
- ASan 绿

### 7.4 bench

同一条消息写 100 万次：

1. 每次 `fprintf` 同步写
2. 你的异步 Logger

记：调用线程耗时、最终文件大小、dropped。

---

## 8. 第 3 周：CLI + TSan

CMake 把 `LOOMLOG_BUILD_CLI` 打开。`src/cli.cpp` 一个可执行文件 `loomlog`。

```text
loomlog bench-pool  --threads 8 --jobs 1000000
loomlog bench-log   --n 1000000 --sync   # --sync 走 fprintf 对照
loomlog grep --level INFO --keyword timeout --file logs/app.log
```

`grep`：**线性扫描**。支持 `--level`、`--keyword`（子串即可，不要上正则引擎）、可选 `--from` `--to`（按行前缀时间过滤，解析不了就跳过）。不要倒排、不要 mmap 优化，除非 bench 显示扫 1GB 太慢且你还能讲清。

TSan：

```bash
cmake -S . -B build-tsan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake --build build-tsan -j
cd build-tsan && ctest --output-on-failure
```

红了就修。常见点：shutdown 和 submit 的 stop_ 可见性（已经 atomic）、析构时还有线程在用 `this`。把修法写进 DESIGN.md，这是简历素材。

GitHub Actions：`ubuntu-latest`，ASan 测试必须绿。TSan 绿了也加上。

---

## 9. 第 4 周：包装到能投

### 9.1 README 必须有的节

1. 一句话
2. 构建命令（复制即用）
3. 架构（线程池一块、日志一块，各画 5 行 ASCII 即可）
4. **数字表**（你机器上的，注明 CPU）
5. 和「作业线程池」的差别：优雅退出、sanitizer、异步日志、dropped 策略
6. 非目标列表

### 9.2 DESIGN.md 三道必答题（面试全从这儿来）

1. 日志为什么不进线程池？  
   保序、批量、不被重任务饿死。
2. 队列为什么第一版用 mutex 而不是无锁？  
   正确退出和 TSan 比微秒重要；无锁留到有测量再说。
3. 队列满了为什么丢低级别而不是堵住业务？  
   选一个场景写死：例如游戏/接入，业务比 DEBUG 重要。

另开一节 **Bug log**：日期、现象、sanitizer 输出摘要、根因、补丁。至少一条真的。

### 9.3 简历四条（数字自己填，禁止空着投）

中文：

- 用 C++20 实现固定大小线程池（任务队列、条件变量、优雅退出），100 万空任务耗时 **x ms**，对比频繁创建 `std::thread` 降低 **y%**
- 实现异步日志：调用线程只入队，后台批量刷盘 + 按大小滚动，吞吐 **x MB/s**，业务线程 P99 **z μs**
- 队列满时丢弃 DEBUG、保留 ERROR，避免日志反压阻塞工作线程；记录 dropped 计数
- GoogleTest + ASan/UBSan/TSan；修复过一次 shutdown 竞态 / 析构死锁（DESIGN.md 有记录）

英文投外企把同一事实翻过去，不要另编。

### 9.4 录屏

90 秒终端：编译 → `ctest` → `loomlog bench-pool` → `loomlog bench-log`。不要配乐、不要 PPT。

---

## 10. 设计约束（避免做着做着变样）

### 10.1 日志必须独立线程

线程池里的任务可能跑很久。日志刷盘必须稳定、短、尽量保序。独立线程。

### 10.2 第一版有锁

`std::mutex` + `std::condition_variable` + `std::queue`。假唤醒用 `while` 不用 `if`。

### 10.3 stop 标志用 `atomic<bool>`

配合 mutex 也没问题，shutdown 与 worker 的握手以 mutex/cv 为主，atomic 防止胡乱猜想。

### 10.4 禁止在析构里 throw

`shutdown` 吞 join 的系统错误可 `std::terminate` 或记录；不要把异常甩出析构。

### 10.5 头文件自给自足

include 什么用什么。不要在头文件 `using namespace std`。

---

## 11. 面试时怎么讲（3 分钟稿）

1. **问题：** 作业里乱 `new thread`、同步 `cout` 拖主路径。  
2. **做法：** 固定 worker + 队列；日志单独后台批量写。  
3. **证据：** README 那张表。  
4. **坑：** 某次析构没 `notify_all`，测试偶发卡死 / TSan 报；改了 stop+notify+join 顺序。

背这四句，再能现场默写 `worker_loop` 的等待条件，C++ 校招够用。

可能追问：

- 假唤醒为什么 `while`？
- `submit` 里锁住时跑用户任务会怎样？
- `packaged_task` 和 `std::function` 包一层的原因？
- shutdown 后已在跑的任务怎么办？（跑完；已排队的要跑完；新的拒绝）
- 为什么不用 `std::async` 当池？（实现未指定，可能每次新线程，无法复用、无法限流）

---

## 12. 完成定义（Definition of Done）

**第 1 周 DoD**

- [ ] `ctest` 在 ASan+UBSan 下全绿
- [ ] shutdown 后再 submit 必抛
- [ ] 析构不卡死（连续跑测试 50 次也稳定）
- [ ] `bench_pool` 能打出一条数字，记入 DESIGN.md

**整项目 DoD（能投简历）**

- [ ] 上面第 1 周 DoD
- [ ] 异步日志测试全绿，ASan 绿，TSan 绿
- [ ] CLI 三条子命令能跑
- [ ] README 有真实数字表
- [ ] DESIGN.md 有三道取舍 + 至少一条 bug
- [ ] GitHub 公开，clone 后 README 命令能复现
- [ ] 没有 API key、没有巨大二进制、没有半截 UI

---

## 13. 换地方接着做时的检查清单

1. 把本文件放进仓库根目录  
2. `git status` 看当前周是否只改了计划允许的文件  
3. 贴第 0 节提示词，改「当前进度」  
4. 先跑 ASan 测试，红的先修  
5. 不要让新 AI「顺便重构」或「升级成无锁」

进度标记（自己改）：

```text
当前周：1
线程池：进行中 / 完成
日志：未开始
CLI：未开始
ASan：未跑 / 红 / 绿
TSan：未跑
数字：无
```

---

## 14. 和这次对话的结论（防止下次又摇摆）

- 不往这个项目里塞 AI。C++ 系统岗看线程、内存、数字。硬贴 AI 会减分。  
- 线程池 alone 没有竞争力；它是砖。  
- 为了项目而项目时，只做自己能讲 15 分钟的东西。  
- 第 5 周想加 Qt / 无锁 / AI 审查：先投递，用面试反馈再决定，不要在第 1 周幻想。

---

## 15. 今天立刻执行的 8 步

1. 建空目录 `loomlog`，`git init`  
2. 放入本文件  
3. 写 `.gitignore` 和 `CMakeLists.txt`  
4. 写 `thread_pool.hpp` / `thread_pool.cpp`（submit + shutdown + worker_loop）  
5. 写两个测试：`SubmitReturnsValue`、`ManyTasksComplete`  
6. 按 6.3 配 ASan 构建并 `ctest`  
7. 绿了再补 6.2 其余测试  
8. 全部绿了再写 `bench_pool.cpp`

不要并行开 Logger。

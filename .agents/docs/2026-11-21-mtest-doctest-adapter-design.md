---
subject: tests
status: active
---

# doctest 测试接入 mtest 协议：改用上游适配器 `mcpplibs.mtest.doctest`

> 日期：2026-11-21 · mcpp 2026.11.20.1 · mtest 0.11.1 · doctest 2.4.12
>
> 来源：[mcpplibs/boost-m#7](https://github.com/mcpplibs/boost-m/issues/7) 的调查与计划；
> mcpp `docs/specs/test.md`（SPEC-017）B6；mtest 0.11.0（mcpp-community/mtest#26）。

## 0. 结论

#7 计划的第一项"写 doctest 适配器入口包"由上游完成：mtest 0.11.0 发布了
`mcpplibs.mtest.doctest`（L1）。它是入口包（`test-protocol = 1`、`test-main = true`），
doctest 的实现和 `main` 只在包里编译一次，依赖只有 `compat.doctest` 2.4.12 与
`nlohmann.json`，不经过 mtest 库、`mcpplibs.os` 与 openkal。

本仓库直接使用它，不再自写适配器：根包与 `tests/dll` 的 `[dev-dependencies]` 加上
`mcpplibs.mtest.doctest = "0.11.1"`，删去全部 139 处 `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`
（含两个移植生成器的模板）。本地（Linux x86_64，gcc 16.1.0）根包 417 个测试程序与
`tests/dll` 的 4 个全部通过；llvm@22.1.8 抽查 6 个受影响最大的文件通过。

[2026-11-09 设计记录](2026-11-09-module-test-hardening-design.md) §4.1 中"每个测试文件
自己定义 `main`"的约定由本记录取代：测试文件只 `#include <doctest/doctest.h>`，不定义
`DOCTEST_CONFIG_IMPLEMENT*`。

## 1. 只能一次全部迁移

mcpp 对待入口包的方式决定了不能逐库试点：

- 适配器包的库源 `src/protocol.cpp`（doctest 实现 + reporter）链接进根包的**每个**测试
  程序，包括冒烟测试与 Boost.Test 测试。仍保留 `IMPLEMENT_WITH_MAIN` 的文件因此重复
  定义 doctest 的全部符号。
- 入口 `src/main.cpp` 链接进"不定义 `main`"的测试。mcpp 判断这一点靠扫描源文件的
  **文本**（`plan.cppm` 的 `source_defines_main`：去掉注释与字面量，按预处理条件取舍，
  找 `int main(` / `auto main(`）。由宏或头文件给出 `main` 的文件被判为没有 `main`。

后者涉及 4 个文件：`tests/cobalt/co_main.cpp`（`main` 在 `<boost/cobalt/main.hpp>`）与
Boost.Test 头文件形态的 `tests/test/custom_types.cpp`、`tests/test/included/{smoke,doc_reference}.cpp`
（`main` 在 `<boost/test/included/unit_test.hpp>`；`#ifdef MCPP_FEATURE_UNIT_TEST_FRAMEWORK`
分支里的 `int main()` 被扫描当作不编译）。处理：在文件中写出与头文件相同的声明
`int main(int argc, char* argv[]);`（cobalt 的 `detail/main.hpp:31` 就有同一声明），
Boost.Test 文件写在 `#else` 分支里。

非 doctest 程序链接了 `protocol.o`，但 Linux 上 `--gc-sections` 去掉了其中的
`mtest-protocol=1` 标记（实测 `accumulators`、`asio/smoke`、`test/included/smoke`、
`cobalt/co_main` 中均无）。mcpp 判断协议程序看的是入口声明，不看标记，记录中这些程序的
`protocol` 为 `null`。

## 2. doctest 实现曾替测试提供的 C 头文件

`DOCTEST_CONFIG_IMPLEMENT` 让 doctest.h 包含 `<csignal>`、`<cerrno>`、`<climits>`、
`<cmath>`、`<cstdint>` 等。移植的上游示例原本经由 Boost 头文件得到它们，移植时 Boost
头文件被模块取代，于是一直靠 doctest 实现兜底。删去实现后 11 个文件编译失败：

| 缺少 | 文件 |
|---|---|
| `SIGINT`、`SIGTERM`、`signal` | asio/network_cpp14_cpp20、asio/network_http、beast/examples_websocket、math/examples_2、mqtt5/examples |
| `errno` | json/examples |
| `INT_MAX`、`INT_MIN` | convert/examples_stream（手写） |
| `FP_ZERO` | decimal/api_cmath_io、math/doc_tools（手写） |
| `::sin`、`::cos`、`::floor` | geometry/examples、geometry/examples_algorithms |
| `::uint16_t` | mqtt5/examples |

这些是宏或全局命名空间中的 C 名字，`import std` 不提供。`tools/port_tests/port_examples.py`
新增 `C_NAMES`：`import std` 形态的移植在用例中用到这些名字、而保留的头文件里没有对应头时，
补上 `#include`。已提交的生成文件按同一规则补齐（另有 `asio/network_cpp11` 多得一个
`<cmath>`，无害）；`regen.py` 需要发布包与 oracle 构建，本轮未重跑。手写的 3 个文件直接补
`#include`。

## 3. GCC 16：`npos` 的强定义

`asio/custom_types` 与 `geometry/api_reference` 链接时 `std::string::npos` 重复定义，另一方是
`libstdc++.a(string-inst.o)`。这两个翻译单元只 `import std`，`CHECK(s.find(..) != std::string::npos)`
经 doctest 的表达式分解按 `const&` 绑定 `npos`（ODR 使用），GCC 16 为它发出了不在 COMDAT 组
中的 `GLOBAL` 定义（`basic_string<char>` 有显式实例化声明，本不应发出）。以前没有其他目标
文件需要 `string-inst.o`；`protocol.o` 需要，静态 libstdc++ 于是把它拉进来。在文件中文本包含
`<string>` 无效。处理：改写为 `s.contains(..)`（C++23），不再 ODR 使用 `npos`。仓库中另有
14 处同类写法，它们的目标文件没有强定义，未改。

## 4. 覆盖率核对

`tools/ci/test_coverage.py check --listing` 原先从程序输出（`run_output`）解析 doctest 的
`--list-test-cases`。在 mcpp 之下，适配器把 reporter 换成自己的，doctest 列表模式调用它的
`report_query`，用例作为协议的 `case` 事件交给 mcpp，进入记录的 `listed` 字段，输出为空。
`listing_cases` 改为优先读 `listed`（`name` 即 doctest 的用例名，模板实例同样补出模板名）。
CI 的命令不变。对照：旧脚本读新的列表报 1791 处"程序未列出该用例"，新脚本通过。

## 5. 实测

**编译时间**（gcc 16.1.0，dev，单线程，同一条编译命令加与不加
`-DDOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`）：

| 文件 | 现在 | 原先 | 节省 |
|---|---:|---:|---:|
| algorithm/api_string | 2.32 s | 3.75 s | 1.44 s |
| json/custom_types | 1.65 s | 3.02 s | 1.37 s |
| math/doc_sf | 4.58 s | 6.11 s | 1.53 s |
| asio/api_overview | 9.00 s | 10.57 s | 1.57 s |
| geometry/api_reference | 17.13 s | 18.49 s | 1.35 s |
| core/api | 1.17 s | 2.60 s | 1.43 s |
| beast/doc | 10.70 s | 12.79 s | 2.09 s |
| container/api | 3.17 s | 4.75 s | 1.59 s |

平均每个文件 1.55 s，与 #7 §3 的估计一致；139 个文件约 3.6 分钟 CPU 时间每条 gcc 腿。

**协议**（临时探测程序，未提交）：`--cases` 列出用例与位置；`--case` 逐字节选择，含 `*` 与
逗号的名字也正确；失败带文件与行；`--failed` 只重跑失败的用例；用例中 `raise(SIGSEGV)`
时整体运行报到该用例，`--split case` 下其余用例照常运行；`doctest::skip()` 报为跳过，
`may_fail` 通过。asio 已有的 `ex_case_names` 看门狗 listener 与适配器共存。全量运行得到
136 个协议程序、1815 个用例记录。

#7 提到的"`--cases` 把 L0 程序报成失败"在 mcpp 2026.11.20.1 中不再出现（报为
"one case, its exit status"）。

## 6. 尚未验证

- windows llvm-msvc 与 arm64 musl 腿：本轮以 CI 为准。Windows 上 lld-link 是否同样去掉
  非 doctest 程序中的协议标记也待 CI 观察（不影响 mcpp，只影响独立运行的 mtest 引擎）。
- `regen.py` 重新生成与已提交文件逐字节一致（§2）。

## 7. 上游适配器的缺口（向 mcpp-community/mtest 反馈）

- 同名用例得到相同 ID（P3.2 要求确定的去重）：`--case duplicate` 运行两个，`--split case`
  合成一个进程。
- 读取后未从环境中清除 `MTEST_*`（P2.2），子进程会继承。
- P3.3 调度提示只映射了 `skip`；`timeout`、`may_fail`、`should_fail`、`expected_failures` 未映射。
- 异常与崩溃的 `failure` 事件 `file` 为空、`line` 为 0。
- 程序在 mcpp 之外运行且设置了 `MTEST_PROTOCOL` 时，doctest 的 `--list-test-cases` 不再打印到
  标准输出（本仓库已改为读 `listed`，记此备查）。

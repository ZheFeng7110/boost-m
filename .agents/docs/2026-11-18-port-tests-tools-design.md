---
subject: tests
status: landed
---

# 示例移植脚本进入 `tools/port_tests/`：升级 Boost 时重跑上游测试的移植

> 日期：2026-11-18 · 分支 `test/module-test-hardening`
>
> 来源：[模块测试加固的遗留问题](todos/module-tests-open-issues.md) 的最后一项；
> [模块封装层测试加固](2026-11-09-module-test-hardening-design.md) §11.3 的“脚本”一段。

## 0. 结论

移植脚本整理后提交到 `tools/port_tests/`，入口是 `regen.py`。整理后的脚本重新生成 `tests/`，
与已提交的文件对照：只有两类差异，一是从不编译的输出（时间、线程完成顺序，见 §3），
二是两个比脚本旧的文件（bloom、container，按当前脚本重新生成）。
升级 Boost 的流程：

```sh
python3 tools/import_boost.py && python3 tools/apply_patches.py
python3 tools/port_tests/regen.py           # 全部库；或 regen.py asio math:2
git diff tests/                             # 审阅：新的输出、FLAG 行
python3 tools/ci/test_coverage.py scan --archive <tarball>
python3 tools/ci/test_coverage.py link -v
python3 tools/port_tests/mark_coverage.py <lib> excluded "<reason>" <path 片段>...
python3 tools/ci/test_coverage.py check
```

## 1. 布局

| 路径 | 内容 | 原来的位置 |
|---|---|---|
| `regen.py` | 入口：准备、按库/组并行运行、汇总 FLAG 行 | 各 `regen_*` 手工逐个运行 |
| `port_examples.py` | 移植器 | `.tmp/scripts/port_examples.py` |
| `net_gen.py` | 回环网络示例的驱动（asio、beast、cobalt、mqtt5 共用） | `net_gen.py` 与 `regen_asio_net.py` 中的副本 |
| `common.py` | 路径与准备步骤 | 散在各脚本里的 `.tmp/...` 路径 |
| `libs/<lib>.py` | 每个库的示例清单、选项与生成文件的头注释 | `regen_<lib>.py` / `.sh`、对话中一次性的命令（bloom） |
| `helpers/*.hpp` | harness（asio 回环、beast 客户端、cobalt、mqtt5 broker、临时目录） | `*_helpers.hpp` |
| `sources/<lib>.patch` | 移植前改过的示例源码（bimap 4 个、describe 1 个），对发布包的补丁 | `.tmp/ports/` 下的改过的副本 |
| `mark_coverage.py` | 批量设置 `coverage.toml` 条目的状态或用例 | `mark.py` |

工作目录是 `target/port-tests/`（已被忽略）：发布包中 17 个库的 `libs/<lib>/`、打过补丁的源码、
oracle 库、日志。发布包就是 `tools/import_boost.py` 固定的那一个（核对 SHA-256），默认取
`target/vendor-import/` 下的，没有就下载。

## 2. 补上的、原来只在本地留下产物的步骤

- **oracle 库。** 原来是对话里手工编出的 `.tmp/oracle-libs/*.a`；现在由 `common.py` 的
  `ORACLE_LIBRARIES` 描述（serialization、container 的 pmr、json、`port/system_extras.cpp`、
  regex、program_options、url、thread、cobalt 的源码表取自 `mcpp.toml`），首次运行时编出。
- **asio 示例分类。** `asio_cls.txt` 是一次 grep 的结果（argv、acceptor、cin 的出现次数），
  现在 `libs/asio.py` 每次从发布包重新统计，结果与原文件一致。
- **改过的示例源码** 改为补丁，`git apply` 到发布文件的副本上；生成文件引用的路径仍是发布包中的路径。
- 头注释原来由 shell 脚本用 `sed -n 1,Np` 从已提交的文件中取，现在写在 `libs/<lib>.py` 中。

未收入的：`count_snippets.py`、`doc_code.py`（调研用）、`mark_cases.py`、`mark_asio.py`
（`test_coverage.py link` 现在也按示例目录下的相对路径匹配 `example: <dir>/<stem>`，
覆盖了 `mark_cases.py`；`mark_asio.py` 的排除理由已在 `coverage.toml` 中，`scan` 会保留）、`mt.sh`。

## 3. 对照与修正

用 `.tmp/scripts/` 原样重跑一遍作为基线，再用整理后的脚本重跑，都与 `HEAD` 的 `tests/` 比较。

- **从不编译的输出会变。** `--output-unless 'X=1'` 的用例仍把 oracle 的输出写进 `#if !(1)` 块作参考，
  其中的耗时、日期、线程顺序、随机读数每次都不同（asio 的 fork_join / parallel_sort / daytime
  客户端、cobalt 的 thread、mqtt5 的 publisher）。重新生成时这些块的变化不必提交。
- **同一示例的多个 `--output-unless`。** 已提交的 asio `wait_for_all`、`ranged_wait_for_all`
  有两层条件，而当时的脚本只保留最后一个。改为按参数顺序嵌套，外层在前。
- **math 的切分。** 已提交的四个文件（31/29/31/32 个用例）不是按步长切分的结果：后来排除的示例
  是直接从文件里删掉的。改为“示例留在已有它的文件中，新示例放进用例最少的文件”，重现了已提交的
  切分，升级时的 diff 也只涉及增删的示例。
- **bloom、container 比脚本旧。** 前者还是早期的 argv 写法，后者保留了 QuickBook 的
  `/*<-*/` 标记（脚本后来会去掉）。按当前脚本重新生成；`mcpp test bloom/`、
  `container/` 在 gcc 16.1.0 与 llvm 22.1.8 上通过。
- **bimap 的用例顺序。** 原脚本用 `ls` 列出 `example/*.cpp` 与 `mi_to_b_path/*.cpp`，顺序是
  整条路径的字节序（`mi_to_b_path/` 在 `mighty_bimap.cpp` 之前）；`libs/bimap.py` 照此排序。
- **callable_traits 头注释中的示例数** 改为统计发布包（现在仍是 37）。
- **`CLANG_NO_DEFAULT_CONFIG`。** `gen_exports.py` 需要它，oracle 不能有它（oracle 程序会链接失败，
  用例随之不比对输出，不报错）；`common.py` 在运行前把它从环境中去掉。
- oracle 程序的 60 秒超时在机器满载时偶有触发（Boost.Test 的 example 组与其他组并行时出现过一次，
  单独重跑通过），表现为该任务失败：已写出的文件是完整的，其余保持原样，重跑该组即可。

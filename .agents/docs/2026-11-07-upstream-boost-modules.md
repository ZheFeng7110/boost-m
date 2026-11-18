---
subject: upstream-modules
status: abandoned
---

# 上游已提供模块的库：直接使用 `boost.<lib>`，撤掉 mwrap 门面

> 日期：2026-11-07（2026-11-18 补 §11.4–§13）
>
> 状态：**尝试记录，未落地。** clang 全量测试通过，gcc 16.1 编译失败（§12）。代码改动没有提交，
> 仓库只保留本记录，供以后 gcc 支持"导入之后再文本包含"时重新考虑这个方向（§13）。文中的
> "删除"、"新增"、"改为"描述的都是当时工作树里的改动，不是仓库现状。
>
> 依据：`.tmp/boost-1.92.0-modules.json`（扫描 `libs/*/modules/*.cppm` 与已安装头文件中的
> `BOOST_USE_MODULES` 守卫得到的 1.92.0 上游模块清单）
>
> 相关：[2026-10-28 与 Boost 上游兼容](2026-10-28-upstream-compat.md) §2.5 ·
> [2026-11-05 JSON AST 生成](2026-11-05-json-ast-generation.md) ·
> [architecture.md §3.1](../../docs/architecture.md) · [usage.md §2.2](../../docs/usage.md)

## 1. 结论

Boost 1.92.0 有 7 个库在仓库内自带 C++20 模块接口单元。其中 6 个按本文处理：

1. **不提供 mwrap 封装**：删除 `src/<lib>.cppm`、`generated/exports/<lib>.inc|.deps`
   以及它们的导出策略条目。
2. **feature 直接编译上游单元**：`[features.<lib>].sources` 指向
   `upstream/boost/libs/<lib>/modules/boost_<lib>.cppm`，消费者写 `import boost.<lib>;`。
3. **`BOOST_USE_MODULES` 成为包级配置**：包内每个 TU 都定义它（`[build].defines`），
   并经这 6 个 feature 的 `defines` 传播给消费者。这是上游头文件在"模块已提供实体"时
   自我中和的开关，不是可选项（§4）。
4. **依赖方门面改为 `export import boost.<lib>;`**；生成器把这些库的头文件归属到
   `boost.<lib>`，并从 include 关系补全 implies，保证 `default-features = false` 时依赖闭合。
5. 不保留 `mcpplibs.mwrap.boost.<lib>` 别名。包尚未发布过版本，直接修改，不升 wrapper
   修订号。

第 7 个库 **stacktrace 暂缓**：上游单元把实现放在私有模块片段（`module :private;`）里，
gcc 尚不支持；直接采用会让 gcc 完全无法使用 stacktrace。它保留现有 mwrap 门面，直到
gcc 支持私有模块片段（§6）。

## 2. 范围

| 库 | feature | 模块 | 上游单元 | 自 |
|---|---|---|---|---|
| Any | `any` | `boost.any` | `libs/any/modules/boost_any.cppm` | 1.89 |
| Conversion | `conversion`（新增） | `boost.conversion` | `libs/conversion/modules/boost_conversion.cppm` | 1.92 |
| DLL | `dll` | `boost.dll` | `libs/dll/modules/boost_dll.cppm` | 1.92 |
| LexicalCast | `lexical-cast` | `boost.lexical_cast` | `libs/lexical_cast/modules/boost_lexical_cast.cppm` | 1.91 |
| PFR | `pfr` | `boost.pfr` | `libs/pfr/modules/boost_pfr.cppm` | 1.87（1.89 重写） |
| TypeIndex | `type-index` | `boost.type_index` | `libs/type_index/modules/boost_type_index.cppm` | 1.89 |
| Stacktrace | `stacktrace` | 仍为 `mcpplibs.mwrap.boost.stacktrace` | —（暂缓，§6） | 1.92 |

这些单元已在 pinned 的 `upstream/boost/libs/<lib>/modules/` 下（`import_boost.py` 没有
剪掉 `modules/`），本次不改 vendored 树。

`conversion` 此前不是 feature：`boost::polymorphic_cast` / `polymorphic_downcast` 由
`range.inc` 作为无属主（shared）实体导出，`boost::implicit_cast` 由 `graph.inc` 导出。它现在
有了上游模块，因此作为新 feature 加入，range / graph 改为依赖它。

## 3. 上游模块单元的形状

```cpp
module;
#include <boost/config.hpp>            // 依赖库的头：留在全局模块片段
#include <boost/type_index.hpp>        //   BOOST_USE_MODULES 下展开为 import boost.type_index;
#ifdef BOOST_ANY_USE_STD_MODULE
import std;
#else
#include <memory>                      // 标准库头
#endif
#define BOOST_ANY_INTERFACE_UNIT
export module boost.any;
#include <boost/any.hpp>               // 本库的头：在模块体内，实体归属 boost.any
```

在单元之外，本库头文件开头是：

```cpp
#if defined(BOOST_USE_MODULES) && !defined(BOOST_ANY_INTERFACE_UNIT)
import boost.any;                       // boost/any/detail/config.hpp
#endif
#if !defined(BOOST_USE_MODULES) || defined(BOOST_ANY_INTERFACE_UNIT)
...整份头文件内容...
#endif
```

即：**定义了 `BOOST_USE_MODULES` 时，`#include <boost/any.hpp>` 等价于 `import boost.any;`。**

与 mwrap 门面的根本区别：mwrap 门面把头文件放在 GMF，实体归属全局模块；上游单元把头文件
放在模块体，实体归属 `boost.<lib>`。同一个名字若在一个程序里既以全局模块身份（未定义宏的
文本包含）又以 `boost.<lib>` 身份出现，就是不同 attachment 的重复声明。

## 4. 关键约束：`BOOST_USE_MODULES` 必须全包一致

### 4.1 包内必须定义

`type_index` 被 `variant`、`beast`、`parser`、`yap`、`property_map` 等门面依赖；`any` 被
`program_options`、`property_tree`、`graph`、`geometry`、`contract`、`property_map` 依赖；
`lexical_cast` 被 `math`、`numeric`、`graph`、`program_options`、`property_map` 依赖。
这些门面的 GMF 与它们的库 TU（例如 `libs/program_options/src/*.cpp`）都会 `#include` 上述头。

- 定义宏：这些 `#include` 展开为 `import boost.<lib>;`，整个包里每个实体只有一份。这正是
  上游 CMake 的做法（`target_compile_definitions(... PUBLIC BOOST_USE_MODULES)`）。
- 不定义宏：门面 GMF 里有一份全局模块副本，`boost.<lib>` 里又有一份。

因此放进 `[build].defines`，与 `BOOST_FILESYSTEM_VERSION=3` 等 ABI 宏同列。

1.92.0 中读取 `BOOST_USE_MODULES` 的头文件只属于 any、conversion（含顶层的
`implicit_cast.hpp`、`polymorphic_cast.hpp`、`polymorphic_pointer_cast.hpp`）、dll、
lexical_cast、pfr、stacktrace、type_index，不影响其它库。

### 4.2 还要传播给消费者

mcpp 语义（mcpp `docs/zh/20-toolchains.md`）：`[build].defines` 包私有，不传播；
`[features.<name>].defines` 作用于本包与消费方的每个 TU。所以 6 个 feature 各自再声明
`defines = ["BOOST_USE_MODULES"]`：任一启用，消费者对这些头的原始包含（直接或经其它 Boost 头
间接）也变成 import，与包内一致。

宏是全局开关，由此产生的消费者规则写进 usage.md §2.2：包含这些库的头文件需要启用对应
feature；不得 `#undef` 该宏换回文本头文件。

### 4.3 Stacktrace 必须局部取消

stacktrace 的头文件同样读取该宏：宏下 `boost/stacktrace/safe_dump_to.hpp` 会
`import boost.stacktrace.dump;`，`detail/backend_config.hpp` 会
`import boost.stacktrace.BOOST_STACKTRACE_BACKEND_MODULE;`，其余头文件变空。stacktrace 保留
门面，所以：

- `src/stacktrace.cppm` 在 GMF 开头 `#undef BOOST_USE_MODULES`（GMF 中的宏不会泄漏给导入方）；
- 库 TU 经 `port/stacktrace_basic.cpp`、`port/stacktrace_dump.cpp` 编译，二者 `#undef` 后
  `#include` 上游的 `src/basic.cpp` / `src/dump.cpp`。用包装 TU 而不是按 glob 的
  `-U` 编译选项，是因为 `-D` / `-U` 的先后由 mcpp 拼命令行的顺序决定，包装 TU 不依赖它；
- 没有其它 Boost 库包含 stacktrace 头文件（已 grep 确认），影响只限于它自己；
- 消费者在宏已定义时不能 `#include <boost/stacktrace.hpp>`，只能导入封装模块（usage.md §2.2）。

### 4.4 实测

2026-11-07，本机 clang 23.1.3 + 本仓库 `upstream/boost`，`-DBOOST_USE_MODULES`：预编译上游
`boost.type_index`、`boost.any`；一个 mwrap 风格门面在 GMF `#include <boost/variant.hpp>`、在
模块体 `export import boost.type_index;`；消费者 TU 同时 `#include <boost/any.hpp>`、
`import boost.any;`、导入该门面，构建并运行通过。消费者去掉宏后 clang 23 同样接受，但这属于
标准上"无需诊断"的情形，设计不依赖它，宏照样传播。

## 5. 模块身份与根模块

- feature 名不变（`any`、`type-index`、`lexical-cast`、`pfr`、`dll`），另增 `conversion`。
- 根模块 `import mcpplibs.mwrap.boost;` 仍重导出当前 feature 集合的全部模块，其中这 6 个以
  `export import boost.<lib>;` 出现。
- `generated/meta/modules.lst` 从 `feature<TAB>后缀` 改为 `feature<TAB>完整模块名`，
  `build.mcpp` 直接写 `export import <完整模块名>;`；gcc 的 `sort` 排除改为比较完整名。

## 6. Stacktrace 暂缓的原因与恢复条件

上游 stacktrace 的模块布局：`boost.stacktrace` 通过宏展开的
`export import boost.stacktrace.BOOST_STACKTRACE_BACKEND_MODULE;` 选择后端；`dump`、`basic`、
`windbg`、`addr2line`、`backtrace`、`noop` 每个后端单元都把实现放在 `module :private;` 中，
取代 `src/*.cpp`。gcc 16.1 不支持私有模块片段，而 gcc 是 CI 矩阵中的必选编译器。

恢复条件：gcc 支持私有模块片段之后，将 stacktrace 从
`boost_common.UPSTREAM_MODULE_DEFERRED` 移入 `LIBS_UPSTREAM_MODULE`，届时需要另行处理：

- `boost_stacktrace_basic.cppm` 无条件 `#include <dlfcn.h>`，只能用于 POSIX；Windows 需用
  `windbg` 后端（链接 `dbgeng`、`ole32`）；
- `windbg` 与 `windbg_cached` 两个单元声明同一个模块名，不能 glob `modules/*.cppm`；
- 后端宏只需作用于 `boost_stacktrace.cppm`，用按 glob 的私有 `flags`；
- 不再编译 `src/basic.cpp`、`src/dump.cpp` 与本次新增的两个 port 包装。

## 7. 生成器改动

### 7.1 库分类（`tools/boost_common.py`）

- `FEATURE_LIBS`：全部带模块的 feature 库（原 `TARGET_LIBS` 的定义 + `conversion`）。
- `LIBS_UPSTREAM_MODULE`：lib → 上游模块名；`UPSTREAM_MODULE_UNITS`：lib → 单元路径；
  `UPSTREAM_MODULE_EXTRA_HEADERS`：conversion 在 `boost/` 顶层的三个头。
- `UPSTREAM_MODULE_DEFERRED`：`stacktrace` 及原因。
- `TARGET_LIBS = FEATURE_LIBS - LIBS_UPSTREAM_MODULE`：导出生成器只处理有门面的库。
- `module_name(lib)` / `lib_of_module(name)`：两种模块名的统一入口。

### 7.2 导出归属（`tools/gen_exports.py`）

若只把 6 个库移出 `TARGET_LIBS`，它们的头文件会变成 shared，下次生成时 type_index 的实体会被
variant 认领、`polymorphic_cast` 继续被 range 认领。改为：

- `build_file_to_lib()` 按 `FEATURE_LIBS` 建表，上游模块库的头文件（含 conversion 的顶层头）
  也有属主，`collect_candidates` 等已有的"属主不是本库也不是 shared 就跳过"规则自然生效；
- `emit_inc` 不把属主为上游模块库的签名闭包实体放进本库导出，改记为 `.deps` 边；
- `_lib_of_include()` 识别上游模块库，故门面 GMF 的 include 闭包（`extra_deps`）直接给出
  `boost.<lib>` 边；
- `collect_body_edges` 接受属主为上游模块库的体引用；
- `.deps` 读写、`check_facade` 的 `omit-imports` 统一经 `module_name()` / `lib_of_module()`；
- `export_policy.toml` 删除 `[any]`（typeindex 的 `add` 清单）与三条 `[[dll.guard]]`，并在
  文件头注明：bundle 解析时不定义 `BOOST_USE_MODULES`（否则得到的是 import，AST 看不到声明）。
- **可见性过滤**（`visible_files` / `facade_sees`）：bundle 不带宏解析，看到的文本比真实构建多——
  只经上游模块头才能到达的文件，在真实构建里是 import 的另一侧，门面的 GMF 根本没有包含它。
  首次重新生成后 `math.inc` 导出了 `boost::detail::basic_pointerbuf`（原由 lexical_cast 门面
  认领，`boost/detail/basic_pointerbuf.hpp` 只被 lexical_cast 的头包含），构建在 clang 上报
  "must be declared before it is used"。现在对每个门面用 `clang -E -M -DBOOST_USE_MODULES`
  求出真实 GMF 的文件集合；声明只在集合之外的 Boost 头里的实体，既不导出、也不认领、不产生
  体边，并记入 `skipped.txt`（11 个门面各丢弃一条 `basic_pointerbuf`）。策略 `add` 的记录
  以策略文件为 file，不受此检查；暂缓库（stacktrace）的门面 `#undef` 该宏，不过滤。

### 7.3 features（`tools/gen_features.py`）

- 上游模块 feature：`defines = ["BOOST_USE_MODULES"]`，`sources` = 上游单元，`implies` =
  单元所包含的库（`boost_common.lib_deps` 扫描单元的 include 闭包）。
- 其它 feature 的 `implies` 额外并入"库 TU 包含到的上游模块库"：宏下包含就是导入，哪怕门面的
  导出面没有引用其实体，模块也必须存在。门面 GMF 一侧由 `.deps` 覆盖（§7.2）。
- stacktrace 的 TU 改为两个 port 包装。
- `modules.lst` 写完整模块名。

`[build].defines` 中的 `BOOST_USE_MODULES` 在手写区，不由生成器写。

## 8. 不变量与 `--check`

`gen_features.py --check`（以及写出前）新增断言：

1. **守卫集合 = 上游模块库 ∪ 暂缓库**：`upstream/boost/boost` 中以 `#if`/`#elif` 读取
   `BOOST_USE_MODULES` 的头文件所属库（注释中提到该宏不算），必须恰好是 `LIBS_UPSTREAM_MODULE` 与 `UPSTREAM_MODULE_DEFERRED` 的并集。升级
   Boost 时上游新增模块，这条先失败。
2. **单元存在且模块名匹配**：每个单元文件存在，`export module` 行与表中模块名一致。
3. **没有残留门面**：上游模块库没有 `src/<lib>.cppm`、`generated/exports/<lib>.{inc,deps}`；
   任何 `src/*.cppm` 不得出现 `mcpplibs.mwrap.boost.<上游模块库>`。

2026-10-28 文档 §6 的字节级不变量（上游子树不出现 `mcpp`/`mwrap`）天然成立：单元是上游文件，
本次不改。

## 9. 变更清单

以下是尝试时工作树中的改动，均未提交（§13）。

| 区域 | 改动 |
|---|---|
| `tools/boost_common.py` | §7.1 |
| `tools/gen_exports.py` | §7.2 |
| `tools/gen_features.py` | §7.3、§8；`[scan_overrides]` 生成块、`SCAN_POSIX_IMPORTS`、`PORT_WRAPPED`（§11.4） |
| `tools/export_policy.toml` | 删除 `[any]`、`[[dll.guard]]`；文件头说明 |
| `build.mcpp` | 使用完整模块名 |
| `mcpp.toml` | `[build].defines` 加 `BOOST_USE_MODULES`；头注释；生成块重新生成；新增 `<gen-scan-overrides>` 块 |
| `src/` | 删除 `any`、`dll`、`lexical_cast`、`pfr`、`type_index` 五个门面；依赖方 `export import` 改为 `boost.<lib>`；`stacktrace.cppm` `#undef BOOST_USE_MODULES` |
| `port/` | 新增 `stacktrace_basic.cpp`、`stacktrace_dump.cpp`、`log_permissions.cpp`（§11.4） |
| `.xlings.json`、`tests.yml`、usage.md §8 | mcpp 固定版本 2026.11.5.1 → 2026.11.10.2 |
| `patches/` | 新增 `msm.patch`：`msm/back/state_machine.hpp` 使用 `boost::typeindex::type_id` 却只经 `<boost/any.hpp>` 间接得到；宏下该包含是 `import boost.any;`，不重导出 `boost.type_index`（上游模块模式同样有此缺口）。`tools/apply_patches.py` 登记 |
| `generated/` | 删除五组 `.inc/.deps`；重新生成依赖方 `.inc/.deps` 与 meta |
| `tests/` | 5 个测试改 `import boost.<lib>;`；新增 `conversion.cpp`、`upstream_modules_mixed.cpp`（原始 include + 上游模块 + 门面同 TU，并断言宏已传到消费者） |
| 文档 | usage.md / .zh §2.2"上游已提供模块的库"（含 stacktrace 例外）、§5 ABI 宏、§7 迁移表；architecture.md / .zh §3.1、§5.4、§7 |

## 10. 风险

- **gcc 16.1 对上游单元的支持未经本机验证**（本机无 gcc 16）。**已实测，见 §12：不成立。**stacktrace 已因私有模块片段排除；
  其余 6 个单元不含私有模块片段。若某个单元在 gcc 上失败，处理方式是在 `build.mcpp` 中按编译器
  报错说明（同 `sort` 的做法），而不是恢复 mwrap 门面。
- **宏是全局开关。** 消费者启用 `type-index` 而未启用 `pfr`，却 `#include <boost/pfr.hpp>`，会
  得到 `import boost.pfr;` 而模块不存在。这是上游语义，usage.md 写明。
- **`import std` 混用。** 测试使用 `import std;`，上游单元 GMF 用文本标准库头（未定义
  `BOOST_<LIB>_USE_STD_MODULE`），与现有门面同构，不引入新的组合。
- **mcpp 扫描器。** 原为阻塞项，mcpp 2026.11.10.1 修复后由生成的 `scan_overrides` 解决（§11.4）。
  声明是单份、跨平台的：macOS 与 clang-cl 下的导入集合未经本机预处理，只由 CI 暴露。
- **仅 Windows 的头文件分支。** implies 由正则 include 扫描得出，条件编译取并集；上游单元在
  Windows 分支若需要额外的库，会在 windows-llvm-msvc CI 腿上暴露。

## 11. mcpp 的模块扫描（原阻塞项，已解决）

### 11.1 现象

mcpp 2026.11.5.1 的默认扫描器（`src/modgraph/scanner.cppm`）逐文件做文本扫描，不展开 `#include`：

1. 拒绝 `#if` 块内的 `import`（"forbidden in M1"）。6 个上游单元都有
   `#ifdef BOOST_<LIB>_USE_STD_MODULE import std; #endif`，`mcpp test` 在扫描阶段即失败。
2. 头文件在 `BOOST_USE_MODULES` 下产生的 `import boost.<lib>;` 对它不可见。门面有显式的
   `export import boost.<lib>;`，不受影响；只经头文件导入的翻译单元没有依赖边——包括上游单元
   自身（`boost.any`、`boost.dll` 的 GMF 经头文件导入 `boost.type_index`），以及 graph（2 个）、
   log（38 个）、program_options（7 个）的库 TU。

`MCPP_SCANNER=p1689` 走编译器扫描，但只传 GCC 的 `-fdeps-*` 选项，clang 报 `unknown argument`。

### 11.2 `scan_overrides` 实验

`[scan_overrides."<glob>"]` 让被匹配的文件跳过文本扫描，由清单声明 `provides` / `imports`；在
GCC 上 mcpp 用编译器自己的 P1689 输出逐 TU 审计，要求集合**完全相等**。导入集合用
`clang -E`（包的宏）求出，clang 把头文件里的导入输出为 `__preprocessed_import <name>;`：

| 文件 | imports |
|---|---|
| `boost_any.cppm`、`boost_dll.cppm` | `boost.type_index` |
| 另外 4 个单元 | 无 |
| program_options 7 个 TU | `boost.any`、`boost.lexical_cast` |
| log 38 个 TU | `boost.type_index`（部分另有 `boost.any` / `boost.lexical_cast`） |
| graph 2 个 TU | `boost.any`、`boost.lexical_cast`、`boost.type_index`（`read_graphviz_new` 另有 `boost.conversion`） |

把这 6 + 47 条声明追加到 `mcpp.toml` 后（log 的 posix/windows 同名文件合并为
`log/src/*/<name>.cpp`），`mcpp test --toolchain llvm@23.1.3` 通过扫描并完成全量构建：
**145 通过、1 失败**；失败的 msm 是上游缺 include（§9 `msm.patch`），修复后 **146/146 通过**。
同一次构建也验证了 §7.2 的可见性过滤与全部依赖方门面。

但 `scan_overrides` 不能直接落地，原因是它的一条规则：**匹配不到任何源文件的声明是硬错误**。

- `mcpp build` 只编译启用 feature 的源文件。默认 feature 下 `mcpp build` 报 program_options 等
  7 条声明 "matched no source file"；pfr、dll、conversion、graph、log 未启用时同理。
- `[target.*]` 排除的平台目录同样触发：`log/src/windows/mapped_shared_memory.cpp` 在 Linux 上不存在
  对应文件。
- 声明只能写在顶层 `[scan_overrides]`，没有按 feature 或 target 的形式。

实验用的声明没有提交；`mcpp.toml` 保持不含 `scan_overrides`。

### 11.3 出路

1. **mcpp 小改（推荐）**：声明匹配到的文件存在于磁盘、只是被 feature 或 target 从本次源文件集合
   排除时，跳过（或像 `[build].flags` 的零命中 glob 一样只警告），而不是报错。改完后本仓库由
   `gen_features.py` 用 `clang -E` 生成 `scan_overrides` 块（作为第三个生成块，`--check` 校验），
   其余改动不变。长期看，P1689 扫描支持 clang 才能覆盖消费者 TU 里头文件带来的导入。
2. **本仓库绕行（不改 mcpp）**：经 `patches/` 给 6 个单元删去条件 `import std` 并补显式
   `import`；为 47 个库 TU 生成带显式 `import` 的 port 包装并替换 feature 源文件。改动上游单元，
   且包装要逐文件维护。

无论哪条路，消费者 TU 里经头文件产生的导入都看不到：usage.md §2.2 要求消费者显式 `import`。

### 11.4 解决（2026-11-18）

mcpp 2026.11.10.1 按出路 1 修复（[mcpp#902](https://github.com/mcpp-community/mcpp/issues/902)）：
声明匹配的文件存在、只是被未启用的 feature 或 `[target.*]` 的 `!` 规则排除时，条目保留；只有
磁盘上确实没有匹配文件时才报错。固定版本升到 2026.11.10.2。

`gen_features.py` 生成第三个块 `<gen-scan-overrides>`（`[scan_overrides]` 内联表，57 条），
写出前与 `--check` 都重新求值：

- 范围：所有 feature 与 profile 的源文件（展开 glob），门面 `src/*.cppm` 除外（导入都写在文本里）。
  上游单元总有一条（`provides` = 模块名）；其它文件在预处理后有导入时才有一条。
- 求值：`clang++ -E`，带 `[build]` 的 include 目录与 defines、目标表的 defines、匹配的按 glob
  flags；导入取 `import` 与 `__preprocessed_import` 行。用生成器的 MinGW 目标与 sysroot，三种
  主机得到同一份结果（verify-scripts 在 ubuntu / windows / macos 上都跑 `--check`）。

**平台差异。** mcpp 只在 gcc（dyndep 模式）上用编译器的 P1689 结果核对声明，且要求
`provides`、`imports` 两个集合**完全相等**（`mcpp::dyndep::verify_unit_expectations`）；gcc 只在
Linux 腿上。clang 不核对：多声明的导入只是一条多余的边，少声明则缺边。逐文件比较 Linux 原生
与 MinGW 的预处理，log 有 4 个文件不同：

| 文件 | POSIX | MinGW | 处理 |
|---|---|---|---|
| `thread_id.cpp`、`timestamp.cpp` | `boost.type_index` | 无 | 声明 POSIX 集合（Windows 多一条边） |
| `posix/object_name.cpp` | `boost.type_index` | 无法预处理（`<pwd.h>`） | 声明 POSIX 集合 |
| `permissions.cpp` | 无 | `boost.type_index` | 无法用单条声明表达 → `port/log_permissions.cpp` |

差异都来自 `#if defined(BOOST_WINDOWS)` 分支是否包含 `<boost/log/exceptions.hpp>`。前三个写在
`SCAN_POSIX_IMPORTS`；生成器断言其 MinGW 集合是 POSIX 集合的子集（仅 POSIX 的文件除外）。
`permissions.cpp` 在 Windows 上导入更多，经 `PORT_WRAPPED` 换成包装 TU：包装先
`import boost.type_index;` 再包含上游文件，两个平台的集合都是 `{boost.type_index}`。

`SCAN_POSIX_IMPORTS` 是手写表，有两道核对：Linux 主机上生成器还会对每个非 Windows 文件做原生
预处理，与声明不同即报错；linux-gcc 腿的 P1689 核对兜底。macOS（libc++）与 clang-cl（MSVC STL）
的集合没有本机预处理手段，若比声明多，表现为 macos-llvm / windows-llvm-msvc 腿上缺模块。

消费者 TU 里经头文件产生的导入仍然看不到，usage.md §2.2 的显式 `import` 规则不变。

### 11.5 实测（2026-11-18，mcpp 2026.11.10.2）

- `mcpp build --toolchain llvm@23.1.3`（默认 feature）：通过，不再报 "matched no source file"。
- `mcpp test --toolchain llvm@23.1.3`：**146/146 通过**。
- `mcpp test --toolchain gcc@16.1.0`：57 条声明全部通过 P1689 核对（没有 "module-graph
  divergence"），但编译失败，见 §12。
- `gen_features.py --check` 通过；`tools/tests` 40 个用例通过。

## 12. 新阻塞项：gcc 16.1 不支持"导入之后再文本包含"

`ninja -k 0` 下 gcc 共 42 个编译失败：

| 类别 | TU |
|---|---|
| 上游单元 | `boost_any.cppm`、`boost_dll.cppm` |
| 门面 | `variant`、`multiprecision`、`yap`、`statechart`、`wave`、`process` |
| log 库 TU | 31 个（含 `port/log_permissions.cpp`） |
| 测试 | `spirit`、`log`、`wave` |

报错都是同一类：`conflicting declaration of template 'std::reference_wrapper'`、
`redefinition of 'class std::__cxx11::basic_stringbuf'`、`conflicting declaration of template
'boost::hash'`、`boost::mp11::mp_list` 等。原因：在 `BOOST_USE_MODULES` 下，头文件在包含序列
中间变成 `import boost.type_index;`（或 any / lexical_cast），之后翻译单元又文本包含了该模块
GMF 里已有的 std 与 Boost 头文件。gcc 16.1 不能把后来的文本定义与导入的 GMF 实体合并
（"先 import、后 #include"），clang 可以。门面同样受影响：它们的 GMF 经 Boost 头文件导入上游模块，
之后再包含其它头文件。

这与 `scan_overrides` 无关：§11 修复之前 gcc 在扫描阶段就失败，没有走到编译。

gcc 是 CI 必选编译器（linux-gcc x86_64 / arm64）。可能的方向（待决定）：

1. **gcc 下保留 mwrap 门面**：按编译器切换实现。`BOOST_USE_MODULES` 只在 clang 下定义；gcc 下
   6 个库仍由 mwrap 门面提供。代价是两套模块名（`boost.<lib>` / `mcpplibs.mwrap.boost.<lib>`），
   消费者代码随编译器不同，`.inc` / `.deps` 也要两份。
2. **整体暂缓**：6 个库都像 stacktrace 一样回到 mwrap 门面，直到 gcc 支持"导入后包含"；
   §7 的生成器改动与 `scan_overrides` 机制保留，恢复时直接启用。
3. **标准库改用 `import std;`**：std 实体不再出现在 GMF 里，可消除 std 部分的冲突；要求全包与
   上游单元（`BOOST_<LIB>_USE_STD_MODULE`）都改用 `import std`。Boost 自身实体（`boost::hash`、
   `mp11`）的冲突仍在，所以单靠它不够。范围最大，未验证。

## 13. 2026-11-18 这一轮：过程与结果

起点：§1–§10 的改动已在工作树中（未提交），唯一阻塞是 §11.2 的 `scan_overrides`
"matched no source file"。mcpp#902 报告该问题，mcpp 2026.11.10.1 修复。

过程：

1. 安装 mcpp 2026.11.10.2，固定版本随之更新（`.xlings.json`、`tests.yml`、usage.md §8）。
2. 决定按 §11.3 出路 1 由生成器产出声明。`gen_features.py --check` 在 verify-scripts 的
   ubuntu / windows / macos 三腿都跑，所以导入集合只能用与主机无关的 MinGW 目标求值
   （与 `gen_exports.py` 相同）。
3. 对 262 个包内 TU 分别做 Linux 原生与 MinGW 预处理比较，发现 log 的 4 个文件导入集合随平台
   变化（§11.4 表）。
4. 读 mcpp 2026.11.10.2 源码确认核对语义：仅 gcc（dyndep 模式）核对，`provides` / `imports`
   要求完全相等；clang 不核对（§11.4）。由此定出规则：声明 POSIX 集合，Windows 集合须为其子集；
   不满足的 `permissions.cpp` 换成 `port/log_permissions.cpp`。
5. 实现 `<gen-scan-overrides>` 生成块、`SCAN_POSIX_IMPORTS`、`PORT_WRAPPED`；Linux 主机上追加
   原生预处理核对。生成 57 条声明，原生核对无差异。
6. clang：默认 feature 的 `mcpp build` 与全量 `mcpp test` 通过（146/146）。
7. gcc：声明全部通过 P1689 核对，但编译失败；用 mcpp 生成的 `test.ninja` 跑 `ninja -k 0` 得到
   完整的 42 个失败（§12）。

结果：

- `scan_overrides` 问题已解决，机制在 gcc 与 clang 上都成立。
- 原计划（§1）**在 gcc 16.1 上不可行**：问题在 `BOOST_USE_MODULES` 让头文件在包含序列中间
  变成导入，不在扫描。§10 第一条风险原本预计"个别单元在 gcc 上失败，按编译器在
  `build.mcpp` 中说明"，实际影响 2 个上游单元、6 个门面、31 个 log TU 与 3 个测试，不能按个案处理。
- gcc 是 CI 必选编译器，§12 的三个方向都不能在本轮内完成，因此**放弃本次尝试**：代码改动全部
  丢弃，仓库保持原样（6 个库仍是 mwrap 门面，mcpp 固定版本仍为 2026.11.5.1），只提交本记录。
- 未验证：macOS（libc++）与 clang-cl（MSVC STL）下的导入集合，以及 CI 的 llvm 22.1.8
  （本机测的是 23.1.3）。

重新考虑时：

- 前提是 gcc（CI 固定版本）能合并"导入之后再文本包含"的重复声明；可以先用本记录 §12 的失败
  清单中任一 log TU（如 `libs/log/src/trivial.cpp`）在 `-DBOOST_USE_MODULES` 下验证。
- 实现按 §7、§8 与 §11.4 重做：§11.4 的 `scan_overrides` 生成规则（MinGW 求值、
  `SCAN_POSIX_IMPORTS`、`permissions.cpp` 包装）不依赖 gcc 的结论，可以直接沿用；mcpp 需不低于
  2026.11.10.1。
- 届时 stacktrace 是否仍需暂缓，按 §6 的恢复条件另行判断。

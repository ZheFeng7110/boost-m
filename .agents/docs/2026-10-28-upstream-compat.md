# 与 Boost 上游兼容

> 同一个仓库如何今天作为 `mwrap` 包发布，日后又能把模块层贡献进 Boost 上游，
> 而不维护两份模块层。
>
> 相关：[architecture.md](../../docs/architecture.md) ·
> [mwrap 规范](https://github.com/mcpplibs/mcpp-index/blob/main/docs/mwrap.md) ·
> [Boost 库要求与准则](https://www.boost.org/development/requirements.html)

## 1. 结论

两套规范在**代码**上并不冲突。双方要的是同一件事：上游头文件、上游名字、上游
命名空间、上游类型身份一律不动，模块层只负责"重新公布"它们。

冲突集中在四件**打包**上的事：

| | mwrap | Boost |
|---|---|---|
| 身份 | `mcpplibs.mwrap.boost.<lib>` | `boost.<lib>` |
| 版本与 tag | 上游版本 + 至多一个数字修订，`mwrap-1.93.0` | 发布列车 `1.93.0`，每个库的 tag `boost-1.93.0` |
| 许可证 | 封装层贡献用 Apache-2.0 | BSL-1.0 |
| 构建与测试 | mcpp、`tests/`、模板、索引 | b2 + CMake、`test/`、禁止外部依赖 |

因此办法是**单向拆分，而不是合并**：

1. **可以贡献出去的模块接口按上游形状编写**，并且自包含：纯 C++20、
   `export module boost.<lib>;`、BSL-1.0、只靠 CMake 就能构建、零非 Boost 依赖。
   这棵子树（`bridge/`）可以逐字节搬进 `boostorg/<lib>`。若某个库暂时接不了，
   封装层就保留一份清单门面——那份门面永远不往上提。
2. **mwrap 专有的东西全部是薄薄的生成别名**：`export module
   mcpplibs.mwrap.boost.<lib>; export import boost.<lib>;`，外加清单、版本、tag、
   索引描述、模板、测试与 README。
3. **一条字节级不变量决定什么能过界**：上游子树里任何文件都不许出现 `mcpp`、
   `mcpplibs`、`mwrap` 或 mwrap 模块名。正是这条规则让拆分长期成立。

因为别名只有三行，而上游单元里的模块名始终不变，所以从"只有封装层"到"上游自己
发模块"的整个迁移过程中，mwrap 的 import 名字保持稳定。

## 2. Boost 上游到底要求什么

依据顺序：[requirements](https://www.boost.org/development/requirements.html) ·
[submissions](https://www.boost.org/development/submissions.html) ·
[boost-ci](https://github.com/boostorg/boost-ci) ·
[CMake 指南](https://github.com/boostorg/boost-ci/blob/master/CMake.md)。

### 2.1 硬性要求（不满足直接拒收）

- **许可证**：允许免费复制/使用/修改；要求在源码副本上保留许可证；不得要求二进制
  附带许可证；不得要求为二进制使用提供源码。首选 BSL-1.0；GPL/LGPL 不可接受。
- **版权**：归属清晰，所有重要文件带版权声明。
- **通用性**：对足够广泛的用户有用。
- **可移植**：接口可移植；至少两个实现当前 ISO 标准的 C++ 编译器能构建并运行，
  实践中即两个操作系统。`boost/config.hpp` 是官方认可的条件编译机制。没有义务支持
  不符合标准的编译器。
- **禁止外部依赖**："不要使用 C++ 标准库与 Boost 之外的库。" 包管理器、生成器
  运行时、vendored 第三方库都不能成为 Boost 库的构建前提。
- **作者愿意参与邮件列表讨论并维护**。

### 2.2 组织（接收后强制）

`build/`（Jamfile）、`config/`、`doc/` + `doc/html/`、`index.html` 重定向、
`example/`、`include/boost/<lib>/`、`meta/libraries.json`、`src/`、`test/`、
`tools/`。

`test/` 是**自动化测试唯一被考虑的位置**，其他地方必须由它引用
（`build-project`）。`meta/libraries.json` 包含 `key`、`name`、`authors`、
`maintainers`、`description`、`category`、`cxxstd`、`documentation`、`sublibs`。

### 2.3 集成（接收后强制）

- **b2**：`build/Jamfile` 里 `project boost/<lib> ;`、库目标与 `boost-install`；
  在 Boost 根目录执行 `b2 libs/<lib>/test` 必须可用。
- **CMake**：`CMakeLists.txt` 用 `boost_install`，产出 `Boost::<lib>` 目标，并有
  `test/cmake_test` / `test/cmake_subdir_test` 覆盖安装与子目录消费。
- **文档**：`doc/` 必须能用 Boost Build 构建，提供 `boostdoc` 别名，默认构建独立
  文档；库根目录的 `index.html` 重定向到 `doc/html/index.html`。
- **CI**：使用 boost-ci 的 `.github/workflows/ci.yml`；覆盖率/Coverity 可选。

### 2.4 模块层会撞上的准则

- **向后兼容**：非破坏性改动自由；小破坏性改动要提前几个版本通知；大改动需要并行
  的目录/命名空间与弃用路径。删除库需要更长周期。模块单元是增量改动，属于非破坏性
  ——但它不得改变头文件路径的含义。
- **命名**：小写加下划线，缩写当普通单词；宏全大写且带 `BOOST_` 前缀；模板参数
  首字母大写。
- **文件名**：小写字母、数字、`_`、`-`、`.`；目录名不含点；每段 ≤31 字符；
  全路径 ≤207 字符；C++ 翻译单元用三个字母、以 `pp` 结尾的扩展名。
- **源码卫生**：空格不用 Tab；80 列；每个文件以描述行、版权/许可证块，以及
  `// See https://www.boost.org/libs/<lib> for library home page.` 开头。
- **头文件要做好邻居**：自包含、不依赖包含顺序；用 `BOOST_ASSERT` 而非 `assert`；
  防范 `min`/`max` 宏。

### 2.5 要求文档里没写的那部分：模块

Boost 已经有一个被接收的 in-tree C++20 模块模式，而且是唯一应当照抄的模式。
`boostorg/any` 与 `boostorg/type_index` 已经这么做了：

```text
modules/boost_any.cppm
```

```cpp
module;
#include <boost/config.hpp>          // 全局模块片段
#ifdef BOOST_ANY_USE_STD_MODULE
import std;
#else
#include <memory>                    // ...
#endif
#define BOOST_ANY_INTERFACE_UNIT
export module boost.any;
#ifdef __clang__
#  pragma clang diagnostic ignored "-Winclude-angled-in-module-purview"
#endif
#include <boost/any.hpp>             // 在模块体内
```

`CMakeLists.txt` 中：

```cmake
if(BOOST_USE_MODULES)
    add_library(boost_any STATIC)
    target_sources(boost_any PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS modules FILES modules/boost_any.cppm)
    target_compile_features(boost_any PUBLIC cxx_std_20)
    target_compile_definitions(boost_any PUBLIC BOOST_USE_MODULES)
    ...
endif()
```

公开头文件里则加一道守卫，使模块提供实体时头文件变成空操作：

```cpp
#if !defined(BOOST_USE_MODULES) || defined(BOOST_ANY_INTERFACE_UNIT)
...
#endif
```

由此得到五条支配下文的结论：

1. **模块单元是 opt-in**（`BOOST_USE_MODULES`，默认关闭）。b2 永不构建它；其他人
   的头文件路径完全不变。
2. **`meta/libraries.json` 不动**——`any` 至今仍写 `"cxxstd": "11"`。模块单元
   不抬高库的语言基线。
3. **钩子放在库自己的头文件里**（`BOOST_<LIB>_INTERFACE_UNIT`、
   `BOOST_<LIB>_BEGIN/END_MODULE_EXPORT`），因为单元是在**模块体**里包含头文件的：
   声明归属 `boost.<lib>`，所以所有头文件消费者必须被 `BOOST_USE_MODULES` 中和。
4. **`import std` 可选**，单独开关（`BOOST_<LIB>_USE_STD_MODULE`、`cxx_std_23`）。
5. **上游化一个模块单元是"每个库的改动"，不是新库**：只需要该库维护者同意，不需要
   正式评审。只有**新的** Boost 库（例如假想的 "Boost.Modules"）才走评审流程——
   联署、评审经理、正式评审，周期以年计。

同时要正视：模块化在上游并不是已解决问题。Rubén Pérez 2025 年的原型
（[anarthal.github.io/cppblog/modules](https://anarthal.github.io/cppblog/modules)）
在与维护者讨论后被搁置，主因是 MSVC 与 `import std` 工具链成熟度。所以贡献必须是
严格增量、不得成为任何东西的前置条件。

## 3. mwrap 要求了什么

只列约束上游交付的部分：

| 方面 | 规则 |
|---|---|
| 仓库 | `<upstream-name>-m` |
| 包 | 命名空间 `mcpplibs.mwrap`，短名 = 上游库名，单一原子段 |
| 模块 | 根模块 = 完整包标识；子模块 = 上游模块分组 |
| 公开 API | 原始上游拼写、命名空间、类型身份、重载；优先 `export using`；每个实体只有一个导出属主 |
| 版本 | 跟上游版本，最多一个数字修订；tag 系列 `mwrap-${version}` |
| 布局 | `mcpp.toml`、`.xlings.json`、`upstream/`、`patches/`、`port/`、`src/`、`generated/`、`templates/`、`tests/`、`tools/` |
| 宏 | `<root>.macros` 模块映射对象常量，外加 `<mwrap/<lib>/macros.hpp>` 在 import 前包含 |
| 特性 | 小写 kebab-case、可叠加、`implies`、不可用目标显式报错 |
| README | 英文 `README.md` + `README.zh.md`，固定导航行，Modules 在 Getting started 之前 |
| 模板 | 至少一个可运行的默认 `templates/<name>/template.toml` |
| 测试 | API、行为、头文件/模块、已安装消费者，都在 `tests/` |
| 索引 | `pkgs/mwrap/<lib>.lua`，`namespace = "mcpplibs.mwrap"` |
| 许可证 | 封装层 Apache-2.0，`mcpp-community` 署名，保留上游文档 |

其中两条对拆分至关重要：*"保留上游拼写与命名空间"* 恰好就是 Boost 要的；*"每个
实体只有一个导出属主"* 正是别名合法的原因——实体属于上游单元，绝不被导出两次。

## 4. 冲突矩阵

| # | 维度 | mwrap | Boost | 处理 |
|---|---|---|---|---|
| 1 | 仓库 | `boost-m` | `boostorg/<lib>` | 封装层仓库持有 `bridge/`；PR 的载荷只有 `bridge/`，绝不是整个仓库 |
| 2 | 许可证 | Apache-2.0 | BSL-1.0 | 按目录**单**许可：`bridge/` BSL-1.0，胶水 Apache-2.0；根 `LICENSE` + `NOTICE` 说明范围；`upstream/` 保留上游原字节；不接受二选一表达式（§5.8） |
| 3 | 命名空间 | 上游 `boost::` | 上游 `boost::` | **一致** |
| 4 | 模块名 | `mcpplibs.mwrap.boost.<lib>` | `boost.<lib>` | 生成别名：`export import boost.<lib>;` |
| 5 | 版本 | `1.93.0[.N]`，tag `mwrap-1.93.0` | `1.93.0`，tag `boost-1.93.0` | 映射表 + `[package.metadata.mwrap]`；Boost 的 `-N` 重发映射为封装修订并逐字记录 |
| 6 | 布局 | `src/`、`tests/`、`upstream/`、`port/`、`templates/`、`tools/` | `include/`、`meta/`、`doc/`、`test/`、`build/`、`example/`、`src/` | 除 `src/` 外基本不相交；bridge 用 `modules/`；测试只留一个 `test/` 入口 |
| 7 | 构建 | mcpp | b2 + CMake | `bridge/` 对 b2 不可见，由 `BOOST_USE_MODULES` 在 CMake 中开关 |
| 8 | 语言基线 | 包默认 C++23 | 每个库各自，常见 C++11/14/17 | 模块单元需要 C++20 且保持 opt-in；不抬高 `meta/libraries.json` 的 `cxxstd` |
| 9 | 宏 | `.macros` 模块 + 兼容头 | 宏仍是宏，外加 `BOOST_USE_MODULES` 守卫 | 宏映射是 mwrap 专有；上游只拿守卫，不拿映射 |
| 10 | 依赖 | `compat.boost` 提供方 | 除 std 与 Boost 外无库 | bridge 零依赖；提供方只存在于封装层 |
| 11 | 测试 | `tests/` | `test/`（唯一自动化位置） | 源码留在 `tests/`；`test/Jamfile` 与 `test/CMakeLists.txt` 引用它 |
| 12 | 文档 | 根 README 布局，中英双语，100–150 行 | `doc/` HTML + `doc/html` + 重定向 | 根 README 走 mwrap 形状；`doc/` 走 Boost 形状；PR 只带 `doc/` |
| 13 | CI | 三个 mcpp 作业 + 模板 + 已安装消费者 | boost-ci 矩阵，2 编译器 × 2 OS | 两者都保留；新增 `bridge-ci` 作业把 `bridge/` 覆盖进临时 Boost 根 |
| 14 | 评审 | 索引 PR | 每库维护者同意；只有新库才正式评审 | 先做每库 PR；只有确实需要元层时才提新库 |
| 15 | 署名 | `mcpp-community` 标签 | 逐文件版权 + `meta/libraries.json` 维护者 | 逐文件版权；标签留在 `mcpp.toml` 与 NOTICE |
| 16 | 配置/ABI | 记录 ABI、运行时、CPU、SDK 约束 | 向后兼容政策 | 每次模块构建固定一套宏，写进文档并测试 |

## 5. 设计

### 5.1 三层，一个事实来源

```text
                    上游头文件（Boost，按发布版本 pin）
                                    │
              ┌─────────────────────┴─────────────────────┐
              ▼                                           ▼
   bridge/modules/boost_<lib>.cppm            generated/<lib>-exports.inc
   export module boost.<lib>;                 export namespace boost { ... }
   （上游形状，BSL，无 mcpp）                  （由 AST 生成的名字清单）
              │                                           │
              ▼                                           ▼
   src/<lib>.cppm  ── export import ──►        src/<lib>.cppm
   mcpplibs.mwrap.boost.<lib>                  mcpplibs.mwrap.boost.<lib>
   （B 档：上游已有单元）                       （A 档：只有封装层）
```

**A 档 —— 清单门面**（今天对所有 Boost 库都成立，且不需要任何上游改动）。上游
头文件在**全局模块片段**里包含，实体仍属于全局模块；模块体重新打开每个上游命名
空间，为每个审计过的实体加一条 `using` 声明。这正是 `onnxruntime-m` 与
`llama.cpp-m` 已验证的设计，也是唯一能让头文件与模块在同一翻译单元里共存的设计。

**B 档 —— 上游单元**（Boost.Any 形状）。头文件在模块体内包含，头文件里有
`BOOST_USE_MODULES` / `BOOST_<LIB>_INTERFACE_UNIT` 守卫，声明被
`BOOST_<LIB>_BEGIN/END_MODULE_EXPORT` 包住。构造上就是完全保真的，但它禁止原始
头文件与模块混用，所以只有在守卫已经进入该库之后才是正确的。

两档最终落到同一个公开名字。封装层默认 A 档；一旦某个库在上游发布了 B 档单元，
封装层就翻成三行别名：

```cpp
// src/asio.cppm —— 上游发布 modules/boost_asio.cppm 之后
export module mcpplibs.mwrap.boost.asio;
export import boost.asio;
```

消费者零改动、清单无需重新审计，实体依旧只有一个导出属主。

### 5.2 仓库布局

```text
boost-m/
  mcpp.toml                  # 包身份、特性、提供方依赖
  .xlings.json               # 精确的 mcpp pin
  LICENSE  NOTICE            # 封装层 Apache-2.0 + 范围声明
  bridge/                    # ← 唯一允许搬去上游的子树
    LICENSE_1_0.txt          # BSL-1.0
    modules/boost_asio.cppm  # B 档单元，`export module boost.asio;`
    modules/boost_json.cppm
    test/                    # 模块测试，纯 C++，不用 mcpp
    doc/                     # 给该库 PR 用的 AsciiDoc/Quickbook 片段
    upstream.meta.toml       # 目标库、维护者、每库 PR 状态
  src/boost.cppm             # mwrap 根模块：export import 已启用闭包
  src/asio.cppm              # A 档门面，或 B 档别名
  src/macros.cppm            # mcpplibs.mwrap.boost.macros
  generated/asio.inc         # 清单（仅 A 档），字节稳定
  port/include/mwrap/boost/macros.hpp
  tests/                     # API、行为、头文件/模块混用、已安装消费者
  tools/gen_exports.py       # AST → 清单，支持 --check
  tools/gen_alias.py         # 清单 → src/<lib>.cppm 门面，支持 --check
  tools/bridge_into_boost.py # 把 bridge/ 覆盖到临时 Boost 根
  templates/                 # 必需的默认可运行模板
  upstream/                  # pin 住的 Boost 文档、校验和、许可证
```

`bridge/` 按库可选，它的缺失就是这个库诚实的现状："封装层有了，上游单元还没有"。

### 5.3 清单

沿用现有 AST 流水线生成，每个上游命名空间一块，排序、无时间戳、可 `--check`：

```cpp
// generated/asio.inc
export namespace boost { namespace asio {
using ::boost::asio::io_context;
using ::boost::asio::steady_timer;
} }
export namespace boost { namespace system {
using ::boost::system::error_code;
} }
```

生成器必须强制的三条规则，每条背后都有真实故障：

- **必须重新打开命名空间，不要在全局作用域导出。** 裸写
  `export using ::boost::system::error_code;` 只导出**非限定**名字，
  消费者写 `boost::system::error_code` 仍然不可见。
- **命名空间作用域的函数与运算符必须显式列出。** 类内的 hidden friend 由 ADL
  找到，无需列出；而 `operator==(optional<T> const&, none_t)` 这类自由运算符
  不会被找到，漏掉就会让消费者在 `opt != boost::none` 上报
  "no match for operator!=" ——而对着头文件编译是好的。
- **被跳过的名字必须计数。** 每个未导出的公开名字（内部链接、成员模板、无法映射
  的宏）都进入机器校验的跳过清单，让"缺失"永远是一个决定，而不是意外。

### 5.4 上游单元

```cpp
// bridge/modules/boost_optional.cppm
module;
#include <boost/config.hpp>
#ifdef BOOST_OPTIONAL_USE_STD_MODULE
import std;
#else
#include <type_traits>
#endif
#define BOOST_OPTIONAL_INTERFACE_UNIT
export module boost.optional;
#include <boost/optional.hpp>   // 模块体，在守卫之后
```

头文件一侧：

```cpp
#if !defined(BOOST_USE_MODULES) || defined(BOOST_OPTIONAL_INTERFACE_UNIT)
#define BOOST_OPTIONAL_BEGIN_MODULE_EXPORT export {
#define BOOST_OPTIONAL_END_MODULE_EXPORT }
...
#endif
```

CMake 一侧照抄 `boostorg/any`：

```cmake
option(BOOST_USE_MODULES "Build and use C++20 modules" OFF)
if(BOOST_USE_MODULES)
  add_library(boost_optional STATIC)
  target_sources(boost_optional PUBLIC FILE_SET CXX_MODULES
      BASE_DIRS modules FILES modules/boost_optional.cppm)
  target_compile_features(boost_optional PUBLIC cxx_std_20)
  target_compile_definitions(boost_optional PUBLIC BOOST_USE_MODULES)
endif()
```

每库一个 `.cppm` 加一段头文件钩子补丁，**就是**那个 PR。它小、可评审、贴合已被
接收的先例，并且对从不设置 `BOOST_USE_MODULES` 的用户零影响。

### 5.5 版本、tag、元数据

```toml
[package]
namespace = "mcpplibs.mwrap"
name      = "boost"
version   = "1.93.0"

[package.metadata.mwrap]
upstream         = "boost"
upstream_release = "1.93.0"          # Boost 自己的写法，逐字记录
upstream_tag     = "boost-1.93.0"    # boostorg/<lib> 里每个库的 tag
libs             = { asio = "<sha>", optional = "<sha>" }  # 发布版本内的 pin
```

映射规则必须写下来，因为 Boost 的编号有一个 mwrap 没有的情形：

| Boost | mwrap | 说明 |
|---|---|---|
| `1.93.0` | `1.93.0`，tag `mwrap-1.93.0` | 该身份的首发，不带修订 |
| `1.93.0` + 封装层修复 | `1.93.0.1` | 封装修订 |
| `1.91.0-1`（重发） | `1.91.0.1`，`upstream_release = "1.91.0-1"` | 连字符形式不可排序，逐字记录且绝不当作版本号；上游重发占用第一个修订槽 |

封装层 tag 系列是 `mwrap-${version}`；自动更新必须跟随 `boostorg/boost` 的发布
tag 以及各库的 `boost-1.93.0` tag，绝不能跟 GitHub "latest"。

### 5.6 构建、测试、CI

`test/` 只作为入口存在，让两套工具链都在它们期望的位置找到测试：

```make
# test/Jamfile —— Boost 看到 libs/<lib>/test，源码只有一份
build-project ../tests ;
```

```cmake
# test/CMakeLists.txt
add_subdirectory(../tests tests)
```

- **Boost 侧**：`bridge-ci` 用 `tools/bridge_into_boost.py` 把 `bridge/` 覆盖进
  临时 Boost 根，然后跑 `b2 libs/<lib>/test`，以及
  `cmake -DBOOST_USE_MODULES=ON`（clang ≥ 19），并额外跑一次不带 `import std`
  的构建。这与 boost-ci 做的事一致，且能在 PR 存在之前就证明载荷可用。
- **mwrap 侧**：三个平台作业（`xlings`、`mcpp build/test`、模板 smoke、已安装
  消费者）加上 `gen_exports.py --check` 与 `gen_alias.py --check`。
- **共享一致性套件**（`tests/`）：清单里每个名字在 mwrap 导入**和**上游导入之后
  都断言可见；另有一个 TU 混用头文件与模块。B 档库还要断言：在
  `BOOST_USE_MODULES` 下包含该头文件是空操作。

### 5.7 文档与索引

- 根 `README.md` / `README.zh.md` 遵循 mwrap README 布局（导航行、Modules 在
  Getting started 之前、100–150 行、特性表）。
- `doc/` 是 Boost 形状，也是 PR 唯一携带的文档。
- `pkgs/mwrap/boost.lua` 声明 `namespace = "mcpplibs.mwrap"`、`name = "boost"`、
  `licenses = {"Apache-2.0"}`，指向封装层发布物。`bridge/` 与 `upstream/` 的
  BSL 写在 `NOTICE` 里，不混进封装层的 SPDX 字段。

### 5.8 许可证：按路径单许可，而不是"二选一"

"封装层能不能写成 Apache-2.0 OR BSL-1.0"有一个法律答案和一个工程答案，两者不同。

mwrap 到底怎么规定的（它是政策文档，不是许可证校验器）：

- 封装层**默认** Apache-2.0；根 `LICENSE` 放 Apache 正文，根 `NOTICE` 写明
  `mcpp-community` 并*说明封装层贡献的范围*；
- `mcpp.toml` 与描述符声明 Apache-2.0；
- 上游 `LICENSE`/`NOTICE` 要保留在 `upstream/` 下——也就是说，"一个仓库里存在
  多种许可证"本来就是常态；
- `onnxruntime-m` / `llama.cpp-m` 里的发布检查器硬断言
  `package['license'] == 'Apache-2.0'`；索引里所有 mwrap 描述符都是
  `licenses = { "Apache-2.0" }`；索引 CI 不校验许可证*取值*。

Boost 真正需要的是 BSL-1.0。原因不是"Apache 不自由"，而是其准则拒绝任何"必须随
二进制一起出现"的许可证，而 Apache-2.0 §4(d) 要求二进制分发中包含 `NOTICE` 内容。
BSL 没有这一条。这就是为什么 Boost 每个库都是 BSL-1.0，也是为什么双许可头会让评审
者先去读许可证正文、而不是代码。

而描述符能表达什么：`licenses` 是 SPDX id 的**列表**，所以
`{ "Apache-2.0", "BSL-1.0" }` 读作"两者都存在"，而不是"任选其一"；`OR` 表达式在
清单里无处安放。

三个选项，如实比较：

| 选项 | `bridge/` | 胶水（`src/`、`tools/`、`tests/`） | 根 `LICENSE` | 描述符 | 结论 |
|---|---|---|---|---|---|
| **A. 按路径单许可** | BSL-1.0 | Apache-2.0 | Apache + `NOTICE` 范围声明 | `{ "Apache-2.0" }` | **推荐**：每个产物只有一个许可证，两套规范都按字面满足，不给评审留问题 |
| B. 全仓 Apache，贡献时改 BSL | Apache-2.0 | Apache-2.0 | Apache | `{ "Apache-2.0" }` | 合法（权利人把要贡献的那份副本改为 BSL），最好管；上游副本与仓库副本只差一行文件头 |
| C. 双许可 "Apache OR BSL" | `BSL-1.0 OR Apache-2.0` | 同左 | 两份正文 | 无法表达 `OR` | 合法，但描述符退化为 AND，标准发布检查器直接失败，Boost 仍需裁定"哪一份适用于树里的副本" |

C 比 A 多做功，实际却没多给任何许可：Boost 收下文件的那一刻，"选择"就坍缩成
BSL。它换来的好处——文件搬去上游时不用改头——是每个库改一次、一次一行。

应当这样做：

```text
LICENSE                     Apache License 2.0（封装层贡献）
NOTICE                      mcpp-community；范围：src/、tools/、tests/、templates/、mcpp.toml；
                            bridge/ 为 Boost Software License 1.0；upstream/ 保留上游条款
bridge/LICENSE_1_0.txt      BSL-1.0 正文
bridge/modules/*.cppm       // Copyright <holder>
                            // Distributed under the Boost Software License, Version 1.0.
                            // See accompanying file LICENSE_1_0.txt or copy at
                            // https://www.boost.org/LICENSE_1_0.txt
```

每个文件只写一个 `SPDX-License-Identifier`，绝不写两个。如果项目日后确实要求整个
仓库只有一个表达式，请选 B 而不是 C：产物边界已经存在，让许可证跟着去向走即可。

**既有材料保留原许可证。** mwrap 要求保留继承来的封装层声明，并要求把遗留材料与
上游材料分别标注。因此，早期代码是 BSL-1.0 的仓库在采用封装层政策时不必整体重新
授权：新的胶水是 Apache-2.0，继承代码保持 BSL-1.0，`NOTICE` 写清哪块是哪块。这是
把选项 A 用到时间轴上——仍然是一个产物一个许可证，而不是 `OR`。

## 6. 不变量

按此顺序在 CI 中可检查。每一条都对应一个"不报错就出事"的错误。

1. `grep -rniE 'mcpp|mwrap|mcpplibs' bridge/` 无输出。
2. `bridge/modules/*.cppm` 每个只声明一个 `export module boost.<lib>;`，没有别的
   模块名。
3. `bridge/` 下每个源文件带 BSL 头与
   `// See https://www.boost.org/libs/<lib> ...`（许可证文本除外）。
4. `bridge/` 只用 `clang++ -std=c++20 --precompile` 加 include 路径就能构建——
   不需要 mcpp、不需要生成器、不需要包装脚本——且
   `cmake -DBOOST_USE_MODULES=ON` 能构建。
5. 无论 `bridge/` 在不在，`b2` 构建结果不变。
6. `bridge/` 加入前后 `meta/libraries.json` 字节相同。
7. `mcpp.toml` 不引用 `bridge/` 下任何文件；`bridge/` 里不出现任何 mwrap 模块名。
8. `generated/<lib>.inc` 的每个名字都能通过封装层导入在编译测试中解析；每个被跳过
   的名字都出现在跳过清单里。
9. 同时包含上游头文件并导入封装层模块的 TU 能编译链接（A 档），或被一个有文档、
   有测试的诊断明确拒绝（B 档）。
10. `gen_exports.py --check` 与 `gen_alias.py --check` 字节幂等，且重新生成不会
    触碰未变化的文件。

## 7. 贡献顺序

| 阶段 | 工作 | 门禁 |
|---|---|---|
| 0 | 只做封装层：对一个纯头文件子集（`asio`、`optional`、`mp11`、`describe`、`static_string` 等）实现 A 档，依赖索引里已经存在的逐库 `compat.boost-<lib>` 提供方（BSL-1.0，`1.92.0`） | mwrap 各门禁、索引发布、真实用户 |
| 1 | 选一个有活跃维护者的库；在 `bridge/` 写 B 档单元与钩子补丁；跑 `bridge-ci` | 维护者同意；`b2` + CMake 在 2 编译器 × 2 OS 上绿 |
| 2 | 提该库的 PR（载荷：`modules/`、头文件守卫、`test/cmake_subdir_test`、`doc/` 片段） | 该库维护者评审；进入下一发布列车（1.94 起；1.93 已于 2026-10-21 对新库关闭） |
| 3 | 该库发布后，把封装层门面翻成 `export import boost.<lib>;`，删掉那份清单 | 封装层测试不变，import 名不变 |
| 4 | 只有确实还需要元层时：贡献生成器/工具，或提新 Boost 库——后者才需要正式评审 | 联署、评审经理、正式评审 |

明确不进入上游的部分：`mcpplibs.mwrap.boost` 根聚合模块（它为满足 mwrap 根模块
规则而存在，而一个 import 全部 Boost 库的 `boost` 模块恰好是 Boost 最不想要的）、
`.macros` 映射（具名模块无法导出宏，上游让宏保持为宏）、模板、索引描述。

## 8. 风险

- **宏配置在模块构建时被冻结。** 一套 `BOOST_*` / 特性开关下编译出的模块单元，
  消费者无法重新配置，混用不同配置就是 ODR 违规。对策：每次模块构建固定一套有
  文档的宏集，映射到包特性，并用头文件/模块一致性测试覆盖。这与
  `onnxruntime-m` 已记录的固有限制相同。
- **编译器成熟度。** 本机实测：A 档机制在 Clang 23 + Boost 1.92 下编译运行通过，
  包括一个混用原始头文件与模块的 TU；GCC 14 在命名空间作用域运算符被导出后能通过
  轻量库，但在 Asio 的模板体上失败于
  "must `#include <typeinfo>` before using `typeid`"——这是消费者可见性缺口，不是
  设计缺陷。因此按库给出编译器矩阵，而不是给一个全局承诺。
- **一个程序里两种身份。** `boost.asio` 与 `mcpplibs.mwrap.boost.asio` 是同一批
  实体，可以同时导入（已实测）。真正的"唯一属主"规则针对的是**实现**，不是模块名：
  绝不同时链接 vendored 副本与 `compat.boost`。
- **上游意愿。** 模块支持是 opt-in，且已被搁置过一次。预算要按"逐库采纳"来算，
  而不是一次性全量接受；封装层永远不能依赖某个上游改动先落地。
- **版本漂移。** Boost 大约一年三次发布，偶尔用连字符重发。两者都由 §5.5 的映射表
  吸收；执行者必须是更新机器人，而不是人。

## 9. 实测证据

2026-10-28 在本机以 `clang 23.1.4` + Boost 1.92 头文件，以及 `g++ 14.2` 验证：

```cpp
// 一份清单，两个模块名，一个程序
// src/asio.cppm
module;
#include <boost/asio.hpp>
export module mcpplibs.mwrap.boost.asio;
#include "../include/boost/asio/module/exports.inc"

// upstream/asio.cppm
module;
#include <boost/asio.hpp>
export module boost.asio;
#include "../include/boost/asio/module/exports.inc"
```

- 两个模块单元都能预编译，两个消费者都能构建并运行；
- 消费者可以用上游拼写写 `boost::asio::io_context`、`boost::system::error_code`；
- 一个同时 `#include <boost/asio/io_context.hpp>` 并 `import boost.asio` 的 TU
  能构建运行（全局模块片段正是它安全的原因）；
- 同一 TU 同时导入 `boost.asio` 与 `mcpplibs.mwrap.boost.asio` 能构建运行；
- 只导出 `boost::optional` 而不导出 `boost::operator==` / `operator!=`，消费者写
  `opt != boost::none` 会失败；补上两条 using 声明即恢复——§5.3 的审计规则不是
  纸上风险。

## 10. 参考

- Boost 库要求与准则 — https://www.boost.org/development/requirements.html
- Boost 库提交流程 — https://www.boost.org/development/submissions.html
- Boost.CI 及其 CMake 指南 — https://github.com/boostorg/boost-ci
- 已被接收的模块先例 — https://github.com/boostorg/any（`modules/boost_any.cppm`）、https://github.com/boostorg/type_index
- 模块原型与分析 — https://anarthal.github.io/cppblog/modules
- mwrap 规范 — https://github.com/mcpplibs/mcpp-index/blob/main/docs/mwrap.md
- mwrap 许可、README 布局、包形状 — 同仓库 `docs/mwrap-licensing.md`、`docs/mwrap-readme.md`、`docs/package-types.md`

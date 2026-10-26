# 使用与 API

> `mcpplibs.mwrap.boost@1.92.0` · [English](usage.md) · 简体中文
> 设计与生成链：[architecture.zh.md](architecture.zh.md)

Boost 1.92.0 被封装为 C++23 具名模块。导入模块即可替代该库的 `#include`，模块导出的一切保持上游拼写：`boost::optional`、`boost::filesystem::path`、`boost::json::value`、自由运算符、ADL。封装层不新增任何 `mcpplibs::mwrap::boost` 类型——该命名空间仅为下文所述的宏映射而存在。

## 1. 依赖声明

```toml
# The default feature set (50-library closure)
[dependencies]
"mcpplibs.mwrap.boost" = "1.92.0"

# A specific set; default-features = false stops this edge from also pulling
# the default closure in
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, features = ["optional", "json"] }

# Everything with a module interface
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, features = ["all-modules"] }

# From the repository at the immutable release tag
[dependencies]
"mcpplibs.mwrap.boost" = { git = "https://github.com/mcpplibs/boost-m", tag = "mwrap-1.92.0", default-features = false, features = ["optional"] }
```

版本方案：首个 mwrap 版本即上游版本 `1.92.0`。仅涉及封装层的修复追加修订号 `1.92.0.1`，且永不发布 `1.92.0.0`（其排序结果与 `1.92.0` 相同）。仓库标签使用另一套序列 `mwrap-<version>`；旧的 `ZheFeng7110.boost` 版本与标签不再复用。

## 2. 使用方式

### 2.1 导入模块

```cpp
import mcpplibs.mwrap.boost.optional;
import mcpplibs.mwrap.boost.filesystem;

boost::optional<int> value = 42;
boost::filesystem::path p = "notes.txt";
```

根模块重导出当前已启用特征所构成的闭包：

```cpp
import mcpplibs.mwrap.boost;   // exactly the active feature set
```

每个特征的 `.cppm` 本身也是独立的公开模块，因此消费者可直接导入子模块；当其对应特征已启用时，根模块会额外重导出该子模块。

### 2.2 模块与原始头文件并存

封装层并不持有头文件：`upstream/boost` 位于公开包含路径上。同一个翻译单元既可以（为使用所有宏 API 所必需地）直接包含 Boost 头文件，也可以导入模块——模块图与头文件图共享同一棵 vendor 树。

```cpp
#include <boost/scope_exit.hpp>          // macro API: include-only library
import mcpplibs.mwrap.boost.optional;    // module API

BOOST_SCOPE_EXIT(void) { /* ... */ } BOOST_SCOPE_EXIT_END
```

### 2.3 仅头文件使用的库

有些库只以头文件方式使用。它们既没有模块，也没有特征。

| 分组 | 库 | 原因 |
|---|---|---|
| 宏 API | `preprocessor`、`mpl`、`fusion`、`proto`、`spirit`、`xpressive`、`typeof`、`vmd`、`phoenix`、`parameter`、`metaparse`、`function_types`、`tti`、`local_function`、`msm`、`foreach` | 公开接口是一组宏；具名模块无法导出宏 |
| 编译器/缺陷回退 | `predef`、`static_assert`、`exception`、`describe`、`openmethod`、`scope_exit` | 宏体 API、与关键字同名的模块名、include+import 的 ODR 冲突，或 gcc 的 CMI pendings 缺陷 |

`static_assert` 完全无法成为模块：其名称是 C++ 关键字，因此 `export module ...boost.static_assert;` 非法。消费者必须包含 `boost/exception/all.hpp`：在任何真实的消费者翻译单元中，gcc 16.1 都无法加载该模块的 pendings。

### 2.4 无模块的编译库

`log` 与 `unit-test-framework` 提供编译型库翻译单元，但没有模块接口。启用对应特征并包含上游头文件即可：

```toml
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, features = ["log"] }
```

```cpp
#include <boost/log/trivial.hpp>
```

Boost.Test 有两种互斥的形态：启用 `unit-test-framework` 时，框架翻译单元参与链接，`main` 由消费者提供；未启用时，使用官方仅头文件的聚合头 `<boost/test/included/unit_test.hpp>`。同一程序内混用两种形态会导致框架符号重复定义。

### 2.5 不支持的库

`context`、`fiber`、`coroutine`、`locale`、`mpi`、`python`、`parameter_python`、`graph_parallel`、`compute`、`mysql` 与 `redis` 需要汇编工具链、ICU、MPI、Python、OpenCL 或 OpenSSL，超出 vendor 树范围。它们未被声明，请求即报错。

## 3. 宏

具名模块无法导出预处理宏。宏接口被拆分为两部分：

| 接口 | 入口 | 用途 |
|---|---|---|
| 映射后的对象常量 | `import mcpplibs.mwrap.boost.macros;` | `mcpplibs::mwrap::boost::macros::BOOST_VERSION`、`...::BOOST_LIB_VERSION` |
| 其余全部 | 在导入语句之前 `#include <mwrap/boost/macros.hpp>`，或包含相应的上游头文件 | 字符串化、记号拼接、条件编译、控制流、断言家族 |

`version` 是一个薄模块，会重导出 `.macros`，因此只想使用版本常量的消费者可导入 `mcpplibs.mwrap.boost.version`。被映射的实体只有一个归属：`.macros`。

```cpp
import mcpplibs.mwrap.boost.macros;

static_assert(mcpplibs::mwrap::boost::macros::BOOST_VERSION == 109200);
const char* tag = mcpplibs::mwrap::boost::macros::BOOST_LIB_VERSION;   // "1_92"
```

已启用的宏即使在限定名内部也会展开。在包含了 `<boost/version.hpp>`（或 `<mwrap/boost/macros.hpp>`）的翻译单元中，上面的拼写会变成 `...::109200`，不再指称该常量。要么在没有原始宏的翻译单元中使用映射常量，要么先记录其取值再 `#undef`：

```cpp
#include <mwrap/boost/macros.hpp>
import mcpplibs.mwrap.boost.macros;

constexpr int raw_version = BOOST_VERSION;
#undef BOOST_VERSION
#undef BOOST_LIB_VERSION
static_assert(mcpplibs::mwrap::boost::macros::BOOST_VERSION == raw_version);
```

映射值保持其类型与求值频率：`BOOST_VERSION` 是 `int`，`BOOST_LIB_VERSION` 是 `const char*`。断言宏仍是宏——`BOOST_ASSERT(expr)` 只求值 `expr` 一次，且在 `NDEBUG` 下消失；函数无法复现这一行为。`tests/macros.cpp` 与 `tests/macros_raw.cpp` 覆盖上述两部分及该名称冲突。

## 4. 特征

名称采用小写 kebab-case。模块后缀沿用 Boost 的分组拼写，因此特征 `type-traits` 启用的是 `mcpplibs.mwrap.boost.type_traits`。

| 特征 | 作用 |
|---|---|
| `default` | 50 个库构成的闭包（见下文） |
| `all-modules` | 全部 117 个模块条目（116 个库接口 + `macros`）；不含 `log`，不含 `unit-test-framework` |
| `macros`、`version` | 映射模块及其薄重导出 |
| `backend-log-avx2`、`backend-log-ssse3` | 构建期的 Boost.Log dump 实现，仅限 x86_64，每条轴各择其一 |

默认闭包沿模块导入边闭合：
`optional variant variant2 any core container-hash mp11 type-traits algorithm
iterator range io endian tuple system filesystem regex thread chrono json array
assert compat concept-check config dynamic-bitset functional integer intrusive
lambda lexical-cast move multi-array ratio smart-ptr throw-exception tokenizer
type-index units utility winapi atomic container date-time math random
serialization numeric version macros`。

特征在各依赖边上可累加。某条边上的 `default-features = false` 只关闭该边本来会请求的默认集合：其他边仍可再次请求默认特征，而显式特征总是生效。启用某特征会追加其 `implies` 闭包，因此选中一个库时，拉取的正好是该库接口所导入的内容。

### 4.1 构建期配置档

`backend-log-avx2` / `backend-log-ssse3` 以逐翻译单元的 `-mavx2` / `-mssse3` 以及包侧分派宏来编译 Boost.Log 的 AVX2/SSSE3 dump 翻译单元。基础构建有意排除这些翻译单元（无条件的 `<immintrin.h>` 会破坏非 x86 目标）。两者都受 x86_64 约束，且在同一轴上互斥；当架构不符或同一轴上出现第二个配置档时，`build.mcpp` 会以指出相应特征的诊断信息拒绝。

```toml
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, backend = "log-avx2" }
```

## 5. 平台、ABI 与运行时

已验证的 CI 组合：Linux x86_64 配 GCC 16.1 与 LLVM，Linux arm64 配 GCC，macOS arm64 配 LLVM（Apple 链接器；lld-macho 会重复一条零 unwind 条目），Windows x86_64 配 clang-msvc。真正的 MSVC（`cl`）、MinGW 以及依赖外部 SDK 的库不在承诺范围内。

与 ABI 相关的配置由包固定，而非由消费者决定：

- `BOOST_FILESYSTEM_VERSION=3` 与 `BOOST_THREAD_VERSION=2` —— 生成导出时所依据的上游默认值，予以固定，以免环境使模块接口与库翻译单元失步；
- `BOOST_ALL_NO_LIB`、`_MT`、`WIN32_LEAN_AND_MEAN`、`SECURITY_WIN32`；
- thread 的翻译单元额外获得 `BOOST_THREAD_BUILD_LIB`（按 glob 设置，绝不传播）；
- C++23，异常与 RTTI 开启，根包的工具链 stdlib 与运行时。
- 构建 profile 决定 `NDEBUG`：release 构建走 Boost.uBLAS 的快速尺寸检查路径，
  因此只在检查构建中存在的 uBLAS 实体（`same_impl_ex`）也只在检查构建中导出——
  模块跟随头文件条件，release 程序中使用它与直接包含原始头文件时一样不可用。

消费者自行定义上述任一宏，不会改变 BMI 中已冻结的内容。此集合之外的配置必须采用仅头文件方式（`#include`）使用，而不能使用模块。

## 6. 聚合上限

- 当任何模块通过伞模块间接重导出 `mcpplibs.mwrap.boost.sort` 时，GCC 16.1 会在写出其 BMI 的过程中内部错误（ICE）。根模块在 GCC 下会略过 `sort` 并给出警告；请直接导入 `mcpplibs.mwrap.boost.sort`（这是允许的，且不受影响）。
- 构建整个 `all-modules` 闭包时，可能触及 Clang 的 2^31 源位置上限。此时应改为导入库分组而非伞模块；该上限是编译器的边界，而非包的缺陷。
- 分组级别的成功并不能证明伞模块闭包可用：`all-modules` 是一个模块集合，而非「每个编译器都能聚合它」的声明。

## 7. 从 `ZheFeng7110.boost` 迁移

| 旧 | 新 |
|---|---|
| `[dependencies.ZheFeng7110] boost = "1.92.0.0.0.1"` | `[dependencies] "mcpplibs.mwrap.boost" = { version = "1.92.0", features = [...] }` |
| `import boost.optional;` | `import mcpplibs.mwrap.boost.optional;` |
| `import boost;` | `import mcpplibs.mwrap.boost;` |
| `features = ["type_traits"]` | `features = ["type-traits"]` |
| `features = ["all"]` | `features = ["all-modules"]` |
| `boost::BOOST_VERSION` (old wrapper extension) | `mcpplibs::mwrap::boost::macros::BOOST_VERSION` |

`boost::optional<int>`、`boost::filesystem::path` 以及其他所有上游实体均未改变。旧包的版本、标签与归档保持不可变；这是一个新的身份，而非一次重命名。

## 8. 工具链固定与测试

受测的构建工具固定在 `.xlings.json` 中（`mcpp 2026.10.24.1`）；CI 在每条组合上运行同一固定版本。生成（重新生成导出列表或审计头文件）还需要 LLVM 23.1.3 及匹配的 libclang，以及 `tools/` 下固定的 MinGW sysroot；普通消费者只使用已提交的生成文件。

```sh
mcpp build                      # default feature closure
mcpp test                       # every tests/*.cpp as its own program
mcpp test optional              # one test by pattern
mcpp test --features unit-test-framework   # compiled Boost.Test form
```

各操作系统的 shell、ABI 变体以及上文的仅头文件列表，界定了测试矩阵的覆盖范围；仅完成构建的结果会在 CI 中如实标注。

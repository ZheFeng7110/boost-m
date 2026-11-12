---
subject: tests
status: active
---

# 模块测试加固的发现复盘：封装层缺陷与上游 Boost 问题（对照 1.93.0.beta1）

> 日期：2026-11-12 · 分支 `test/module-test-hardening`（上游草稿 PR
> [mcpplibs/boost-m#5](https://github.com/mcpplibs/boost-m/pull/5)，调查时的 head 为 `46559d3e`）
>
> 输入：PR #5 的本意是用更完整的测试找出封装层漏掉的问题。本记录回答两个问题：
> 这一轮测试是否暴露了以前没有发现的问题；其中属于上游 Boost 的问题，官方是否已跟踪、
> 是否已在 1.93.0 预览版中修复。

## 0. 结论

1. **测试暴露了一批以前没发现的封装层缺陷**，十几处，都已在 PR #5 内修复。
   逐项与 `origin/main` 的 `generated/exports/*.inc` 对照，这些问题在 main 上都存在（§2）。
   其中最严重的是 regex：main 上原有补丁（M5）引入了运行时错误，能编译、能链接，
   只在错误路径上出错，原来的 smoke 测试覆盖不到。
2. 有 3 个问题**测出来了但没有解决**，在 CI 上靠跳过绕开（§2.4）。风险最高的是
   windows-llvm-msvc 上的堆损坏，怀疑是封装层的 ODR 问题。
3. 测试顺带发现了一批**上游 Boost 自身的问题**（§4、§5）。对照 1.93.0.beta1 rc1：
   只有 container 的 include guard、decimal 的 `logb`/`ilogb`、decimal 文档里的名字
   这 3 项已修复，其余都还在。除这 3 项外，上游 issue/PR 中都没找到对应的跟踪。
4. algorithm 的 include guard 问题已提交上游 PR
   [boostorg/algorithm#136](https://github.com/boostorg/algorithm/pull/136)（2026-11-11，open）。
   其余待报项见 §8。
5. 调查中还发现 PR #5 自己的两条排除理由写错了（§6）。
6. Boost.Math 1.93 开始自带官方 C++ 模块（`libs/math/module/math.cppm`），
   升级到 1.93 时需要考虑它和 boost-m 的 math 模块怎么共存（§7）。

## 1. 来源与方法

- **封装层问题**：来自 PR #5 的提交说明与
  [测试加固记录](2026-11-09-module-test-hardening-design.md) §11.4、§11.6。对每一项，
  在 `origin/main` 的导出文件中确认缺失或错误确实存在。
- **1.93.0 预览版**：GitHub 上还没有 `boost-1.93.0.beta1` 的 tag（截至 2026-11-11，
  最新 tag 是 `boost-1.92.0`）。归档站点已有
  `archives.boost.io/beta/1.93.0.beta1/source/boost_1_93_0_b1_rc1.tar.bz2`，
  基本信息如下：
  - 打包时间：2026-11-05。
  - superproject commit：`f976a2b7b525300c0ff63ace104d2d83793d3153`。
  - SHA256：`d195115f488cf1a30670a005967f28a933471f00151e4eef37b63e085f60528d`，已校验。
  - 对照基线是 `.tmp/boost-ref/boost_1_92_0.tar.gz`。
- **验证方式**：
  - 逐个比较相关文件在 1.92 与 1.93 之间的差异。
  - 在两个版本的头文件上各编译运行一次探针程序和原始示例，编译器为 clang 23.1.3。
  - 部分项目另查了上游 develop 分支。
  - 用 `gh` 搜索各库仓库的 issue/PR。

## 2. 测试暴露的封装层问题（PR 之前就存在）

### 2.1 运行时行为错误

- **regex**：main 上的 `patches/regex.patch`（M5）为解决 gcc 的多重定义问题，
  把 `regex_traits_defaults.hpp` 中函数内的 static 消息表改成了内部链接的命名空间级数组。
  在 import 了 regex 模块的 TU 里，gcc 会生成一份未初始化的本地副本，后果有三：
  - `get_default_error_string()` 返回 nullptr。
  - 非法正则抛出的是 `std::logic_error`（从 null 构造 string），而不是 `regex_error`。
  - `[[.space.]]` 之类的 collating name 也会失败。

  PR #5 改为 `inline constexpr`，修复后仍保留 COMDAT 语义，原来的多重定义问题依然解决。
  这类问题能编译、能链接，只在错误路径上出错，是本轮测试价值最直接的体现。

### 2.2 模块缺少文档中的公开 API

| 库 | main 上的状态 | 根因 |
|---|---|---|
| algorithm | 没有 `transform_reduce` | 上游 include guard 冲突（§4 #1） |
| container | 没有 `pmr::vector` / `vector_of` | 上游 include guard 冲突（§4 #2） |
| conversion | 没有这个模块，cast 由 range/graph 代为导出 | 前缀启发式找不到 `boost/` 顶层的头文件 |
| core | 没有任何模块导出 `boost::core::string_view` | 它唯一的头文件在 `detail/` 下 |
| convert | 有 `convert()`，没有任何 converter | converter 头文件不能单独编译，被过滤器丢弃 |
| asio | 没有 `experimental::use_promise`、`stream_errc`、`socket_errc` | 同上；生成器不收集只含 using 声明的命名空间 |
| beast | 没有 `beast::errc` | 它是命名空间别名，生成器不收集 |
| config | 没有 `exit_success` 等 | `boost/cstdlib.hpp` 不属于任何模块 |
| dll | `dll::fs::path` 不可达 | 上游用 using 指令定义，导入方看不到 |
| decimal | `std::format` 无法处理 decimal 类型 | GMF 中没有先包含 `<version>`，formatter 特化没有被声明 |

### 2.3 TU-local 对象无法导出，以及其他编译期问题

**TU-local 对象。** 以下命名空间作用域的 `constexpr`/`const` 对象是 TU-local，无法导出：

- convert 的 `throw_on_failure`；
- bimap 的 `_key`/`_data`；
- url 的 19 个字符集与语法规则；
- json 的 kind 标签；
- cobalt 的 `use_op`/`use_task`/`this_coro::*` 与 io 协议常量；
- interprocess 的创建标签与锁选项；
- math 的 `*_constants`。

[2026-09-20 的记录](2026-09-20-internal-linkage-export-completion.md)已经处理过同一根因的问题，
但只覆盖了 range、multi_array、accumulators、mqtt5。所以这些属于已知根因的漏网项：
以前是按已知案例逐个修，这次是靠实际使用扫出来的。

**其他编译期问题：**
- cobalt：GMF 带入了 `main.hpp` 中定义的 `main`，与使用方自己的 `main` 冲突。
- geometry：fusion 适配与 tuple 适配同时可见，产生歧义；`boost::size` 与 `std::size` 有歧义。
- variant：匿名结构体使 gcc 16 在每次使用 rtree 时 ICE。
- math：`cubic_roots` 等头文件依赖使用方带入的 `using std::sin`。

### 2.4 不计入「以前就有」的部分，以及未解决的问题

**不计入的部分：**
- **PR 自己引入又修掉的回归**：导出 `using ::operator new;` 后，clang/libc++ 上出现歧义
  （`f62e1725` 引入，`41d89f02` 修复）。相对 main 是零增量。
- **测试代码自身的平台适配**：macOS 的 `struct user`、Windows 的 `min`/`max`/`near` 宏、
  long double 宽度、libm 末位、MSVC 空基类 ABI 等。这些不是产品缺陷。

**未解决的问题**（见 [todos/module-tests-open-issues.md](todos/module-tests-open-issues.md)）：

1. **windows-llvm-msvc 上的堆损坏（0xC0000374）与 SIGSEGV**：涉及 asio `composed_4`、整个
   `cobalt/doc`、math `daubechies_scaling`，只在这一条腿上出现。怀疑方向是同一段 asio
   内联代码在不同模块的 GMF 中以不同配置实例化。如果属实，这是影响 Windows 使用方的真实缺陷，
   目前靠跳过绕开。三项中风险最高。
2. **aarch64 上 `ooura_fourier_cos<double>` 返回 NaN**：还没确定是上游问题还是封装层问题（§4 #6）。
3. **Windows 上以 shared 形态构建本包时，包内编译源码的符号没有导出**，`tests/dll` 链接失败。
   以前没有使用方以 shared 形态依赖本包，这是新暴露的打包问题，或 mcpp 的
   `windows_auto_export` 问题。

## 3. 上游问题的归类

测试发现的上游问题分三类：

- **库本身的 bug**（§4）：只用头文件的普通用户也会遇到。
- **示例与文档的错误**（§5）：示例照原样编译或运行失败，或文档与实现不一致。
- **对模块不友好的写法**（§7）：TU-local 对象、伞头文件带入 `main`、命名空间别名等。
  对只用头文件的用户不算 bug，boost-m 用补丁或 policy 处理。

## 4. 上游库的 bug

| # | 问题 | 上游跟踪 | 1.93.0.beta1 |
|---|---|---|---|
| 1 | **algorithm**：`cxx17/transform_inclusive_scan.hpp` 使用 `transform_reduce.hpp` 的 include guard（`BOOST_ALGORITHM_TRANSFORM_REDUCE_HPP`），两个头文件都包含时，后包含的那个整体消失 | 原先无。2020 年的 PR #81（`8c26a50`）只修了同样复制而来的 `inclusive_scan.hpp`。本轮已提交 [#136](https://github.com/boostorg/algorithm/pull/136)，同时修正两个文件 `\file`/`\brief` 注释的复制错误 | ❌ 未修复（develop 也未修复） |
| 2 | **container**：`pmr/devector.hpp` 使用 `pmr/vector.hpp` 的 guard | 无 issue，维护者直接提交 `a011b1c4`（2026-10-15，"Fix copy-paste error for header include guard"） | ✅ 已修复 |
| 3 | **decimal**：`long double` 为 113 位时，`static_cast<long double>(decimal_fast128_t(1.5))` 得 `1.5e15`；`decimal128_t` 正确 | 未找到 | ❌ 未修复。在 x86_64 上用 `clang -mlong-double-128` 实测，1.92 与 1.93 结果相同 |
| 4 | **decimal**：`logb`/`ilogb` 返回带偏置的指数，`logb(decimal64_t(1000)) == 401` | [#1437](https://github.com/boostorg/decimal/issues/1437) / [#1439](https://github.com/boostorg/decimal/pull/1439)，2026-08-29 修复，晚于 1.92.0（2026-08-05） | ✅ 已修复（实测返回 3） |
| 5 | **charconv**：`from_chars("1.3a2bp-10", …, chars_format::hex)` 得 8.0427e-10：十六进制数字被当成一个整数读入，再按十的幂缩放。文档写 8.0427e-18，C hexfloat `0x1.3a2bp-10` 的正确值约为 1.1985e-3，三者互不一致 | 未找到。上游测试 `from_chars_float.cpp`（`simple_hex_scientific_test`）把 `80427e-14` 写成了期望值 | ❌ 实现、文档、测试都没变 |
| 6 | **math**：aarch64 Linux 上 `ooura_fourier_cos<double>` 返回 NaN（结果与误差估计都是 NaN，`ooura_fourier_sin` 正常），节点以 113 位 `long double` 计算 | 未找到。[#1606](https://github.com/boostorg/math/issues/1606) 第 6 条是另一个 ooura 缺陷（`levels = 0` 越界） | ❓ 1.93 只改了命名空间宏。是不是上游问题，还要在 aarch64 上用纯 include 程序确认 |
| 7 | **math**：`tools/cubic_roots.hpp` 与 `optimization/detail/line_search_policies.hpp` 使用未限定的 `sin`/`fabs`，依赖 C 头文件把 `::sin`/`::fabs` 带进来 | 未找到 | ❌ 未修复（1.93 只加了模块宏） |
| 8 | **variant**：`detail/has_result_type.hpp` 的 `typedef struct { char array[2]; } no;` 使 gcc 16.1 在模块导入方 ICE | 未找到 | ❌ 未修复。本质上是 gcc 的 bug，应报给 GCC |

## 5. 示例与文档的错误

| 问题 | 上游跟踪 | 1.93.0.beta1 |
|---|---|---|
| math `example/double_exponential.cpp` 段错误：`tanh_sinh<double>(tol, 10)` 把 `tol` 传给了 `max_refinements`（参数顺序反了） | 未找到（[#546](https://github.com/boostorg/math/issues/546) 只是相关的参数陷阱） | ❌ 实测两个版本都段错误 |
| math `example/lambert_w_example.cpp` 使用已不存在的 `lambert_w`，编译失败 | 未找到 | ❌ 未修复 |
| cobalt `example/spsc.cpp` 的 `awaitable_spsc_queue` 有数据竞争：`await_suspend` 写入 executor 与句柄，没有和另一线程的检查与 post 同步。可能丢失唤醒而挂起；在 aarch64 上曾崩溃 | 未找到 | ❌ 文件没变 |
| geometry `doc/src/examples/geometries/adapted/boost_range/uniqued.cpp` 编译失败（`uniqued` 需要 `operator==` 与双向迭代器） | 未找到 | ❌ 未修复 |
| geometry `index/example/random_test.cpp` 使用私有的 `qbegin_`，编译失败 | 未找到（[#1050](https://github.com/boostorg/geometry/issues/1050) 不是这个问题） | ❌ 未修复 |
| geometry 文档中 `strategy::within::winding` 的用法：`bg::within` 实际不接受这个通用策略，只接受 `cartesian_winding` | 未找到 | ❌ 相关文件没变 |
| dll `example/mangled/import_class.cpp`（漏了 `<<`）与 `smart_lib.cpp`（`cout`/`endl` 未限定）编译失败。clang 23 报的第一个错误分别是未限定的 `smart_library` 和不存在的 `boost::dll::smart_lib` | 未找到 | ❌ 未修复 |
| hash2 文档 `hashing_objects` 的 arrays/ranges 示例两次调用 `h1.result()`；`result()` 会推进算法状态，第二个断言不成立 | 未找到 | ❌ 文档没变 |
| decimal 文档的名字与实现不一致（`quantized` → `quantize`、`quantexp64` → `quantexpd64` 等） | 有相关修复（[#1415](https://github.com/boostorg/decimal/issues/1415) / [#1416](https://github.com/boostorg/decimal/pull/1416)） | ✅ 文档已更新 |

## 6. PR #5 自身记录的错误

`tests/math/coverage.toml` 中有两条排除理由与事实不符：

- `continued_fractions.cpp` 的理由（"用了库里已经没有的名字"）是从 `lambert_w_example` 那条复制来的。
- `policy_eg_8.cpp` 的理由说用了未限定的 `endl`，但它第 15 行就有 `using std::endl`。

这两个示例在 1.92 与 1.93 上都能编译，也能正常运行。应当二选一：改为可在模块形态下运行的用例；
或写明真实理由，比如 `policy_eg_8` 依赖在包含头文件前定义的策略宏，宏无法作用于已编译的模块。

## 7. 1.93 中与 boost-m 相关的变化

- **Boost.Math 自带官方模块。** 1.93 新增：
  - `libs/math/module/math.cppm`；
  - `BOOST_MATH_BUILD_MODULE`（225 个头文件）；
  - `BOOST_MATH_EXPORT` / `BOOST_MATH_NAMESPACE_BEGIN`；
  - 用 `BOOST_MATH_INLINE_CONSTEXPR` 声明的 `float/double/long_double_constants`。

  升级时 `math.patch` 中常量那部分可以去掉，需要决定 boost-m 的 math 模块是沿用生成器，
  还是对接官方模块。nonfinite facet 的标志（`legacy` 等）在 1.93 中仍是 `const int`。
- **container**：升级到 1.93 后可删去 `patches/container.patch`。
- **其他对模块不友好的写法都没变**，对应补丁和 policy 升级后需继续保留：
  - interprocess 的 `static const create_only_t create_only`；
  - `boost/cstdlib.hpp` 的 `const int exit_success`；
  - bimap 的 `_key` 引用；
  - regex 的 `static const char* const s_default_error_messages[]`；
  - `boost/cobalt.hpp` 包含 `main.hpp`；
  - beast 的 `namespace errc = boost::system::errc;`。
- decimal 的 `logb` 修复改变了绝对值。现有用例（`tests/decimal/api_cmath_io.cpp` 的 `doc: cmath#cmath_manipulation_funcs`）比较的是差值，不受影响；
  升级后可以改为直接比较文档给出的值。

## 8. 后续

已完成：
- algorithm include guard：[boostorg/algorithm#136](https://github.com/boostorg/algorithm/pull/136)。
  合并后视版本删去 `patches/algorithm.patch`。

待报上游（尚未提交）：
1. decimal：113 位 `long double` 上 `decimal_fast128_t` 的转换错误（§4 #3）。
2. charconv：十六进制浮点解析与文档、C 语义都不一致（§4 #5）。
3. math：`double_exponential.cpp` 参数顺序；`lambert_w_example.cpp` 编译失败；
   `cubic_roots`/`line_search_policies` 的未限定数学函数（§4 #7、§5）。
4. hash2：文档示例两次调用 `result()`。
5. geometry：`uniqued.cpp`、`random_test.cpp` 两个示例，以及 `winding` 策略的文档。
6. dll：`mangled/` 两个示例。
7. cobalt：`spsc.cpp` 的数据竞争。
8. GCC：`has_result_type.hpp` 匿名结构体触发的 ICE，先做最小复现。

待确认：
- aarch64 上 `ooura_fourier_cos` 的 NaN（§4 #6）。
- windows-llvm-msvc 的堆损坏（§2.4）。

待修正：§6 的两条排除理由。

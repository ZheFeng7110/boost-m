---
subject: tests
status: landed
---

# 模块封装层测试加固：常用 34 个库的示例覆盖与自定义类型套件

> 日期：2026-11-09 · 状态：实施中（进度与过程见 §11，截至 2026-11-10）；§9 的问题均已答复
>
> 输入：按 boost.org「most popular」排序选出的 34 个库；要求补全被测 API，至少覆盖官方
> 使用示例；凡支持自定义类型的库，必须带自定义类型的测试套件；按
> `tests/<lib>/<srcs>.cpp` 组织，必要时用 `tests/<lib>/mcpp.toml` 建独立测试工程。

## 0. 结论

1. 34 个库的测试从 `tests/<lib>.cpp` 的单文件 smoke 改为目录 `tests/<lib>/`，每个文件
   仍是一个独立程序（`mcpp test` 的既有语义），测试名随之变为 `<lib>/<file>`。
   其余库保持平铺，不在本次范围内。
2. 每个目录有固定的几类文件：`smoke.cpp`（原测试原样迁入）、`api_<topic>.cpp`（按主题
   补齐的 API）、`examples_<group>.cpp`（官方示例的移植）、`custom_types.cpp`（自定义
   类型套件，凡有定制点的库必须有）。必要时再加 `include_<topic>.cpp`，放纯 include 形态的
   宏 API。
3. 测试框架用 doctest，从 mcpp index 的 `compat.doctest`（2.4.12）引入，放在根清单的
   `[dev-dependencies]` 中，只有 `mcpp test` 会解析它，发布出去的包不受影响。
   不自写框架；也不用 Boost.Test，原因有二：它有两种互斥的形态，而且它本身就是被测对象（§4.1）。
4. 「覆盖官方示例」以清单的方式落实：`tests/<lib>/coverage.toml` 把 Boost 1.92.0 发布包中
   该库的每个示例文件（以及文档里的示例片段）映射到一个用例，或者写明排除理由。
   `tools/ci/check_test_coverage.py` 检查这份清单。vendored 树已经裁掉了 `example/` 和
   `doc/`，所以清单是提交进仓库的事实，升级 Boost 时重新生成。
5. 独立测试工程只用于根工程无法表达的场景：`dll`（需要构建共享库插件）与 `test`
   （Boost.Test 的两种互斥形态）。根清单用 `[test] discover` 的 `!` 规则把这两个目录
   排除出根测试，CI 再分别进入这两个目录运行测试。
6. 先做 P0 验证（§8），再动手写测试，重点验证 gcc 16 下 include 与 import 混用的问题。
   这个问题决定了 json、geometry、hash2、container_hash、describe 这些库的自定义类型套件
   能否走 import 形态。
7. conversion 的模块化放在本轮一起做（§5.3）。它没有模块，原因是生成器的头文件启发式漏掉了
   它（§9 Q4），并非技术限制。修复先于 conversion 的测试，排在 P1。

## 1. 现状

### 1.1 现有测试

`tests/` 下每个 `.cpp` 是一个独立程序。`mcpp test` 默认发现 `tests/**/*.cpp`，测试名是
文件相对 `tests/` 的路径去掉扩展名。目标库的测试几乎都是 20～60 行的 smoke：只调几个
入口，以 `assert` 判定，没有自定义类型（`describe.cpp` 例外），也不对照官方示例。

| 库 | 现有测试 | 形态 |
|---|---|---|
| 31 个有模块的库 | `tests/<lib>.cpp` 各一 | `import mcpplibs.mwrap.boost.<lib>;` |
| describe | `tests/describe.cpp` | 纯 include（无模块，见 docs/usage.zh.md §2.3） |
| test | `tests/test_utf.cpp`、`tests/test_included.cpp` | 纯 include，两种形态由 `MCPP_FEATURE_UNIT_TEST_FRAMEWORK` 切换 |
| context、coroutine2 | 无 | 未声明（需要汇编工具链，usage §2.5） |
| conversion | 无 | 没有模块，也没有 feature；`polymorphic_cast` 等头文件在 vendored 树中 |

### 1.2 官方示例的来源

`upstream/boost/` 是裁剪过的发布树，保留了 `libs/<lib>/test/`，但没有 `example/` 和
`doc/`。本次从 `upstream/boost/SOURCE.md` 记录的官方地址下载了
`boost_1_92_0.tar.gz`（SHA-256 `c4a3b310…296ce`，与记录一致），解压到 `.tmp/boost-ref/`
（已被 gitignore），只作为参考，不进仓库。各库的示例规模：

| 规模 | 库（`example/` 下 `.cpp`/`.hpp` 文件数） |
|---|---|
| 巨大 | asio 240、geometry 202、math 171、test 143、beast 70、decimal 46 |
| 中等 | callable_traits 37、context 27、bimap 27、dll 27、url 26、regex 22、container 20、describe 16 |
| 小 | json 12、cobalt 12、hash2 11、mqtt5 11、convert 7、container_hash 7、algorithm 5、circular_buffer 5、coroutine2 5、bloom 4、dynamic_bitset 3、unordered 3、crc 1 |
| 只在文档里 | uuid、variant2、any、compat、charconv、core、conversion（示例写在 `doc/*.adoc` / `*.qbk` 中） |

巨大的几类大多是网络服务、命令行工具和基准：asio 的 `cpp11/http`（32 个）、beast 的
websocket 与 http server/client，math 的大量数值演示，test 的 143 个框架演示。
「逐一照搬」既不可行，也不是测试的目的，因此 §4.5 给出收录规则。

### 1.3 已验证的 mcpp 行为（mcpp 2026.11.5.1，在 `.tmp/probe` 的最小工程上实测）

- `tests/foo/one.cpp` 能被发现，测试名为 `foo/one`；`mcpp test foo` 只跑 `foo/*`；
  同目录的 `.hpp` 不会被当成测试。
- **子目录里放 `mcpp.toml` 并不能把它从根测试中排除**：根工程照样把 `tests/bar/x.cpp`、
  `tests/bar/src/main.cpp`、`tests/bar/tests/t.cpp` 都当成自己的测试。
- 根清单的 `[test] discover = ["tests/**/*.cpp", "!tests/bar/**"]` 能正确排除。
  （`[test]` 支持的键：`discover`、`main`、`start`、`windows_code_page`、`windows_long_paths`。）
- 嵌套工程用 `{ path = "../.." }` 依赖根包，在它自己的目录里运行 `mcpp test` 可以通过；
  在根清单中声明 `[workspace] members = ["tests/bar"]` 后，`mcpp test --workspace`
  也能跑到它。
- `[targets.<name>] kind = "shared"` 受支持（从二进制的诊断信息确认，尚未实测），
  dll 插件依赖这一点。

### 1.4 约束

- CI 有 5 台宿主，每台跑 A、B 两组；`mcpp test` 编译的是基础源文件的并集，因此**每个
  测试程序在 10 条腿上各编译一次**，单腿超时 240 分钟。测试的数量和单个 TU 的重量直接
  乘到 CI 时间上。
- 具名模块不导出宏。凡是以宏注册定制点的 API（`BOOST_DESCRIBE_STRUCT`、
  `BOOST_GEOMETRY_REGISTER_POINT_2D`、`BOOST_FUSION_ADAPT_STRUCT` 等），用户 TU 都必须
  include 头文件。
- gcc 16 在同一 TU 中 include 与 import 混用时，对部分实体族报 ODR 冲突，这是
  `describe.cpp`、`statechart.cpp`、`scope_exit.cpp` 的先例。`hof`、`units` 的 CI 证明
  并非所有库都会出问题。而 `generated/exports/json.inc` 重新导出了
  `boost::describe::*`，所以 json 加 describe 恰好是高风险组合。
- 本机只有 9 GB 内存，按 docs/architecture §10 的预算构建，不做全树并行。

## 2. 范围

### 2.1 库名规范化

| 输入 | 解释 | 备注 |
|---|---|---|
| `char_conv` | `charconv` | |
| `cicular_buffer` | `circular_buffer` | |
| `bitmap` | `bimap` | 已确认（§9 Q1） |
| `conversion` | Boost.Conversion（`polymorphic_cast`、`polymorphic_downcast`、`polymorphic_pointer_cast`、`polymorphic_pointer_downcast`、`implicit_cast`） | 与 `convert` 不是同一个库；本轮新增模块（§5.3） |
| `test` | Boost.Test | 两种形态，独立工程 |
| `context`、`coroutine2` | 未声明的库（coroutine2 依赖 context） | 已确认：只记为不支持，不在本次接入（§9 Q2） |

### 2.2 不在范围

- 列表之外的库，以及这些库的平铺测试。
- 修改生成器或导出策略。测试暴露出的导出缺口按既有流程（`tools/export_policy.toml`、
  补丁）另行修复，测试中先以 `// GAP:` 注释加 `coverage.toml` 的 `gap` 状态记录，
  不在测试里绕开。

## 3. 目录与命名

```text
tests/
├── test_assert.hpp            保留，平铺测试与迁入的 smoke.cpp 继续使用
├── json/
│   ├── smoke.cpp              原 tests/json.cpp，git mv 迁入
│   ├── api_value.cpp          value/object/array/string 的完整操作
│   ├── api_parse_serialize.cpp parser/stream_parser/serializer/parse_options
│   ├── api_pointer.cpp        JSON Pointer、at_pointer、set_at_pointer
│   ├── examples.cpp           example/*.cpp + 文档片段
│   ├── custom_types.cpp       value_from/value_to/parse_into 的自定义类型（tag_invoke 形态）
│   ├── include_describe.cpp   BOOST_DESCRIBE_STRUCT 驱动的转换（视 P0 结论，§4.4）
│   └── coverage.toml          示例 → 用例 / 排除理由
├── dll/                       独立工程（§5）
│   ├── mcpp.toml
│   ├── plugin_sum.cpp …
│   └── coverage.toml
└── …
```

- 文件名只用小写字母、数字和 `_`。测试名形如 `json/api_value`，CI 日志和
  `mcpp test json/` 都能直接定位。
- 一个文件对应一个主题，不对应一个示例。callable_traits 有 37 个示例，每个只有几行，
  逐个成文件会让编译开销翻 37 倍（每个 TU 都要加载一次 BMI）。示例以用例为单位收进
  `examples_<group>.cpp`，用例名就是上游的示例名。
- 文件头注释写明：被测模块、覆盖的上游示例路径、include 与 import 的形态及原因。
  这沿用现有测试的注释风格。

## 4. 方案

### 4.1 测试框架：doctest（`compat.doctest`）

根 `mcpp.toml` 在生成标记之外新增：

```toml
[dev-dependencies]
"compat.doctest" = "2.4.12"
```

每个测试文件是一个独立程序，因此每个文件自己定义 `main`：

```cpp
// tests/json/api_value.cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.json;

TEST_CASE("object: emplace / at / contains") {
    boost::json::object o{{"a", 1}};
    CHECK(o.contains("a"));
    CHECK_EQ(o.at("a").as_int64(), 1);
    CHECK_THROWS_AS(o.at("missing"), std::out_of_range);
}
```

在 `.tmp/` 的最小工程上实测（mcpp 2026.11.5.1，gcc 16.1.0 与 llvm 22.1.8）：

- `#include <doctest/doctest.h>`、`import std;` 和对一个具名模块的 import 写在同一个 TU 中，
  两个工具链都能编译通过。include 必须写在所有 import 之前。
- 失败的用例使程序以 1 退出，`mcpp test` 判为 FAIL；表达式两侧的自定义类型通过
  `operator<<` 打印出值。
- `mcpp test json/api_value -- -tc="object*"` 可以只跑部分用例。
- `mcpp build` 不会解析 `[dev-dependencies]`，下游消费者不会引入 doctest。

约定：

- `TEST_CASE` 名称即 `coverage.toml` 里的用例名。示例移植的用例以上游示例名命名，例如
  `TEST_CASE("example: pretty")`；文档片段以 section 命名，例如 `TEST_CASE("doc: tag_invoke")`。
- 同一文件内按主题用 `TEST_SUITE` 分组，需要共享状态时用 `SUBCASE`，不用全局变量。
- 自定义类型的值打印用 `operator<<` 或特化 `doctest::StringMaker`，写在
  `custom_types.cpp` 内，这本身也是对该类型可用性的检查。
- 检查不依赖 `assert`，所以 `--profile release`（`NDEBUG`）下同样生效。
- 迁入的 `smoke.cpp` 保持原样（`assert` 形态），不改写成 doctest，以免迁移提交混入行为变化。
- doctest 头文件与 Boost 模块的组合会在 P0 中实测，以真实的重模块（asio、geometry）为准；
  上面的探针只覆盖了 `import std` 和一个小模块。
- `tests/test/` 是 Boost.Test 自身的测试，不使用 doctest。两个子工程（§5）若需要 doctest，
  各自在 `[dev-dependencies]` 中声明。

### 4.2 各类文件的职责

| 文件 | 内容 | 必须 |
|---|---|---|
| `smoke.cpp` | 原测试原样迁入，只改 include 路径 | 是 |
| `api_<topic>.cpp` | 按官方参考文档的章节补齐公开 API：构造/赋值/比较/迭代/错误路径/异常/`constexpr` 可用性 | 是 |
| `examples[_<group>].cpp` | 官方示例的移植，每个示例一个 `TEST_CASE`，`main()` 改为用例体，标准输出改为断言 | 是 |
| `custom_types.cpp` | 自定义类型套件（§4.3） | 库有定制点时必须 |
| `include_<topic>.cpp` | 纯 include 形态，用于宏注册的定制点（§4.4） | 视情况 |

示例移植的原则：保留上游示例的逻辑与数据，把输出到 `std::cout` 的内容改写为对期望值的
检查。上游示例自带期望输出的（注释或文档中给出），以它为准；没有给出的，在本地跑一次
上游原版，取其输出作为期望值，并在用例注释中注明。

### 4.3 自定义类型套件

每个有定制点的库，`custom_types.cpp` 至少覆盖下表列出的定制点，每个定制点同时给出
正向用例（自定义类型能用）和负向的编译期判定（未定制的类型被正确拒绝，用 `requires`
表达式或该库自己的 trait 做 `static_assert`，不写编译失败的测试）。

| 库 | 定制点（必须覆盖） |
|---|---|
| asio | 自定义 completion token（`async_result` 特化）、自定义 executor、`async_compose` 写的组合操作、自定义 `buffer` 序列类型、`associated_allocator`/`associated_executor` 特化 |
| beast | 自定义 `Body`（满足 `is_body`，带 reader/writer）、自定义 `Fields`、自定义 `DynamicBuffer` |
| bloom | 自定义元素类型配合自定义 hash、自定义 subfilter |
| cobalt | 自定义 awaitable（`await_ready/suspend/resume`）、`promise`/`generator`/`task` 承载用户类型、自定义 executor |
| container | 自定义 allocator（`allocator_traits` 路径）、不可拷贝/仅可移动类型、递归容器（`vector<Self>`）、`flat_map` 自定义比较器、`small_vector`/`static_vector` 的 options |
| decimal | 用户类型与 decimal 的混合运算边界（显式转换）、`std::formatter`/流输出、`numeric_limits` |
| geometry | 自定义 point/box/linestring/polygon：手写 `traits::tag/coordinate_type/coordinate_system/dimension/access` 特化；宏注册形态见 §4.4；自定义坐标类型 |
| describe | 结构体/类（含基类、私有成员）、枚举，以及基于 describe 的通用 `==`、`hash_value`、打印、`to_tuple`（全部 include 形态） |
| hash2 | 用户类型的 `tag_invoke(hash_append_tag, …)`、基于 describe 的类型、自定义哈希算法（满足 HashAlgorithm 概念）、自定义 flavor |
| json | `tag_invoke(value_from_tag/value_to_tag, …)`、`is_described_class` 路径（§4.4）、`parse_into` 到用户类型、自定义 `storage_ptr`/memory_resource |
| math | 自定义 policy、以用户实数类型（如包装 `double` 的 `strong_real`）实例化分布与特殊函数、自定义 tools（`newton_raphson_iterate` 用的函子） |
| mqtt5 | 自定义 stream 层（满足 next_layer 要求的内存流），无需真实 broker |
| regex | 自定义 `regex_traits`、自定义字符类型的 `basic_regex`（在上游支持范围内）、自定义迭代器类型上的 `regex_search` |
| test | `boost_test_print_type` 自定义打印、自定义 fixture、数据驱动用例的自定义数据集 |
| unordered | 自定义 hash 与 equal（含透明 `is_transparent`）、自定义 allocator、用户 key 的 `hash_value` 走 `boost::hash`、`concurrent_flat_map` 存放用户类型 |
| url | 自定义 grammar rule（满足 Rule 概念）、自定义 `string_token` |
| uuid | 自定义 UniformRandomNumberGenerator 喂给 `basic_random_generator`、name generator 的命名空间 |
| variant2 | 用户类型（含不可移动、抛异常的构造）作为备选项，`visit` 用户 visitor，`valueless` 语义（variant2 的 never-empty 保证） |
| algorithm | 用户类型配合自定义谓词/比较器（`clamp`、`is_palindrome`、`apply_permutation`、搜索器） |
| any | 用户类型（含大对象与小对象优化边界）、`basic_any` 自定义缓冲区大小、`unique_any` 仅可移动类型 |
| bimap | 用户 key/value 类型、tagged 视图、自定义集合类型（`unordered_set_of` 带自定义 hash） |
| callable_traits | 用户类的成员函数（各种 cv/ref/noexcept 限定）、函数对象、函数指针 |
| charconv | 无用户类型定制点；补齐各进制、格式、错误码（无需 `custom_types.cpp`） |
| circular_buffer | 用户元素类型（非平凡析构、仅可移动）、`space_optimized` 版本 |
| compat | `function_ref`/`move_only_function` 承载用户可调用对象，`bind_front/back`、`invoke` 作用于用户成员指针 |
| container_hash | 用户类型的 `hash_value` 友元、`boost::hash` 特化、基于 describe 的类型（§4.4）、范围类型与 tuple-like 类型的自动支持 |
| convert | 用户类型经流转换器（`operator<<`/`operator>>`）、自定义 converter |
| core | `empty_value` 承载用户类型、`span` 包装用户容器、`allocator_access` 作用于自定义 allocator、`ref`、`noinit_adaptor` |
| crc | 自定义参数的 `crc_optimal<…>`/`crc_basic<…>`（官方示例的形态） |
| dll | 用户类型跨插件边界（导出工厂函数返回用户接口的实现） |
| dynamic_bitset | 自定义 `Block` 类型（`unsigned char`/`unsigned long long`）、自定义 allocator |
| conversion | 用户类层次上的 `polymorphic_cast`/`polymorphic_downcast`，`polymorphic_pointer_cast`/`polymorphic_pointer_downcast` 作用于原始指针、`std::shared_ptr`、`boost::shared_ptr` 和用户自定义智能指针（经 `pointer_cast` 定制点），`implicit_cast` 作用于用户转换构造/转换运算符 |

### 4.4 include 与 import 混用的处理

宏注册的定制点必须 include 头文件，而用户 TU 又要 import 模块。处理顺序如下，每个
受影响的库在 P0 中实测后落定，结论写进对应文件的头注释：

1. **混用可行**（clang 与 gcc 都通过）：直接在 `custom_types.cpp` 中 include 宏头文件并
   import 模块。
2. **混用在 gcc 上冲突**：`custom_types.cpp` 改用不依赖宏的等价形态，留在 import 侧，
   例如 geometry 手写 traits 特化（官方文档同时给出了这两种写法），json 用 `tag_invoke`。
   宏形态单独放进 `include_<topic>.cpp`，整个 TU 纯 include。先例是
   `hof_include.cpp`、`units_include.cpp`。
3. **只有 gcc 守卫可行**（`statechart.cpp` 的模式）：这是最后手段，因为 gcc 腿实际没有测到
   模块。只有在 1、2 都不可行时才采用，并在 `coverage.toml` 里把该用例标为 `gcc-header-only`。

describe 本身没有模块，始终走纯 include 形态。它与 json、hash2、container_hash 组合的
用例，按上述顺序判定。

### 4.5 官方示例的收录规则

收录的示例：自包含、确定性、秒级完成，不需要外部资源。以下几类排除，排除理由写进
`coverage.toml`：

| 排除类别 | 例子 | 替代 |
|---|---|---|
| 访问外部网络 | beast `http/client/sync` 访问真实主机，mqtt5 的 `hello_world_over_tcp` 连 broker | 改写为 127.0.0.1 回环加端口 0 的同进程客户端/服务端对（asio echo、beast http、websocket 均可），超时由用例自己设置 |
| 依赖未 vendor 的库或 SDK | TLS/OpenSSL（beast ssl、mqtt5 tls）、ICU（regex `icu_example`）、MFC、Python（cobalt `python.cpp`）、fmtlib（describe 的 fmtlib 示例）、Qt/wxWidgets（geometry） | 不替代 |
| 长时间运行的服务或交互程序 | asio 的 `chat`/`http` server 主循环、读 stdin 的程序 | 抽取其中的核心组件做用例，例如 http request parser |
| 基准与大数据 | json `parse_into_canada`/`citm_catalog`、math 的精度表生成 | 用小数据替代，或者排除 |
| 依赖不支持的库 | context、coroutine2（§9 Q2，记为 `unsupported`）、asio 的 `spawn`（依赖 context） | 不替代 |
| 平台专属 | asio `windows/*`、`local`（Unix 域套接字）在 Windows 上 | 在用例内按平台跳过，计入 `skip-on` |

文件系统只能在临时目录下使用。多线程示例（asio thread pool、cobalt thread、unordered
concurrent）允许，但必须带超时，避免 CI 卡死。

对 asio、geometry、math、test、beast、decimal 这几个巨大的库，「完整」的判定标准是：
每个示例要么有对应的用例，要么有一条排除记录，不能遗漏不记。数量上不设下限。

### 4.6 覆盖清单 `coverage.toml` 与检查

```toml
# tests/json/coverage.toml — generated by tools/ci/gen_test_coverage.py from boost_1_92_0.tar.gz, then edited
upstream = "1.92.0"

[[example]]
path   = "libs/json/example/pretty.cpp"
case   = "json/examples:example: pretty" # <test name>:<TEST_CASE name>

[[example]]
path   = "libs/json/example/parse_into_canada.cpp"
status = "excluded"
reason = "benchmark over a 2 MB fixture not vendored"

[[snippet]]                               # 文档片段：以 qbk/adoc 中的 [section] 为单位
doc    = "libs/json/doc/qbk/conversion/custom.qbk#tag_invoke"
case   = "json/custom_types:doc: tag_invoke"
```

- `tools/ci/gen_test_coverage.py <archive> <lib>`：从发布包列出示例文件与文档片段的
  section，生成骨架（全部标 `todo`），已有条目保留不动。它只在初始化和升级 Boost 时运行，
  归档包放在 `.tmp/`，校验 `SOURCE.md` 记录的 SHA-256。
- `tools/ci/check_test_coverage.py`：不需要归档包，只检查仓库内的一致性：
  `todo` 数为零；`case` 指向的测试文件存在，且文件里有同名的 `TEST_CASE("…")`；
  `excluded`/`gap` 必须带 `reason`；有定制点的库（§4.3 的表固化在脚本里）必须有
  `custom_types.cpp`。
- CI：在 `tests.yml` 中作为一个很轻的步骤加到 `examples` job 之前，或者单独起一个 job
  （只依赖 Python，几秒钟即可完成）。

文档片段以 section 为粒度，不以代码块为粒度，以免清单过细、无法维护。

### 4.7 迁移现有测试

- 用 `git mv tests/<lib>.cpp tests/<lib>/smoke.cpp` 迁移，内容只把
  `#include "test_assert.hpp"` 改为 `#include "../test_assert.hpp"`。
  `test_utf.cpp`、`test_included.cpp` 进入 `tests/test/`（§5.2）。
- `container_hash.cpp` 原本和 `container` 同为前缀匹配，迁移后各在各的目录，不再混淆。
- 其他文件和文档中引用旧路径的地方（`hof.cpp`、`units.cpp`、`openmethod.cpp`、
  `scope_exit.cpp`、`statechart.cpp` 的「describe.cpp 先例」注释，`docs/usage*.md`）
  一并改为新路径。
- 测试名改变：`json` 变为 `json/smoke`。CI 没有按测试名过滤，所以不受影响。

## 5. 独立测试工程

### 5.1 机制

根 `mcpp.toml` 在生成标记之外新增：

```toml
[test]
discover = ["tests/**/*.cpp", "!tests/dll/**", "!tests/test/**"]
```

子工程 `tests/<lib>/mcpp.toml`：

```toml
[package]
name    = "boost-m-tests-dll"
version = "0.0.0"

[dependencies]
"mcpplibs.mwrap.boost" = { path = "../..", default-features = false, features = ["dll", "filesystem"] }

[test]
discover = ["*.cpp"]          # 源文件仍在 tests/<lib>/ 下（P0 验证相对 glob 的语义）
```

CI 在 `Test libraries` 之后加一步，遍历 `tests/*/mcpp.toml`，进入每个目录运行
`mcpp test --toolchain …`。**不采用**根清单的 `[workspace] members`：根包会被发布和打包
（`package-source.sh`、`mcpp-index`），在已发布的清单中引入 workspace 的影响没有验证过，
而显式遍历的成本只是 CI 里的几行。

子工程只激活自己需要的 feature，避免重复编译整棵 Boost；全局 BMI 缓存
（`--cache global`）能否在根工程与子工程之间复用，在 P0 中测量。

### 5.2 用到独立工程的库

| 库 | 原因 | 内容 |
|---|---|---|
| dll | 官方教程 1～9 的核心是构建插件共享库并在运行时加载 | `[targets.<plugin>] kind = "shared"` 构建插件；测试程序通过 `boost::dll::import_symbol` 等加载；`load_self`/`static_plugin` 不需要插件 |
| test | `unit-test-framework` 的编译形态和 `included` 头文件形态不能链接在同一程序中；根工程测试不激活该 feature | 两个子工程：`tests/test/utf/`（`features = ["unit-test-framework"]`，迁入 `test_utf.cpp`，加上官方示例）、`tests/test/included/`（迁入 `test_included.cpp`），去掉现有的 `MCPP_FEATURE_*` 切换 |

其余库（含 asio、beast、cobalt、mqtt5）都留在根工程：它们需要的只是 import 与回环
网络，不需要另外的 feature 或 define。

### 5.3 conversion 模块化（本轮新增的封装层改动）

原因见 §9 Q4。改动如下：

| 位置 | 改动 |
|---|---|
| `tools/boost_common.py` | `LIBS_T1A` 加入 `conversion`；新增一张显式映射 `EXTRA_TOP_HEADERS = {"conversion": ["implicit_cast.hpp", "polymorphic_cast.hpp", "polymorphic_pointer_cast.hpp"]}`，由 `resolve_headers_heuristic`（头文件集合）、`build_file_to_lib`（归属）与 `_lib_of_include`（依赖边）三处共同读取，三者必须一致，否则排序和归属会彼此矛盾 |
| `generated/meta/libs.json` | `gen_exports.py --scan` 重新生成，新增 `conversion` 一行 |
| `src/conversion.cppm` | `gen_exports.py --emit-cppm` 生成草稿后手工维护，与其他门面同形；GMF 只 include 这三个头文件 |
| `generated/exports/*` | 全量重新生成。先到先得的归属取决于处理顺序，不能只生成 conversion。预期：`conversion.inc` 导出五个函数模板；`range.inc` 去掉 `polymorphic_cast`、`polymorphic_downcast`，`graph.inc` 去掉 `implicit_cast`，两者的 `.deps` 增加到 conversion 的边。`import mcpplibs.mwrap.boost.range;` 的使用者经 `export import` 仍能看到这两个名字，API 不变 |
| `mcpp.toml` | `gen_features.py` 重新生成 `[features.conversion]` 与基础源文件列表 |
| `.github/workflows/tests.yml` | `BOOST_GROUP_A` 加入 `conversion`，否则「Validate feature-group coverage」会拒绝运行 |
| 文档 | `docs/architecture*.md` 与 `docs/usage*.md` 的模块计数（116 → 117）及库清单；`README*` 若列出计数也一并修改 |
| 测试 | `tests/conversion/`（§6）；`tests/range/`、`tests/graph/` 之外的既有测试不受影响 |

不纳入的部分：`boost/cast.hpp` 已被弃用，并且会引入 numeric_conversion。上游自带的
`boost_conversion.cppm` 也不包含它。

验证：`gen_exports.py --check`、`gen_features.py --check`、审计脚本以及
`tools/ci/verify_generated.py` 都通过；range、graph 的 smoke 测试保持通过。生成器使用
`.xlings.json` 固定的 llvm 23.1.3，并遵守 docs/architecture §10 的资源预算（单线程 AST 生成）。

## 6. 各库计划

「示例」一列是 1.92.0 发布包中 `example/` 下的文件数，以及示例所在的文档。

| 库 | 示例 | 计划文件（除 `smoke.cpp`、`coverage.toml` 外） | 备注 |
|---|---|---|---|
| asio | 240 | `api_io_context`、`api_timers`、`api_buffers`、`api_strand_executors`、`api_sockets_loopback`、`api_coroutines`（C++20 `co_spawn`/`awaitable`、`deferred`、`parallel_group`、`experimental::awaitable_operators`）、`examples_tutorial`、`examples_echo`、`examples_operations`、`custom_types` | `spawn` 依赖 context，排除；http server 只抽取 request parser |
| beast | 70 | `api_http_message`、`api_http_parser_serializer`、`api_websocket_loopback`、`api_buffers`、`examples_http`、`examples_websocket`、`custom_types` | ssl 全部排除；client 示例改为回环 |
| bloom | 4 | `api_filter`、`examples`、`custom_types` | serialization 示例走 std 流 |
| cobalt | 12 | `api_promise_task_generator`、`api_channel`、`api_race_join`、`examples`、`custom_types` | `python.cpp`、`http.cpp`（ssl）排除；`echo_server` 改回环 |
| container | 20 | `api_vectors`（vector/devector/small/static/stable）、`api_flat_tree`、`api_deque_list_slist`、`api_string`、`api_pmr`、`examples`、`custom_types` | `doc_custom_*` 的 options 示例全部收录 |
| decimal | 46 | `api_arith_cmp`、`api_cmath`、`api_charconv_format`、`api_literals_limits`、`examples`、`custom_types` | 示例中以基准为主的文件排除 |
| geometry | 202 | `api_core_geometries`、`api_algorithms_measure`（area/length/distance/perimeter）、`api_algorithms_relate`（within/intersects/relate）、`api_algorithms_setops`、`api_io_wkt`、`api_rtree`、`api_srs_strategies`、`examples_core`、`examples_algorithms`、`examples_rtree`、`custom_types`、`include_register` | `with_external_libs/*` 排除；`geometries/register` 与 `adapted` 走 §4.4 |
| describe | 16 | `include_members_bases`、`include_enums`、`include_examples`、`include_custom_types`（全部纯 include） | fmtlib 两例排除；json/serialization 示例与 json、serialization 交叉，按 §4.4 |
| hash2 | 11 | `api_algorithms`（fnv/siphash/xxhash/md5/sha1/sha2/ripemd/hmac）、`api_hash_append`、`examples`、`custom_types`、`include_describe` | `md5sum`/`hash2sum` 改为对内存数据计算 |
| json | 12 | `api_value`、`api_parse_serialize`、`api_pointer`、`api_conversion`、`examples`、`custom_types`、`include_describe` | 见 §3 示例 |
| math | 171 | `api_special_functions`、`api_distributions`、`api_tools_roots_minima`、`api_quadrature`、`api_interpolators`、`api_constants`、`api_ccmath`、`examples_*`（按主题 3～5 个）、`custom_types` | 收录规则对 math 最严格：精度表、`dot_net_example` 排除 |
| mqtt5 | 11 | `api_client_config`、`api_properties_codes`、`custom_types`（内存流驱动 connect/publish/subscribe） | 所有 hello_world 均需要 broker，按 `custom_types` 的内存流改写其流程 |
| regex | 22 | `api_match_search_replace`、`api_iterators`、`api_partial`、`api_syntax_flags`、`examples`、`custom_types` | icu、mfc 排除 |
| test | 143 | 子工程，§5.2 | 示例按 suite/fixture/data-driven/decorator/floating-point 分组 |
| unordered | 3 | `api_node_based`、`api_flat`（flat/node map/set）、`api_concurrent`、`api_erase_if_visit`、`examples`、`custom_types` | 示例少，覆盖以参考文档章节为主 |
| url | 26 | `api_parse_views`、`api_modify`、`api_encoding`、`api_grammar`、`api_segments_params`、`examples`、`custom_types` | `router`、`file_router` 收录核心逻辑 |
| uuid | 文档 | `api_generators`（random/name/string/nil/time-based v1/v6/v7）、`api_io_hash`、`examples_doc`、`custom_types` | |
| variant2 | 文档 | `api_variant`、`api_visit`、`examples_doc`、`custom_types` | |
| algorithm | 5 | `api_cxx11_17`（all_of…、`apply_permutation`）、`api_searching`（BM/BMH/KMP）、`api_string_algo`、`api_misc`（clamp/hex/is_palindrome/gather/minmax）、`examples`、`custom_types` | string_algo 是 algorithm 模块的一部分，已有测试已涉及 |
| any | 文档 | `api_any`、`api_basic_unique_any`、`examples_doc`、`custom_types` | |
| bimap | 27 | `api_views`、`api_collection_types`、`examples`、`custom_types` | `bimap_and_boost/*` 跨库示例按库是否支持来判定 |
| callable_traits | 37 | `api_queries`、`api_transforms`、`examples`、`custom_types` | 全部为编译期检查，37 例全收 |
| charconv | 文档 | `api_from_chars`、`api_to_chars`、`examples_doc` | 无定制点 |
| circular_buffer | 5 | `api_buffer`、`api_space_optimized`、`examples`、`custom_types` | `bounded_buffer_comparison` 是基准，只收录正确性部分 |
| compat | 文档 | `api_function_ref_move_only_function`、`api_bind_invoke_misc`、`examples_doc`、`custom_types` | |
| container_hash | 7 | `api_hash_value`、`examples`、`custom_types`、`include_describe` | |
| convert | 7 | `api_converters`（stream/lexical_cast/strtol/spirit/charconv）、`examples`、`custom_types` | `default_converter_fail` 是编译失败示例，排除 |
| core | 文档 | 按 qbk 章节分为 3～4 个 `api_*`、`custom_types` | 约 40 个组件，以参考文档的章节为清单 |
| crc | 1 | `api_crc`、`examples`、`custom_types` | |
| dll | 27 | 子工程，§5.2 | mangled（`import_class`/`smart_library`）是实验性功能，视平台排除 |
| dynamic_bitset | 3 | `api_bitset`、`examples`、`custom_types` | |
| conversion | 文档（`doc/conversion.qbk`）、`libs/conversion/test/*.cpp` | `api_casts`、`examples_doc`、`custom_types` | 先完成 §5.3 的模块化；import 形态 |
| context / coroutine2 | 27 / 5 | 只有 `coverage.toml`，全部记为 `unsupported` | 不支持（Q2） |

## 7. 成本与风险

| 项 | 风险 | 对策 |
|---|---|---|
| CI 时长 | 34 个库由 34 个程序变为约 170～200 个，每个都在 10 条腿上编译；asio/beast/geometry/math 的 TU 很重 | P0 测量单个重 TU 的编译时间与内存，按结果控制每个库的文件数（宁可单个文件大一些）；超出预算时，深度测试只在一组中运行（§9 Q3） |
| gcc 的 include 与 import 混用 | 自定义类型套件可能被迫降级 | §4.4 的三级处理，P0 先行 |
| 导出缺口 | 新 API 测试暴露模块漏导出的实体 | 记录为 `gap`，另开修复，不在测试里绕开 |
| 回环网络 | CI 宿主（尤其 Windows/macOS）的防火墙与端口 | 只绑定 127.0.0.1 的端口 0，每个用例都设超时；不稳定的用例以 `skip-on` 记录 |
| 示例期望值 | 上游示例只打印结果，不做断言 | 期望值来源写在用例注释里（§4.2） |
| 浮点结果（math、geometry、decimal） | 跨平台的 ulp 差异 | 一律用容差比较，不做精确比较 |

## 8. 实施顺序

| 阶段 | 内容 | 退出条件 |
|---|---|---|
| P0 验证 | ① doctest 头文件加重模块（asio、geometry）在 gcc 16 / clang 22 上的组合；gcc 16 / clang 22：json+describe、geometry 注册宏、hash2+describe、container_hash+describe 的 include 与 import 混用；② 子工程 `[test] discover = ["*.cpp"]` 的相对 glob 语义；③ `kind = "shared"` 插件加 dll 加载；④ 单个重 TU（asio/geometry）的编译时间与峰值内存；⑤ 子工程与根工程的 BMI 缓存复用 | 结论写进一篇 `-analysis` 记录 |
| P1 基础 | conversion 模块化（§5.3）；根清单的 `[dev-dependencies]`（doctest）与 `[test] discover`；迁移 34 个 smoke；`gen/check_test_coverage.py` 与全部 `coverage.toml` 骨架 | `--check` 类检查全部通过，根 `mcpp test` 全绿，测试名为新形式 |
| P2 小型库 | algorithm any bimap callable_traits charconv circular_buffer compat container_hash convert core crc dynamic_bitset uuid variant2 bloom describe conversion | 这些库的 `coverage.toml` 中 `todo` 为零 |
| P3 中型库 | container decimal hash2 json regex unordered url | 同上 |
| P4 重型库 | asio beast cobalt mqtt5 geometry math | 同上，CI 时长在预算内 |
| P5 子工程 | dll、test | 子工程在 CI 中全绿 |
| P6 收尾 | CI 接入覆盖检查；更新 `docs/architecture*.md` 的 `tests/` 描述 | 本记录标为 `landed` |

每个阶段一个或几个 PR，按库拆分，便于 review。

## 9. 问题与答复

已确认（2026-11-09，用户答复）：

- **Q1** `bitmap` 是笔误，指 `bimap`。
- **Q2** `context`、`coroutine2` 只在 `coverage.toml` 中记为 `unsupported`，不在本次接入。
- **Q3** CI 时长先在 P0 测量再决定。若超出预算，深度测试只在一组中运行，具体机制
  （例如由 CI 传入 `discover` 的排除规则或环境变量）在 P0 的 analysis 记录中给出。
  预算内则保持现状，两组都跑全部测试。
- **Q5** 接受 §4.5 的覆盖规则：每个示例要么映射到一个用例，要么记录排除理由，不设数量下限；
  网络示例改为回环。
- **Q4** `conversion` 没有模块。调查结论（2026-11-09）：这不是技术限制，而是生成器的
  头文件启发式漏掉了它。
  - `tools/boost_common.py` 的 `resolve_headers_heuristic` 只收 `boost/<lib>/**.hpp`（排除
    `detail/`）和 `boost/<lib>.hpp`。conversion 的公开头文件是顶层的
    `boost/implicit_cast.hpp`、`boost/polymorphic_cast.hpp`、`boost/polymorphic_pointer_cast.hpp`，
    `boost/conversion/` 下只有 `detail/config.hpp`，所以得到空集，被
    [2026-09-08 汇总](2026-09-08-consolidated-design.md) §2.2 归为「无头 stub」。
  - 这几个头文件没有面向用户的宏，只有函数模板。上游 1.92 自带
    `libs/conversion/modules/boost_conversion.cppm`，导出的正是这三个头文件；上游的模块开关
    只在定义 `BOOST_USE_MODULES` 时生效，封装层不定义它，因此不冲突。
  - `build_file_to_lib` 也按路径前缀判定归属，这三个文件因此被记为 `shared`，由先到先得的
    规则归给了别的库：`range.inc` 导出了 `polymorphic_cast`、`polymorphic_downcast`，
    `graph.inc` 导出了 `implicit_cast`；`polymorphic_pointer_cast`、
    `polymorphic_pointer_downcast` 没有任何模块导出。
  - 在 `upstream/boost/libs/` 中，除了刻意排除的库之外，被这条启发式漏掉的只有 conversion
    一个（describe、openmethod、scope_exit 是 C1 的有意降级）。
  - 修复方向：在 `libs.json` 登记 conversion 的三个头文件，加入 `LIBS_T1A`，并让
    `build_file_to_lib` / `_lib_of_include` 认得这几个顶层文件（一个显式的「库 → 额外顶层
    头文件」映射）。之后重新生成：range、graph 不再导出这三个名字，改为通过 `.deps`
    中的 `export import …conversion` 边提供，使用者看到的 API 不变。`boost/cast.hpp`
    已弃用且引入 numeric_conversion，按上游模块的做法不纳入。这是封装层改动，需要走
    `gen_exports.py`、`gen_features.py`、CI 分组和文档计数的完整流程，应另写一篇记录；
    做完之后 conversion 的测试改为 import 形态。
  - **答复**：本轮一起做，方案见 §5.3。

- **Q6**（用户追加）测试框架：不自写，直接用 mcpp index 收录的 doctest，见 §4.1。

## 10. 否决的方案

- **每个官方示例一个测试文件。** 编译开销与 BMI 加载次数乘以示例数，callable_traits
  一个库就要 37 个程序，而 CI 要在 10 条腿上各编译一次。
- **自写一个测试框架（原方案中的 `tests/common/mtest.hpp`）。** 用户否决：mcpp index
  已收录 doctest，它是单头文件，没有需要编译的 main 库，能直接满足多用例、失败不中断、
  值打印、用例过滤的需求，自写只会多一份需要维护的代码。
- **用 Boost.Test 作为测试框架。** 它有两种互斥的形态，必须二选一并通过 feature 控制。
  每个 TU 都要 include Boost 头文件，又要 import 模块，正好落入 gcc 的 ODR 冲突区，
  而被测的 Boost.Test 本身也成了所有测试的前提。
- **每个库都建子工程。** 每个子工程都要重新解析依赖并编译它的 feature 闭包。根工程的
  基础源文件列表已经覆盖了所有库，子工程只在根工程无法表达时才有必要。
- **把官方示例目录 vendor 进仓库。** 这会违背 `upstream/` 只放裁剪后上游树的约定，
  并使仓库增大；`coverage.toml` 只记路径，归档按需下载并校验。

## 11. 实施记录（截至 2026-11-11）

分支 `test/module-test-hardening`（fork `ZheFeng7110/boost-m`），上游草稿 PR
[mcpplibs/boost-m#5](https://github.com/mcpplibs/boost-m/pull/5)。本节记录做到哪里、实际怎么做的、
与上文设计不一致的地方，以及测试中发现的封装层问题。

### 11.1 进度

`python3 tools/ci/test_coverage.py check` 的统计（covered：映射到用例；excluded：写明理由的排除；
gap：已知的封装层缺口）：

| 状态 | 库 |
|---|---|
| 完成（`todo` 为零，本地 gcc 16 全绿） | algorithm 36、any 2、asio 229+25、beast 38+35、bimap 40、bloom 11+1、callable_traits 36+4、charconv 18+1、circular_buffer 5、cobalt 42+4、compat 75、container 27+gap 1、container_hash 7、conversion 2、convert 15+1、core 67、crc 7、decimal 155+19、describe 33+4、dll 23+4+gap 1、dynamic_bitset 3、geometry 295+23、hash2 30、json 36+7、math 428+41+gap 6、mqtt5 16+2、regex 40+14、test 219+10、unordered 12+2、url 40+7、uuid 43、variant2 5 |
| 记为不支持（§9 Q2） | context 39、coroutine2 16 |

`todo` 已全部清零，`test_coverage.py check` 在 CI 的 `example coverage` 作业中运行。

提交（按时间）：设计记录 → conversion 模块化、`core::string_view` 导出、transform_reduce 的 include
guard（P1）→ 34 个 smoke 迁入 `tests/<lib>/` 并接入 doctest → convert、bimap、bloom/describe/
container_hash、hash2、unordered、container、regex、url、json、decimal 各库套件 → 平台无关的期望值
（MSVC 空基类 ABI、反修饰名、临时路径）→ asio → CI 修复（clang 的 `operator new` 歧义、Windows 的
`near` 宏与 POSIX 专有示例、aarch64 的 128 位 `long double`）→ beast → cobalt → mqtt5 → geometry →
math → Boost.Test → dll → CI 步骤与文档。

§11.1 早先记录的 CI 失败均已修复：clang 上 `operator new` 的歧义确是 container 导出全局
`::operator new` 的回归，改为只在 gcc 上导出，clang 在 json、geometry 的门面里以 purview 内的具名引用
使 placement 形式可达（§11.4）；decimal 的 `near` 是 Windows 头文件的宏，改名；aarch64 的
`fast128 → long double` 比较是上游在 113 位 `long double` 上的问题，该平台跳过并注明。

### 11.2 与设计不一致之处

- **工具名与子命令。** §0、§4.6 写的 `gen_test_coverage.py` / `check_test_coverage.py` 合并为
  `tools/ci/test_coverage.py`，子命令 `scan`（从发布包重建清单，校验归档 SHA）、`link`（按命名约定把
  `todo` 连到用例）、`check`（CI 用）。用例命名约定：`example: <stem>`、`doc: <page>#<section>`，
  section 在库内唯一时也可写 `doc: <section>`；doctest 的 `TEST_SUITE(type, "name")` 视为
  `doc: name`。清单里 `case` 的写法是 `<lib>/<file>:<用例名>`。
- **文件布局。** 没有严格按 `api_<topic>` / `examples_<group>` 拆分：示例移植按目录或语言版本成组
  （asio 的 `examples_cpp11/14/20.cpp`、`network_cpp11.cpp`、`network_http.cpp`……），文档页的用例
  放在 `api.cpp` / `api_overview.cpp` 等少数文件中，宁可单文件大一些（§7 的 CI 时长对策）。
- **子工程。** 只有 `tests/dll/` 与 `tests/test/utf/` 是独立工程；`tests/test/included/`
  （Boost.Test 的 header-only 形态）留在根工程。根清单为
  `discover = ["tests/**/*.cpp", "!tests/dll/**", "!tests/test/utf/**"]`。
- **dll 子工程的链接形态。** 程序和每个插件都依赖本包；mcpp 只在 ELF 上允许插件绑定到程序里静态链接的
  那一份，Mach-O 与 PE 上拒绝这种依赖图，所以 `tests/dll/mcpp.toml` 对本包写 `linkage = "shared"`
  （链接形态只由根工程决定，插件里写了会被忽略）。从程序自身导入的示例（tutorial4/5，`self.cpp`）
  要求程序导出符号，Unix 上加 `-rdynamic`。插件名不用 `shared_library::decorate()`：它对以 `lib`
  开头的名字（library1）不加前缀。
- **Boost.Test 的用例形态。** Boost.Test 没有模块，`tests/test/` 下全部是 include 形态的完整程序：
  文档示例与 `libs/test/example/` 各移植为一个程序（原 `main` 改名，harness 按 `.output` 的命令行
  运行并与 oracle 输出比对，行号与耗时归一化），`coverage.toml` 的 `case` 写作 `<file>:program`
  （`test_coverage.py` 把含 `main` 的文件视为一个名为 `program` 的用例）。`tests/test/utf/` 是编译
  形态（unit-test-framework 特性把框架编入程序）的 smoke；静态库形态定制入口与初始化函数的两页
  （`custom_main`、`custom_init`）要求以 `BOOST_TEST_ALTERNATIVE_INIT_API`（及 `BOOST_TEST_NO_MAIN`）
  重新构建框架，特性不提供这种构建配置，这两个示例以 header-only 形态运行，清单中写明。
- **conversion 之外的封装层改动** 比 §5.3 预期的多，见 §11.4；每一项都在对应提交里重新生成并通过
  `gen_exports.py --check`。

### 11.3 方法

**示例移植与输出 oracle。** 官方示例不改逻辑，由本地脚本机械移植成用例：`#include` 换成
`import std;` 与模块 import（宏所在的头文件保留 include，§4.4 的混用在 gcc 16 / clang 上实测可行），
`main` 改名为 `example_main`，`std::cout/cerr` 重定向到捕获流，argv / stdin 由用例提供。期望输出
不手写：同一份移植后的代码只用 `upstream/boost` 头文件、以 clang++ / libc++ 编译运行一次（oracle），
其输出写进用例逐字比对；oracle 跑两次，结果不同就不比对输出并在文件头说明原因（时间、线程完成顺序、
`priority_queue` 的同优先级顺序等）。需要编译库的示例给 oracle 链接本地编出的上游库
（serialization、json、regex、url、container、program_options、thread、system 的补充）。
oracle 的输出按字节读取：raw 字符串字面量无法保存 `\r`，含控制字符的输出改用转义字面量。
对示例做的改动只有几类，并都写在生成文件的头注释里：固定端口改为空闲端口、`"daytime"`/`"http"`
服务名改为端口号、秒级延时缩短、无限循环在输入结束时退出、读取进程 stdin 描述符的地方换成预先
写好内容的管道、写入 cwd 的文件改到临时目录。

**回环网络（asio，beast 同法）。** 不起子进程，全部在一个测试进程内：服务端示例在后台线程运行
（`ex_serve_async`，进程退出时结束），harness 客户端发一次请求并打印收到的内容；客户端示例连接
同一文件中已移植的、与它配对的服务端示例。harness 的连接（包括探测服务端是否已监听的那次连接）
一律保持打开：服务端看到客户端断开会从自己的线程打印异常，与主线程的输出竞争，有的协程版本还会因此
终止。只接受一个连接的服务端（beast `echo-op`）不探测，由客户端重试连接。UDP 无法探测，短暂等待后
由客户端重发。

**文档片段。** 以章节为粒度：每个带代码块的章节（qbk 的 `[section]`、adoc/md 的标题）一个用例，
或一条排除。扫描规则修正过一次：Quickbook 的代码块按任意缩进识别（asio 的文档缩进两格），
最初只认 4 格以上，漏掉了 asio、regex、convert 的若干章节；修正后这些章节都已补上用例。

**挂起与输出位置。** 网络类示例的程序带一个 doctest 监听器：Windows 上打印每个用例名，各平台上
用例运行超过 60 秒时在 stderr 报出用例名，CI 超时时能看出卡在哪里。会写文件到当前目录的示例
（geometry 的 svg 与 mapped_file、Boost.Test 的 example50 日志）在临时目录中运行，不在仓库根目录
留下文件。mqtt5 的示例连接同一进程内的最小 broker（`tests/mqtt5/broker.hpp`，实现示例用到的
CONNECT/SUBSCRIBE/PUBLISH 与 websocket 传输）。cobalt 的 `co_main` 以 `cobalt::run` 作为 task 运行。

**脚本。** 移植与生成脚本（`port_examples.py`、各库的 `regen_*.py`、asio/beast 的 harness 头文件）目前
只在本地 `.tmp/scripts/`，未提交；提交的是生成结果，每个生成文件的头注释说明了来源与改动。是否把它们
整理进 `tools/` 以便升级 Boost 时重跑，待定（§11.6）。

### 11.4 测试发现的封装层问题

都在对应提交中修复，并重新生成导出：

| 库 | 问题 | 修复 |
|---|---|---|
| algorithm、container | 上游 include guard 冲突，`transform_reduce`、`pmr::devector` 在模块里不存在 | `patches/algorithm.patch`、`container.patch` |
| convert、bimap、url、json | 命名空间作用域的 `constexpr` 对象是 TU-local，无法导出（`throw_on_failure`、`_key`/`_data`、url 的字符集与语法规则、json 的 kind 标签） | 补丁改为 `inline constexpr` / `BOOST_INLINE_CONSTEXPR` |
| regex | GMF 中内部链接的消息表在使用方看来是空串 | `regex.patch` 改为 `inline constexpr` |
| container | 全局的 placement `operator new(…, boost_container_new_t)` 在实例化点不可见 | `export_policy.toml` 以空命名空间导出 |
| convert | converter 头文件不能单独编译，被过滤掉 | policy 的 `gmf` 补上 |
| decimal | GMF 里的 `std::formatter` 偏特化对使用方不可达 | `#include <version>` 并在 purview 中具名引用 |
| asio | `experimental::use_promise` 缺失（`use_promise.hpp` 不能单独编译，被过滤掉）；`stream_errc` / `socket_errc` 只含 using 声明，生成器不收 | `libs.json` 补头文件；policy 的 `extra` |
| core | `boost::core::string_view` 没有任何模块导出 | `core.cppm` 包含 `core/detail/string_view.hpp` |

| container、json、geometry | §11.1 的 clang 回归：导出 `using ::operator new;` 后 libc++ 的 `__new/allocate.h` 调用有歧义 | 该导出改为只在 gcc 上（policy `extra` 的 `#if`）；clang 在 json、geometry 的门面中以 purview 内的具名引用使 placement 形式可达 |
| cobalt | `use_op`、`use_task`、`this_coro::*` 与 io 协议常量是 TU-local；`main.hpp` 定义的 `main` 与使用方的 `main` 冲突；`this_coro` 缺 asio 的同名对象；Windows 没有 `IPPROTO_SCTP` | `cobalt.patch`；policy 的 `gmf` 去掉 `main.hpp`；`extra`；`guard` |
| beast | `beast::errc` 是命名空间别名，生成器不收 | policy 的 `extra` |
| geometry | fusion 适配与 tuple 适配同时可见而有歧义；`boost::size` 与 `std::size` 有歧义；svg_mapper 未收入 | policy 的 `gmf`（去掉 `adapted/boost_fusion.hpp`、补 `io/svg/svg_mapper.hpp`）与 `core.drop` |
| variant | `has_result_type.hpp` 的匿名结构体使 gcc 16 在每次 rtree 使用时 ICE | `variant.patch` 给结构体命名 |
| interprocess | 创建标签、锁选项、`offset_type_alignment`、`ec_table` 是 TU-local | `interprocess.patch`（`BOOST_INLINE_CONSTEXPR`） |
| math | `float/double/long_double_constants` 与 nonfinite facet 的标志是 TU-local；`cubic_roots.hpp`、`line_search_policies.hpp` 依赖使用方带入的 `using std::sin/fabs` | `math.patch` |
| config | `boost/cstdlib.hpp` 不在任何模块中，`exit_success` 等又是 TU-local 的 `const int` | `libs.json` 把它归入 config；`config_cstdlib.patch` |
| dll | `boost::dll::fs` 以 using 指令（`using namespace boost::filesystem;`）定义，导入方看不到 using 指令，`dll::fs::path` 不可达 | policy 的 `extra` 逐个 using 声明文档用到的名字 |

记为 `gap` 而不绕开的：container 的 `doc_extended_allocators`（dlmalloc 分配器需要
`alloc_lib.c`，包没有构建它）；dll 的 `tutorial8_static`（`this_line_location()` 在上游是
`static inline`，按设计每个映像各有一份，TU-local，无法导出）；math 的 C99/TR1 页（编译库
`libboost_math_tr1` 未构建）以及四个靠 `BOOST_MATH_*_POLICY` 宏在包含头文件前
配置默认策略的示例（宏无法作用于已编译的模块）。上游文档与实现不一致的地方（charconv 十六进制解析、hash2 文档里两次
`result()`、decimal 的若干名字等）在用例注释中写明，以实现为准。

### 11.5 CI 时长（§9 Q3）

首次全量运行（run 42951552436，提交 `f09adfbb`，全部套件就位）的作业时长：

| 腿 | A 组 | B 组 |
|---|---|---|
| linux-gcc-x86_64 | 56m24s | 44m57s |
| linux-llvm-x86_64 | 40m01s | 16m32s |
| linux-gcc-arm64 | 26m59s（失败，见下） | 31m49s（失败） |
| macos-llvm-arm64 | 31m28s（失败） | 14m40s（失败） |
| windows-llvm-msvc-x86_64 | 36m48s（失败） | 36m33s（失败） |

全部通过的运行（run 42978425044，提交 `a46bfc87`）整轮约 65 分钟（含之后的 examples + template 作业），
各测试作业：

| 腿 | A 组 | B 组 |
|---|---|---|
| linux-gcc-x86_64 | 51m39s | 46m24s |
| linux-llvm-x86_64 | 34m54s | 25m57s |
| linux-gcc-arm64 | 29m19s | 30m19s |
| macos-llvm-arm64 | 56m59s | 38m53s |
| windows-llvm-msvc-x86_64 | 29m32s | 32m36s |

时间几乎都花在构建测试程序上。linux-gcc-x86_64 A 组：库构建 6.5 分钟，根工程 `mcpp test` 2049 秒
（build 1986 秒 + run 59 秒），两个子工程 15 分钟（`tests/dll` 444 秒、`tests/test/utf` 471 秒，各自重新
构建本包）；B 组 `mcpp test` 2502 秒（build 2440 秒 + run 59 秒）。linux-llvm-x86_64 A 组 `mcpp test`
1165 秒（build 1079 秒 + run 83 秒）。416 个测试程序的运行合计约 1 分钟。最长的腿约 57 分钟，在作业的
240 分钟上限之内，所以按 §9 Q3 暂不拆分测试分组；若要缩短，收益最大的是让两个子工程复用根工程的
BMI 与目标文件（目前各自重建本包）。这一轮各平台腿的失败都是平台差异（macOS 的 `struct user`、
Windows 的 `min`/`max`/`near` 宏与类型名前缀、长双精度宽度与 libm 末位、系统对访问违例的描述），
已在后续提交中修正。

### 11.6 剩余工作

§11.6 原列的 1～4 已完成：CI 失败已修复；beast、cobalt、mqtt5、geometry、math、dll、test 的套件已落地；
CI 增加了 `example coverage` 作业与 A 组的子工程步骤；`docs/architecture*.md` 已更新。提交 `a46bfc87`
的 CI 全部通过（20 项检查）。尚未解决的问题如下，已移入待办
[`todos/module-tests-open-issues.md`](todos/module-tests-open-issues.md)：

1. **windows-llvm-msvc 上的进程崩溃。** 只在这一条腿上出现，其余五条腿与本地 gcc / clang 均通过：
   asio 的 `composed_4` 与 cobalt 的文档程序以堆损坏（0xC0000374）结束进程，cobalt 文档程序每次崩在
   不同的用例（`a-minimal-executor`、`race`），math 的 `daubechies_scaling` 以 SIGSEGV 结束。目前
   `composed_4`、整个 `cobalt/doc` 与 `daubechies_scaling` 的求值在 Windows 上跳过并在源码旁注明。
   堆损坏在释放时才被检测到，确切出错的位置未定；怀疑方向是同一段 asio 内联代码（如
   `aligned_new` / `aligned_delete` 的配对、`call_stack<>::top_`）在不同模块的 GMF 中以不同配置实例化，
   需要在 Windows 主机上复现后再定。
2. **aarch64 Linux 上 `ooura_fourier_cos<double>` 返回 NaN**（结果与误差估计都是 NaN；同一用例的
   `ooura_fourier_sin` 正常）。它以 `long double` 计算节点，该平台的 `long double` 是 113 位；是上游
   问题还是封装层问题未定，`doc: fourier_integrals` 在该平台跳过这一项检查。
3. **Windows 上的 `tests/dll`。** 本包以 `linkage = "shared"` 构建成 DLL 时，包内编译源码的符号
   （`boost::filesystem::detail::path_algorithms::*`、`boost::system::error_category` 的转换运算符等）
   没有导出，测试程序链接失败；CI 在 Windows 上暂不运行 `tests/dll`（`tests/test/utf` 照常运行）。需要
   确认 mcpp 的 `windows_auto_export` 对以共享形态构建的依赖包是否生效。linux-gcc-arm64 腿的 gcc 16.1.0
   以 `aarch64-linux-musl` 静态 C 库为目标，静态程序无法加载插件，`tests/dll` 在该腿上同样不运行
   （这是平台限制，不是缺陷）。
4. 待定：是否把移植脚本整理进 `tools/`（§11.3）。
5. 子工程各自重新构建本包，A 组因此多约 15 分钟（§11.5）；可以研究复用根工程的构建产物。

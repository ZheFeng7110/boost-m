# Boost 1.92.0 迁移到 mwrap 规范

> 日期：2026-10-26  
> 状态：迁移设计与执行验收文档  
> 源仓库：`../boost-module-b1.92.0wdev`  
> 目标仓库：`mcpplibs/boost-m`

本文规定把现有 Boost C++23 模块封装迁移到 `mcpplibs.mwrap` 的步骤。迁移以源仓库
现有的 Boost 1.92.0 实现为基础，不重新设计 Boost API；模块只是上游头文件 API 的
命名模块入口。

盘点基线：源仓库分支 `b1.92.0wdev`、提交
`77c6b48f4f6a0f0564f8e4a17cbc029468299067`；目标仓库提交
`fcb6583d140e17a8459632918812eeecd2c17ea4`。规范核对的是 mcpp-index `main`
提交 `6153a5af63ed2a79543428a25fdc8101553fb36f`。本文列出的构建、发布命令和验收框
均为后续执行要求；本次仅完成仓库盘点与文档编写。

## 1. 现状盘点

源仓库目前已经完成了主要的模块化工作：

- 上游版本为 Boost 1.92.0，导入脚本锁定的官方源码 URL 为
  `https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.gz`，SHA-256 为
  `c4a3b310ddd2472416e091067166b0713be97c63f38c212c484ada022fd296ce`。
- `src/` 中有 116 个模块接口，另有 `log` 和 `unit_test_framework` 两个编译但
  只通过头文件消费的 feature。
- 库 feature 共 118 个，另有 2 个 backend profile 和 `all` 聚合；默认闭包为 49 个库。
- `deps/boost/` 保存裁剪后的 Boost 源码、头文件和编译库源码。
- `src/gen_exports/*.inc`、`.deps` 和 `scripts/meta_datas/*` 是提交到仓库的生成输入。
- `scripts/patchs/` 有 60 个适配补丁，`scripts/reapply_hand_edits.py` 负责重放。
- `tests/` 有 142 个自带 `main()` 的测试源文件，覆盖模块库、编译库和 include-only 库。
- 当前旧清单身份为 `ZheFeng7110.boost`，版本为 `1.92.0.0.0.0`，模块名为
  `boost.*`；已发布的旧身份、tag、归档和摘要必须保持不可变。

目标仓库目前仍是 `mcpplibs.mylib` 模板。应以源仓库的实现、生成物、测试和补丁为迁移
主体，再适配目标仓库的 CI、模板和文档结构。

## 2. 迁移后的公开身份

身份必须统一为下表。任何依赖、模板、文档、索引描述符和测试都使用完整身份。

| 项目 | 迁移后值 |
| --- | --- |
| GitHub 仓库 | `https://github.com/mcpplibs/boost-m` |
| 包 namespace | `mcpplibs.mwrap` |
| 包 name | `boost` |
| 完整包选择器 | `mcpplibs.mwrap.boost` |
| 根模块 | `mcpplibs.mwrap.boost` |
| 子模块示例 | `mcpplibs.mwrap.boost.optional`、`mcpplibs.mwrap.boost.filesystem` |
| 宏模块 | `mcpplibs.mwrap.boost.macros` |
| 上游公开 namespace | `boost` |
| 封装贡献许可 | Apache-2.0 |
| 上游版本 | `1.92.0` |

必须遵守以下边界：

1. 包名、模块名中不添加 `-m`。
2. 不把 `boost::optional`、`boost::filesystem` 等实体搬到
   `mcpplibs::mwrap::boost`；消费者看到的类型、重载、ADL 和函数名仍然是上游拼写。
3. `mcpplibs::mwrap::boost::macros` 只承载宏映射模块中新增的映射实体，不能替代
   `boost` namespace，也不能声称 named module 可以导出预处理宏。
4. 旧的 `ZheFeng7110.boost` 版本、旧 tag 和旧源码包不改写。迁移后的包是新的
   mwrap 身份，不是旧包的原地改名。

### 2.1 版本和 tag

首个 mwrap 发布使用上游版本本身：

```toml
[package]
namespace = "mcpplibs.mwrap"
name = "boost"
version = "1.92.0"

[package.metadata.mwrap]
upstream-version = "1.92.0"
wrapper-revision = 0
```

仓库 tag 使用独立的 `mwrap-1.92.0` 序列，源码归档命名为
`boost-m-1.92.0.tar.gz`。不能使用旧的六段版本，也不能把 `v1.92.0.0.0.0` 移到新
仓库。后续只修复封装而不升级 Boost 时，版本从 `1.92.0.1` 开始；不要发布排序等同
于 `1.92.0` 的 `1.92.0.0`。Boost 升级到 `1.92.1` 后，版本重新从 `1.92.1` 开始。

## 3. 源码和目录迁移

目标目录应采用下列结构：

```text
boost-m/
├── mcpp.toml
├── build.mcpp
├── .xlings.json
├── upstream/boost/              # 裁剪后的 Boost 源码、README、许可证
├── patches/                     # 可重放的 Boost/平台适配补丁
├── port/                        # mwrap 适配头、配置头、宏兼容入口
├── src/                         # 模块接口和封装层实现
├── generated/                   # 稳定、可复现、需要提交的生成输出
├── templates/                   # 至少一个可运行的默认模板
├── tests/                       # API、行为、头文件/模块混用和安装后消费
├── examples/
├── tools/                       # 同步、生成、审计和测试工具
└── docs/
```

迁移对应关系如下：

| 源仓库 | 目标仓库 | 要求 |
| --- | --- | --- |
| `deps/boost/` | `upstream/boost/` | 只放上游和明确记录的裁剪结果，不混入封装代码 |
| `scripts/patchs/` | `patches/` | 修正拼写并记录每个补丁的原因、来源和适用版本 |
| `src/gen_exports/` | `generated/exports/` | 生成输出稳定后提交，并由脚本验证字节幂等 |
| `scripts/meta_datas/` | `generated/meta/` | 放置 libs.json、features.lst、profiles.lst 生成输出；人工输入留在 tools/ |
| `include/mwrap/...`（新增） | `port/include/mwrap/...` | 放宏保留头和必须的配置/兼容头 |
| `src/boost_*_extras.cpp` | `port/*.cpp` | 将 ABI/平台适配实现与模块接口分层，并显式纳入 sources |
| `examples/default_usage` | `examples/basic` | 依赖完整包身份并实际运行 |
| `scripts/` | `tools/`、`tools/generate/` | 生成与审计工具不得在模块编译阶段下载源码 |

目录搬迁后必须同步更新所有 Python、shell、补丁和 CI 路径。不得只修改 manifest
中的 `include_dirs`，留下脚本仍指向 `deps/boost` 的隐性路径。

### 3.1 上游来源记录

迁移后的 `tools/import_boost.py` 作为唯一的上游导入入口，改为写入
`upstream/boost/`。它必须保留并记录：

- 官方来源 URL、版本 tag 或 release 名称、压缩包 SHA-256；
- 保留的顶层目录和文件；
- 删除的文档、示例、CI 和构建文件清单；
- 只对明确列出的 CRLF 文件做换行归一化；
- 上游 `README.md`、`LICENSE_1_0.txt` 以及适用的 NOTICE/第三方声明。

建议新增 `upstream/boost/SOURCE.md` 保存上述元数据。原始 Boost `README.md` 和许可证
文件要以原始字节保留，不能用该说明文件代替。`.gitattributes` 应对这些字节固定的
上游文本关闭换行转换，例如加入 `upstream/** -text`，并在 Linux、Windows checkout
上核对 SHA-256。

上游实现复杂且包含编译库，因此采用 vendored 实现；不再额外依赖一个会与其重复链接
的 `compat.boost`。同一消费程序中只能有一个 Boost 实现所有者。

### 3.2 补丁重放

现有补丁分为两类并分别审计：

1. 上游头文件和库源码的最小适配，例如内部链接对象改为 `inline`、平台宏和编译库
   所需修正；
2. 模块生成或消费者适配，例如平台 guard、导出归属和额外的封装实现。

`reapply_hand_edits.py` 需要改成独立于当前 Git HEAD 的可重放流程：

- `import_boost.py` 后先应用 `patches/`；
- 生成导出清单和 `.cppm` 草稿；
- 再应用模块侧的补丁和平台 guard；
- 不使用 `git checkout` 从历史恢复源文件；
- 对已应用补丁和内容不变的文件保持幂等，并保留原始 mtime；
- 补丁锚点不存在时只允许有书面说明的兼容分支，不能静默丢弃 API。

生成工具应能在没有目标仓库历史提交、只有源码归档和工具输入的环境中解释其输出。
现有 `scripts/curated/`、`pyproject.toml`、`uv.lock`、Python 单元测试也一起迁移到
`tools/`。生成器使用完整 LLVM 23.1.3、匹配的 libclang/builtin headers 和锁定的
MinGW sysroot；普通消费端使用已提交生成物，不要求安装这些生成工具。

迁入文件以源仓库 Git 跟踪清单为准，跳过 `.git`、`target/`、`compile_commands.json`、
`__pycache__/`、下载缓存和本地工具载荷。将旧封装的 LICENSE、README 和来源提交单独
保留在 `upstream/boost-module/`，与 `upstream/boost/` 的官方来源区分；保留有用的设计
和历史发布文档及其链接。确认对应功能已迁入后，再清理目标模板的 `mylib` 占位文件。

## 4. 模块接口迁移

### 4.1 根模块和子模块

将所有：

```cpp
export module boost.optional;
export import boost.core;
```

改为：

```cpp
export module mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.core;
```

根模块改为 `mcpplibs.mwrap.boost`。`build.mcpp` 生成的聚合模块必须输出：

```cpp
export module mcpplibs.mwrap.boost;
export import mcpplibs.mwrap.boost.optional;
```

它只重新导出当前 feature 激活的模块闭包。生成文件写入 `MCPP_OUT_DIR`，不能写入
仓库内的固定 `generated/boost.cppm`，避免根包被多个消费者以不同 feature 并发构建时
互相覆盖。

每个模块继续采用“全局模块片段 include 上游头文件、模块体 `export using` 上游实体”
的结构。导出清单的实体归属必须唯一，跨库依赖写成完整 mwrap 模块 import；根模块只
聚合已经激活的公共模块。

### 4.2 API 兼容和导出审计

`gen_exports.py` 的两遍扫描和 `.deps` 依赖闭包应保留，并改成对新的上游路径和模块
前缀工作。生成器必须审计公共头文件中的：

- 类、函数、变量模板、别名模板、枚举和重载；
- 模板参数和签名中引用的跨库类型；
- friend operator、ADL 所需的自由函数；
- 经过补丁取得外部链接的公共常量和 forwarder。

`gen_audit.py` 的报告必须作为迁移门禁。任何因 libclang、平台宏或内部链接而不能导出
的实体，都要在 `docs/usage.md` 中列出替代写法并有行为测试；不能把生成器失败当成
“非公共 API”而静默 prune。当前已经通过补丁导出的 `range`、`multi_array`、
`accumulators`、`mqtt5` 常量必须逐项复验。

旧 `boost.version` 中的 `boost::BOOST_VERSION`、`boost::BOOST_LIB_VERSION` 是封装
自行创建的宏映射，不是 Boost 上游的普通 C++ 实体。按 mwrap 规范把它们迁至：

```cpp
import mcpplibs.mwrap.boost.macros;
static_assert(mcpplibs::mwrap::boost::macros::BOOST_VERSION == 109200);
```

保留 `mcpplibs.mwrap.boost.version` 作为重新导出 `.macros` 的薄入口，`version`
feature implies `macros`，映射实体只有 `.macros` 一个导出所有者。上游真正的 C++ API
保持 `boost::`；上述旧封装扩展的限定名变化必须写入消费者迁移说明。宏形式继续使用
`#include <boost/version.hpp>`，与映射常量分开测试。

## 5. feature 和平台配置

所有 feature 名称改为小写 kebab-case，同时替换所有 `implies`、生成脚本、示例、CI
和文档中的引用。需要替换的 23 个下划线名称如下：

| 旧名 | 新名 |
| --- | --- |
| `container_hash` | `container-hash` |
| `static_string` | `static-string` |
| `type_traits` | `type-traits` |
| `program_options` | `program-options` |
| `callable_traits` | `callable-traits` |
| `circular_buffer` | `circular-buffer` |
| `concept_check` | `concept-check` |
| `dynamic_bitset` | `dynamic-bitset` |
| `lexical_cast` | `lexical-cast` |
| `multi_array` | `multi-array` |
| `multi_index` | `multi-index` |
| `poly_collection` | `poly-collection` |
| `property_map` | `property-map` |
| `property_tree` | `property-tree` |
| `ptr_container` | `ptr-container` |
| `safe_numerics` | `safe-numerics` |
| `smart_ptr` | `smart-ptr` |
| `stl_interfaces` | `stl-interfaces` |
| `throw_exception` | `throw-exception` |
| `type_index` | `type-index` |
| `date_time` | `date-time` |
| `unit_test_framework` | `unit-test-framework` |
| `type_erasure` | `type-erasure` |

迁移后的 feature 约定：

- `default` 以原有 49 个库闭包为基线，改为 kebab-case；加入 `macros` 后重新计算并
  公布实际闭包，不硬编码迁移前计数；
- feature 采用 kebab-case，模块后缀继续保留 Boost 分组的下划线拼写，例如
  `type-traits` 启用 `mcpplibs.mwrap.boost.type_traits`；
- `all-modules` 替代旧 `all`，展开为迁入的 116 个接口和新增 `.macros`，共 117 个
  模块入口；生成并提交精确展开列表，不包含 `log`、`unit-test-framework` 或 CPU
backend。它表达模块集合，不能用来宣称全量聚合已通过所有编译器；
- `log` 和 `unit-test-framework` 可以单独启用以编译其库 TU，但没有对应 named module，
  消费者通过上游头文件使用；
- 纯 include-only 库不创建虚假的模块 feature，宏 API 仍通过头文件使用；
- `backend-log-ssse3` 和 `backend-log-avx2` 保持现有 profile 名称、CPU 条件和互斥
  校验；feature 只选择实现，不负责从链接图删除另一份实现。

源架构文档列出 20 个纯 include-only 库：

```text
preprocessor mpl fusion proto spirit xpressive typeof vmd phoenix parameter
metaparse function-types tti local-function msm foreach exception describe
openmethod scope-exit
```

其中前 16 个主要是宏 API，后 4 个是因编译器缺陷或宏/混用问题降级的库。它们必须在
使用文档中明确标注为 include-only。另外，`boost_common.py` 中还保留 `predef` 和
`static_assert` 两个 include-only 项，因此完整清单至少为这 22 项；迁移时统一脚本、
README 与统计口径，不能直接沿用旧文档“共 138 库”的计数。暂缓支持的 11 个库
`context/fiber/coroutine/locale/mpi/python/parameter_python/graph_parallel/compute/mysql/redis`
继续记录各自的汇编、ICU、MPI、Python、OpenCL 或 OpenSSL 条件。

feature 生成器应把库名、模块名和 feature 名分开处理，不能用简单的字符串替换破坏
`boost` 上游文件名。生成的 `[features]`、sources、`.deps` 和聚合模块必须在内容不变
时保持 mtime 不变。

feature 仍按依赖边做加法并集：某条边 `default-features = false` 只关闭该边请求的默认
集合，不能撤销另一条边启用的默认或显式 feature。验证单库选择、传递 `implies`、两条
依赖边合并、另一条边重新请求默认及互斥 profile 的诊断。启用默认集合关闭后的最小
consumer 也应证明未选中的编译库 TU 没有进入链接图。

平台配置继续覆盖 Linux GCC/LLVM、macOS LLVM 和 Windows clang-msvc 的已验证组合。
当前没有验证的 real MSVC、MinGW 或外部依赖库不能写成受支持平台。每个 target 的
宏、系统库、CPU baseline、C++ runtime 和外部 SDK 约束都要写在 manifest 或使用文档
中；请求了不可用 backend 时必须显式失败，不能静默跳过。

有两项根聚合问题必须重新验证：旧 `build.mcpp` 会在 GCC 下跳过 `sort`，这与“根模块
重新导出所有已启用公共模块”冲突；旧 clang 全量聚合会遇到 source-location 上限。
应先复测当前工具链，修复后才能声明对应组合受支持。仍受限制时，在扫描前对该根聚合
组合报出明确诊断，记录可用的分组消费范围，不能继续静默少导出。分组测试成功只证明
分组覆盖，不能算作 `all-modules` 根聚合通过。

## 6. 宏接口迁移

named module 不能导出预处理宏，Boost 的宏面必须分成两部分：

1. 新增 `src/macros.cppm`，模块名为 `mcpplibs.mwrap.boost.macros`。只把能够保持
   求值次数、类型、常量值和调用语义的对象常量或计算宏映射成 inline constexpr/函数，
   放到 `mcpplibs::mwrap::boost::macros`，并保留原宏名作为映射实体名。
2. 新增 `<mwrap/boost/macros.hpp>`，放字符串化、token 拼接、条件编译、控制流和
   依赖调用点预处理的宏。消费者必须在 import 前 include 此头文件。

不能用函数替代断言宏，也不能假设映射后的常量能在限定名中阻止宏展开。至少要有以下
测试：

- 原始头文件宏的展开和多次参数求值行为；
- `import mcpplibs.mwrap.boost.macros` 后的映射常量/函数；
- `#include <mwrap/boost/macros.hpp>` 后再 import 的宏调用；
- raw header 与普通模块的兼容混用，以及 raw header 与同名映射常量的冲突用例；
- `BOOST_VERSION` 原始宏与 `.macros` 中同名常量的分离用法。

首期宏模块至少提供 `BOOST_VERSION`、`BOOST_LIB_VERSION` 映射。include 路径优先保留
上游头中宏定义的原字节，`macros.hpp` 只作为有文档的兼容入口，不重新实现 Boost
Preprocessor。映射模块内部读取宏值后 `#undef` 同名宏再声明常量；消费端若保留了原始
宏，限定名同样会被展开，需采用分离 TU 或在记录值后显式 `#undef` 的使用方式。

## 7. manifest、许可和 README

### 7.1 `mcpp.toml`

以源仓库清单为基础重写，而不是保留模板中的 `mylib` 字段。至少包含：

```toml
[package]
namespace = "mcpplibs.mwrap"
name = "boost"
version = "1.92.0"
description = "Boost 1.92.0 C++23 named module wrapper"
license = "Apache-2.0"
authors = ["mcpp-community"]
repo = "https://github.com/mcpplibs/boost-m"

[package.metadata.mwrap]
upstream-version = "1.92.0"
wrapper-revision = 0
```

保留 vendored 头文件、库 TU、平台 flags、feature sources、`build.mcpp` 和明确的
`[lib] path`。构建期生成根模块时，`[lib].path` 应指向每个构建自己的输出目录。
默认 feature、feature 依赖和 target 条件必须与第 5 节一致。

公开 include 目录使用 `port/include` 和 `upstream/boost`；原子库、日志库的私有源码
include 目录通过 `private_include_dirs` 限定在本包内部。模块扫描、GMF、库实现与原始
头消费者必须共享会影响 ABI 的配置。至少复核 `BOOST_FILESYSTEM_VERSION`（原基线 v3）、
`BOOST_THREAD_VERSION`（原基线 v2）、`BOOST_ALL_NO_LIB`、Windows API 基线、线程配置、
异常和 RTTI，以及具体使用 libstdc++/libc++/MSVC STL 与静态/耦合 runtime 的契约。
不得仅在消费者 `#define` 一个宏就改变已经冻结在 BMI 中的 ABI；只支持显式、经过
生成与链接验证的配置，任意宏配置仍走有说明的头文件消费方式。

### 7.2 许可证和通知文件

封装层改用 Apache-2.0：

- 根目录 `LICENSE` 为 Apache-2.0；
- 根目录新增 `NOTICE`，归属标识为 `mcpp-community`，只声明封装贡献；
- `upstream/boost/README.md`、`LICENSE_1_0.txt` 和上游 NOTICE 保持原文件、原作者和
  原许可证；
- README 明确说明封装层与 Boost 上游许可分别适用；
- 旧仓库继承的许可证声明若仍有必要，单独标注为保留材料，不覆盖上游版权。

继承的旧封装贡献及其 BSL-1.0 声明原样保留，根 Apache-2.0 说明新增封装贡献的适用
范围；不能通过替换根 LICENSE 消除既有许可。GitHub 仓库添加 `mcpp-community` topic
并保留已有 topics，README 作者说明、NOTICE 和 manifest authors 保持一致。

源码归档必须包含根 `LICENSE`、`NOTICE` 和 `upstream/boost/` 下的所有上游许可文件。
对运行时或安装后消费测试，使用 `runtime.deploy` 把这些文件部署到不冲突的
`licenses/boost/...` 路径，包括旧封装 BSL 许可。`to` 以独立来源目录区分同名文件，
例如根 LICENSE 进入 `licenses/boost/wrapper/`，Boost LICENSE 进入
`licenses/boost/upstream/boost/`，旧封装 LICENSE 进入
`licenses/boost/upstream/boost-module/`；验证搬迁后的消费目录仍有逐字节一致的通知。

### 7.3 README

重写 `README.md` 和 `README.zh.md`，其中 `README.md` 为英文，`README.zh.md` 为必需
的简体中文版本。两者的版本、feature、平台和命令必须一致。页面顶部按固定顺序包含：

1. 项目名和一句 wrapper 介绍；
2. 单行语言、`[Upstream](https://github.com/boostorg/boost)`、Releases、mcpp、Index 导航；
3. badge；
4. Modules；
5. Getting started。

Modules 至少按下列形式逐项列出，并在后面附
`> More Modules API -> [Usage and API](docs/usage.md)`：

```text
- `import mcpplibs.mwrap.boost;`: 当前启用的公共模块闭包
- `import mcpplibs.mwrap.boost.optional;`: Boost.Optional
- `import mcpplibs.mwrap.boost.filesystem;`: Boost.Filesystem
- `import mcpplibs.mwrap.boost.macros;`: 可映射宏常量
```

并链接 `docs/usage.md`。不要在 README 重复固定工具版本和索引发布套话；工具版本、
源码 pin、include-only 限制、feature 全集和平台矩阵放到使用/架构文档的对应章节。
每种语言以约 100–150 行以内为目标；已有繁体中文入口同步调整，并保留有用的原栏目。

## 8. 模板、示例和测试

### 8.1 默认模板

至少保留一个 `templates/basic/template.toml` 和可运行的项目骨架。模板必须使用完整
包选择器作为单个依赖 key，并把版本绑定到 `{{self.version}}`：

```toml
[dependencies]
"{{self.name}}" = { version = "{{self.version}}", default-features = false, features = ["optional"] }
```

模板代码使用：

```cpp
import std;
import mcpplibs.mwrap.boost.optional;
```

默认模板不得需要 GPU、摄像头、桌面会话、模型或额外数据下载。它应创建一个简单的
console 程序，实际构造并使用 `boost::optional` 或另一个无运行时依赖的 Boost API。
高级库、GUI 或外部后端另设模板并写明前提。

模板验收必须执行真实命令：

```bash
mcpp new boost-smoke --template mcpplibs.mwrap.boost@1.92.0
cd boost-smoke
mcpp build
mcpp run
```

候选版本尚未进入索引时，可以使用隔离的已安装包 fixture 或本地 path 覆盖，但必须先
检查渲染后的依赖身份和版本仍为 `mcpplibs.mwrap.boost`、`1.92.0`。发布归档检查必须
确认 `templates/` 没有被打包脚本排除。

当前 `tools/template_smoke.sh` 通过 sed 手动渲染，把 `self.name` 当短名处理，不能作为
这一验收的替代。重写它以调用实际 `mcpp new`，再通过 TOML 解析器检查完整 key、版本
与 feature；候选本地覆盖也用解析器修改，保留显式 feature。`templates/lib/` 一并适配
并验证下游模块库消费，模板的 `.xlings.json` 同步工具 pin。

### 8.2 测试重分类

源仓库 142 个测试均自带 `main()`，目标模板则使用 gtest。先核实选定稳定 mcpp 的
测试执行语义：`2026.10.26.1` 的官方 `docs/08-testing.md` 明确每个 `tests/**/*.cpp`
编译成独立程序，以退出码判定，不强制框架。保留原有 standalone smoke，并用一个
迁入测试完成构建运行。采用 gtest 的新增测试显式声明框架依赖及其入口，不与 Boost.Test
的 main 同时链接。若具体测试需独立项目运行，将其注册为明确的 consumer。保留
release 模式仍有效的 `test_assert.hpp` 检查语义。

测试分为以下集合：

1. 每个模块的 API/行为测试，覆盖模板、重载、自由 operator、异常和 ADL；
2. 纯 include-only 库的宏和头文件测试；
3. `log`、`unit-test-framework` 的编译和链接测试，确保不生成不存在的模块 import；
   Boost.Test 编译框架与 `<boost/test/included/**>` 聚合实现使用独立构建图，不重复链接；
4. raw header 与 named module 混用测试；
5. 版本模块、宏模块和宏头的独立测试；
6. profile 的正常、CPU 不匹配和同轴冲突测试；
7. 安装后的外部 consumer 测试，不能只依赖仓库内 path dependency。

保留源仓库当前对 gcc 16.1、clang 和 macOS typeinfo/unwind 的已知限制，并在测试结果
中区分 build-only、实际运行、GUI/设备和硬件测试。

## 9. CI、资源和生成门禁

截至本文日期，mcpp-index CI pin 与 mcpp 最新稳定发布均为 `2026.10.26.1`；源仓库
构建 CI 为 `2026.10.8.1`，目标模板 pin 为 `2026.8.1.1`。选 `2026.10.26.1` 为迁移
候选，先验证 Boost 后再统一固定 `.xlings.json`、manifest 的最低版本要求、CI、示例
和模板。本次未构建迁移包，因此该候选尚不能称为 Boost 已测试版本。执行时再次核对
远端 main 和稳定 release，记录最终精确版本及编译器/runtime 组合。

CI 至少包含：

- Linux GCC、Linux LLVM、macOS LLVM 和 Windows clang-msvc 的原生构建/测试；
- 生成脚本的导出清单、补丁重放、feature 生成和审计幂等检查；
- 分组覆盖原有 116 个模块、新宏模块及根聚合；单独记录全量聚合是否可用；
- 示例构建运行和模板 smoke；
- 冷索引下的真实源码归档下载、`mcpp new`、构建和运行；
- 安装后的许可证/NOTICE 文件部署检查。

沿用源 CI 的 Linux x86_64 GCC/LLVM、Linux arm64 GCC、macOS arm64 LLVM、Windows
x86_64 clang-msvc 五种配置，并更新旧 `b1.92.0wdev` 分支过滤器为目标仓库实际主分支。
触发路径覆盖 `upstream/`、`port/`、`generated/`、`patches/`、`tools/`、模板和工具 pin。
普通模块/异常测试以原生执行结果为准，交叉 sysroot AST 通过不等于原生运行通过。

本地验证遵守 mwrap 的资源预算：所有并发构建、编译器、AST 子进程和测试合计低于
机器有效 CPU/内存的 80%，并预留桌面余量。重型构建设置 `MCPP_JOBS` 或清单中的
`[build].jobs`，不要为了缩短时间并发运行多个全量 Boost 构建。

生成门禁建议固定为：

```bash
# 在干净的迁移 checkpoint 工作树中执行；先按 tools/ 锁文件准备 Python/LLVM/sysroot。
python3 tools/import_boost.py --tarball <verified-boost-1.92.0.tar.gz>
python3 tools/reapply_hand_edits.py
python3 tools/gen_exports.py --emit-cppm
python3 tools/reapply_hand_edits.py
python3 tools/gen_features.py
python3 tools/gen_features.py --check
python3 tools/gen_audit.py
python3 tools/gen_audit.py --macros
git diff --exit-code
git status --short
```

导入上游、生成导出和生成 feature 的顺序必须在 CI 中复现。内容未变化时检查字节和
mtime 都不变化，防止公共配置头或普通 `.inc` 被无意义触碰后导致所有 vendored TU
重编。

全量重新导入会替换上游树，主要验证结果可复现；增量门禁另在同一配置上连续构建两次，
记录第二次实际编译 TU 数和生成文件 mtime。再并行构建两个 feature 不同的独立 consumer，
检查各自根模块闭包与构建输出不互相覆盖，总资源预算仍共享同一上限。

## 10. 消费者迁移示例

旧项目：

```toml
[dependencies.ZheFeng7110]
boost = { version = "1.92.0.0.0.0" }
```

```cpp
import boost.optional;
```

新项目：

```toml
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", features = ["optional"] }
```

```cpp
import mcpplibs.mwrap.boost.optional;

boost::optional<int> value = 42;
```

`boost::optional<int>` 等上游类型与普通 API 拼写保持不变。使用宏 API
时，仍然按上游规则 include 原始 Boost 头或 `<mwrap/boost/macros.hpp>`。
另外，旧 `boost::BOOST_VERSION` 扩展按第 4.2 节迁到 `.macros` 的命名空间，带下划线的
feature 按第 5 节改名，`all` 改为 `all-modules` 且不再隐式要求两个 include-only 编译库。

## 11. mcpp-index 发布流程

索引描述符必须直接放在：

```text
pkgs/mwrap/boost.lua
```

描述符声明 `namespace = "mcpplibs.mwrap"`、`name = "boost"`，不能放在
`pkgs/b/` 或把 `mwrap.boost` 当作 namespace。描述符需要：

- 封装 Apache-2.0 许可及与 manifest 一致的归属信息；
- `mcpplibs/boost-m` 仓库地址；
- `extensions.autoupdate.tag = "mwrap-${version}"`；
- Linux、macOS、Windows 的真实源码归档地址和 SHA-256；
- GLOBAL 与 CN 镜像的相同字节校验；
- `tests/mwrap/boost/` 消费成员，并注册到索引 workspace 的选择器、分片、lint、镜像、
  搜索和站点流程。

发布顺序：

1. 迁移提交通过当前分支的完整 CI；
2. 创建不可移动的 `mwrap-1.92.0` tag；
3. 生成并上传 `boost-m-1.92.0.tar.gz` 等源码归档；
4. 对 GLOBAL/CN 下载区域重新计算并核对 SHA-256；
5. 用真实归档执行冷索引 consumer 和模板测试；
6. 向 mcpp-index 提交 `pkgs/mwrap/boost.lua` 及对应 consumer。

发布工具需同步修改旧 `scripts/package-source.sh`、release workflow 和 index updater：
精确识别 `mwrap-` tag，从 manifest 核对版本，产物根目录/文件名、URL、摘要和签名以同一
候选提交为依据。替换旧“任意 tag 发布”、旧仓库镜像路径和旧 `pkgs/z/` 更新目标，使用
新仓库的明确配置。压缩进程也纳入本地资源预算，不沿用无限线程 `xz -T0`。若使用
`mcpp publish --dry-run`，检查其输出，不直接照抄可能生成的 `v${version}` tag。

索引消费成员保留空 `[indices]`。按索引当前 `tests/run_members.sh` 流程，将完整且字节
不变的候选 `pkgs/` 与 `index.toml` 准备到隔离 `MCPP_HOME` 的唯一内置索引；需要时用
`tests/prepare_builtin_index.py --home <isolated-home>`，避免多个 namespace 别名注册
同一 checkout 引起候选歧义。`tests/prepare_mwrap_indices.py` 的视图仅用于单项目调试。
实际安装使用 `tests/mwrap/check_installed_template.py` 依当前 CLI 验证真实带 SHA-256
的归档，执行 `mcpp new` 后不使用 path 替换，再 build/run；另验证混合 workspace
顺序和搬迁后的运行/通知文件。静态 Boost 实现也必须部署许可，不能依赖共享库扫描。

在 mcpp-index 根目录执行安装检查的命令为：

```bash
python3 tests/mwrap/check_installed_template.py --package mcpplibs.mwrap.boost@1.92.0 --toolchain llvm@23.1.3
```

该脚本目前的 `--deploy` 只为 ORT/OpenCV 实现部署验收分支。先新增 Boost 分支，按
`runtime.deploy` 逐项比对原始许可字节，检查独立旧封装许可目录，并移除旧的根目录
`compile_commands.json` 假设（当前 mcpp 已不再默认生成该文件），再添加 `--deploy`
执行验收；仅调用现有通用安装分支不能证明 Boost 的通知部署完成。

已创建的 tag、release、URL 与摘要保持不可变，发布后修复使用下一个单数字 revision。
规范所述“七库首发替换授权”属于对应任务背景，本次迁移没有获得该授权。

## 12. 分阶段执行与交付

| 阶段 | 工作 | 退出条件 |
| --- | --- | --- |
| M1 来源与许可 | 锁定源提交、上游归档；迁入跟踪文件；整理来源、LICENSE/NOTICE 与补丁目录 | 来源和原始许可字节可验证，迁入清单可审阅 |
| M2 身份与生成链 | 包/module 改名、feature 映射、端口分层、去除 Git 恢复依赖、版本宏迁移 | 生成闭包一致，基础独立 consumer 构建运行 |
| M3 能力与回归 | 原有测试、宏、混用、双形态 Boost.Test、CPU profile、根聚合限制 | 各组均有实际命令、退出码、结果和限制记录 |
| M4 模板与文档 | 默认及 lib 模板、README、usage/architecture 和工具 pin | 真实 new/build/run 成功，语言版本内容一致 |
| M5 CI 与归档 | 五配置原生 CI、生成/增量验证、归档内容与许可证部署 | 当前候选提交相关 CI 全通过，归档可消费 |
| M6 发布与索引 | mwrap tag、GLOBAL/CN 资产、boost.lua、workspace consumer | 冷索引无 path 替换消费及索引 CI 全通过 |

每阶段保留 checkpoint commit；每个仓库各自一个 PR。执行记录包括提交、工具版本、
命令、退出码、实际执行/仅编译状态、资源预算和 CI 链接。最终检查模块/API、ABI、
实现所有权及运行时依赖图；所有相关 CI 完成后才能报告合入或发布就绪。迁移回退使用
阶段提交恢复开发状态，已发布的身份与资产依不可变版本规则处理。

## 13. 最终验收清单

- [ ] 包身份为 `mcpplibs.mwrap.boost`，没有遗留 `mcpplibs.mylib` 或新旧身份混用。
- [ ] 包版本为 `1.92.0`，元数据单独记录上游版本，tag 为 `mwrap-1.92.0`。
- [ ] 所有 import 使用 `mcpplibs.mwrap.boost[.模块]`，公开 C++ API 仍使用 `boost::`。
- [ ] `upstream/boost/` 有来源、tag、SHA-256、裁剪说明和原始许可证文件。
- [ ] `patches/` 可重放，生成脚本不依赖 Git 历史恢复文件。
- [ ] 原有 116 个接口、新宏模块、2 个编译 include-only feature 和完整头文件清单归类一致。
- [ ] feature 全部使用 kebab-case，`all-modules` 的展开范围有文档和测试。
- [ ] `.macros` 模块、`<mwrap/boost/macros.hpp>` 和 raw macro 测试已完成。
- [ ] 根 Apache-2.0、NOTICE 和上游许可证均进入源码归档及运行时通知部署。
- [ ] 默认模板真实执行 `mcpp new`、build、run，且归档包含模板。
- [ ] 模块/API/行为/异常/头文件混用/安装后 consumer 测试通过。
- [ ] 生成物字节幂等、mtime 稳定，`git diff --exit-code` 通过。
- [ ] Linux、macOS、Windows CI 及生成审计 CI 全部通过，资源使用低于 80% 上限。
- [ ] `pkgs/mwrap/boost.lua` 和 `tests/mwrap/boost/` 通过 mcpp-index 校验。
- [ ] GLOBAL/CN 归档 hash、冷索引安装、模板消费和许可证部署均已实际验证。

## 参考规范

- [mwrap 规范](https://github.com/mcpplibs/mcpp-index/blob/main/docs/mwrap.md)
- [mwrap README 规范](https://github.com/mcpplibs/mcpp-index/blob/main/docs/mwrap-readme.md)
- [mwrap 许可规范](https://github.com/mcpplibs/mcpp-index/blob/main/docs/mwrap-licensing.md)
- [mcpp-index](https://github.com/mcpplibs/mcpp-index)
- [mcpp 稳定发布 v2026.10.26.1](https://github.com/mcpp-community/mcpp/releases/tag/v2026.10.26.1)
- [mcpp 测试规则](https://github.com/mcpp-community/mcpp/blob/v2026.10.26.1/docs/08-testing.md)
- [源仓库基线](https://github.com/ZheFeng7110/boost-module/tree/77c6b48f4f6a0f0564f8e4a17cbc029468299067)
- [Boost](https://www.boost.org/)

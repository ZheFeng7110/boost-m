# 架构

> `mcpplibs.mwrap.boost@1.92.0` · [English](architecture.md) · 简体中文
> 消费者指南：[usage.zh.md](usage.zh.md)

## 1. 本仓库是什么

一个模块包装层：Boost 1.92.0 的头文件保持为未改动的上游代码，由一层 C++23
模块让其可被 import，同时保留上游 API。包装层不重新实现 Boost，不把它的实体
搬进新的命名空间，也不链接第二份 Boost 实现。

- 116 个库模块接口 + 根模块 + 宏映射模块。
- 22 个纯 include-only 库，以及 2 个需要编译的 include-only feature（`log`、
  `unit-test-framework`）：按头文件方式消费，列于 [usage.zh.md](usage.zh.md)。
- 11 个库需要外部工具链/SDK，未做声明。

## 2. 目录布局

```text
boost-m/
├── mcpp.toml                    package identity, features, targets (generated blocks)
├── build.mcpp                   root-closure partition generator
├── upstream/boost/              pinned official Boost tree (pruned) + SOURCE.md
├── upstream/boost-module/       retained pre-mwrap wrapper licence/README/history
├── patches/                     vendored adaptations of upstream/boost (apply_patches.py)
├── port/                        wrapper TUs (system/thread/leaf extras)
├── port/include/mwrap/boost/    <mwrap/boost/macros.hpp> include entry
├── src/                         hand-maintained module facades, root module, macros module
├── generated/exports/           export lists (.inc) and edges (.deps) — generator output only
├── generated/meta/              api-surface.json, skipped.txt, lists/maps build.mcpp reads
├── templates/                   basic (default) and lib templates
├── tests/                       one standalone program per file
├── examples/                    basic, git-dep, profiles consumers
├── tools/                       import, generation (+ export_policy.toml), audit, packaging, CI
└── docs/
```

包装层自身的内容不放在 `upstream/` 下；上游内容也不放在 `src/` 或 `port/` 下。

## 3. 模块模型

每个 `src/<lib>.cppm` 都遵循同一种形态：

```cpp
module;                        // global module fragment
#include <boost/optional.hpp>  // the exact headers the export list came from
#include <boost/optional/optional_io.hpp>

export module mcpplibs.mwrap.boost.optional;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/optional.inc"   // export namespace boost { using ...; }
```

- 门面文件由人工维护，生成器从不改写它。它的 GMF include 集合恰好就是计算导出
  列表时所用的 bundle 翻译单元（该库的公共 include DAG，去掉在快照平台上无法编译
  的头文件）；`.inc` 文件承载生成的 `export namespace boost { using ...; }` 列表。
  `gen_exports.py --check` 校验两半一致：GMF 等于 bundle（外加策略声明的
  `facade-includes`），且 `export import` 块覆盖 `.deps` 中的每条边，因为 `.inc`
  不再导出更早模块拥有的实体。门面无法接受的边（range 不导入 regex：gcc 下 abi
  tag 不匹配）须在策略的 `[<lib>.facade]` `omit-imports` 中声明并写明原因。
- 一个实体只有一个导出归属。由另一个目标库拥有的依赖，在主模块中成为一条
  `export import` 边，并以完整模块名记录在 `<lib>.deps` 中。
- 跨库类型引用、友元运算符、ADL 辅助函数，以及补丁赋予外部链接的常量，都在
  审计范围内；无法导出的实体在 [usage.zh.md](usage.zh.md) 中记录，并给出改用原始
  头文件的替代方式，绝不静默裁剪。
- `src/version.cppm` 是一层薄入口，重新导出 `mcpplibs.mwrap.boost.macros`；
  映射后的常量只存在于 `src/macros.cppm`。

## 4. 根模块

`import mcpplibs.mwrap.boost;` 必须恰好重新导出当前激活 feature 集合所对应的
模块。这一点无法由已提交的文件表达；而当多个消费者构建同一份检出时，在构建期
改写已提交的文件也不可能安全。

因此接口根 `src/boost.cppm` 是固定的：

```cpp
export module mcpplibs.mwrap.boost;
export import :closure;
```

由 `build.mcpp` 把分区 `:closure` 写入 `MCPP_OUT_DIR`
（`target/.build-mcpp/out/<configuration>/<package>/boost_closure.cppm`），再用
绝对路径通过 `mcpp::source()` 选中它。输出目录按配置区分——目标、profile、
工具链和 feature 集合——因此两次使用不同 feature 的构建不会互相覆盖对方的
闭包，包根目录也保持只读。只有内容变化时才会写入该文件，所以 feature 集合没变
就不会重建模块。

`build.mcpp` 还会校验 profile feature：每条轴只能有一个 profile，按架构门控的
profile 只能在其对应架构上使用。这两类失败都会在扫描模块图之前给出诊断。

已知的聚合限制会如实报告而非隐藏：在 GCC 下闭包会排除
`mcpplibs.mwrap.boost.sort`（聚合重新导出它时触发 BMI ICE）并给出警告；用
clang 构建整个闭包则可能触及 source-location 限制。

## 5. 生成链

原则：**由编译器计算公开面，生成器只负责列出，结果不再被任何东西改写。**上游
头文件是唯一事实来源；生成文件可以从它们逐字节复现。

```sh
python3 tools/import_boost.py --tarball boost_1_92_0.tar.gz   # repopulates upstream/boost/
python3 tools/apply_patches.py             # vendored adaptations (patches/*.patch)
python3 tools/gen_exports.py               # generated/exports/*, api-surface.json, skipped.txt
python3 tools/gen_features.py              # mcpp.toml blocks + generated/meta/*.lst
python3 tools/gen_exports.py --check       # byte-compare, writes nothing
python3 tools/gen_features.py --check
python3 tools/gen_audit.py                 # linkage audit (AST, target/gen/audit)
python3 tools/gen_audit.py --macros        # macro-surface inventory
```

### 5.1 AST 来自 clang++

每个库有一个 bundle TU，包含门面 GMF 的全部头文件，只编译一次：

```sh
clang++ -std=c++23 --target=x86_64-w64-mingw32 --sysroot=<pinned MinGW> \
        -fsyntax-only -Xclang -ast-dump=json -Xclang -ast-dump-filter=boost bundle.cpp
```

同一次调用就是语法门禁：失败时，诊断里点名的 GMF 头文件（平台互斥的备选实现，
如 pthread/win32 两套线程后端）会被剔除并重新编译；剔除后的集合记录在
`api-surface.json` 中。`-ast-dump-filter=boost` 只保留 `namespace boost` 子树。
实例化密集的库输出仍有数 GB，因此 `tools/ast_json.py` 以流的方式把它读成一张
扁平的声明表：

- clang 只在文件与上一次打印的位置不同时才打印 `file`，所以读取器按输出顺序在
  所有位置（包括被丢弃的位置）之间延续这一状态，从不孤立地相信某一个 `loc.file`；
- 隐式模板实例化（`*TemplateDecl` 中模式之后的所有声明）、`UsingShadowDecl` 副本、
  注释、源码范围和记录的 definition data 在流式读取时丢弃；指向被丢弃实例化的
  引用解析到其模板；
- 声明子树中找到的引用（声明引用、lookup、类型节点声明、书写出的类型）归属到
  拥有该子树的命名空间作用域实体。

只需要 `.xlings.json` 中固定的 clang++ 驱动（`llvm`）——不需要 libclang、Python
绑定或 LLVM 发行包。

### 5.2 导出什么

- **候选**：定义位于该库拥有的头文件（`boost/<lib>/**`、`boost/<lib>.hpp`）或不属于
  任何目标库的头文件（"shared"）中、具有外部链接的命名空间作用域 `boost::` 实体。
  链接性从 AST 读取：匿名命名空间、`static`、非 inline 的 const 变量以及带 Itanium
  `L` 标记的修饰名为内部链接；typedef 名是可导出的成员。编译器隐式声明、仅以
  friend 形式出现的声明、显式与偏特化（经由主模板可达）、推导指引以及类成员都
  不是候选。
- **using 注入**：赋予实体公开拼写的命名空间作用域 `using X::y;` /
  `using namespace X;`（ADL 屏障、`detail` 再导出）。
- **签名闭包**：候选签名中点名的实体（形参、返回值、基类、字段、typedef 目标——
  typedef 会被看穿），使 API 自身的类型随之导出。
- **归属**：按依赖优先的顺序处理各库；第一个认领实体的库拥有它，之后的库在
  `<lib>.deps` 中得到一条指向拥有者的 `export import` 边。第二遍扫描每个导出实体的
  函数体，补上实例化这些函数体的消费者所需的边（跳过成环和可传递的边）。
- 输出有序、不含路径或时间戳、使用 LF；在导出块内被遮蔽的名字
  （`boost::hana::ext::boost`）以前导 `::` 拼写。

### 5.3 AST 决定不了的部分：`tools/export_policy.toml`

快照只是一种平台风味（MinGW x86_64、libstdc++）。关于公开面的所有人工决策都在
`tools/export_policy.toml` 中声明并写明原因，由生成器应用，绝不以补丁形式改写其
输出：

| 规则 | 作用 |
|---|---|
| `[[<lib>.guard]]` | 用上游头文件自身的条件包裹匹配的导出（`#if defined(_WIN32)` ...），可选 `#else` 行 |
| `[[<lib>.drop]]` | 永不导出匹配的名字 / 命名空间 |
| `[[<lib>.extra]]` | 原样声明（gcc `extern template` 虚表变通） |
| `add` | 没有收集器能到达的名字（只在函数体或 ADL 中出现的引用） |
| `gmf`、`defines`、`facade-includes` | bundle 的 include 集合与宏；仅门面使用的 include |
| `inline-namespaces` | 经由父命名空间导出由宏选择的 inline 命名空间 |
| `[<lib>.deps]` | 模块边修正；手写特殊模块的边 |

`generated/meta/skipped.txt` 列出每条规则守卫、丢弃或添加的名字，边界可以在一个
文件里审阅；`generated/meta/api-surface.json` 记录生成器身份（clang 版本、目标、
宏定义）以及每个库的 GMF、导出数和边。因此编译器升级后的差异会显示为生成器
变化，而不是无法解释的 API 变化。

### 5.4 检查

- `gen_exports.py --check` 在临时副本中重新生成，逐字节比较每个生成文件，再对照
  bundle 校验每个门面的 GMF；它从不写代码树。CI 在 Linux、macOS、Windows 上
  运行它。
- `import_boost.py` 是唯一的上游导入入口：URL、压缩包 SHA-256、保留的条目、
  裁剪掉的目录/文件以及做过 CRLF 规范化的文件，全部记录在
  `upstream/boost/SOURCE.md` 中。发行版自带的 `README.md` 和
  `LICENSE_1_0.txt` 按字节原样复制；`.gitattributes` 在 `upstream/**` 下禁用
  文本转换。
- `apply_patches.py` 只把 `patches/*.patch` 重放到 `upstream/boost/`（正向/反向
  幂等，没有 Git 时回退到 `patch -p1`），见 [patches/README.md](../patches/README.md)；
  它从不触碰生成文件或门面。
- `gen_features.py` 拼接 `mcpp.toml` 中两个带标记的块，并写出
  `generated/meta/{libs.json,features.lst,modules.lst,profiles.lst}`。它在字节和
  mtime 上都是幂等的。feature 名使用 kebab-case；`implies` 列出的是 feature
  名（绝不是内部键），由各接口的 `export import` 行加上生成的函数体边推导而来。
  `modules.lst` 是 `build.mcpp` 读取的 feature → 模块映射；`all-modules` 展开为
  117 个模块条目。
- `gen_audit.py` 从同一份 AST 报告每个库的内部链接函数、匿名命名空间实体和显式
  特化；其报告要结合已提交的代码树审阅，不能当成“不属于公共 API”。

生成需要 `.xlings.json` 中固定的 clang++ 和固定版本的 MinGW sysroot
（`tools/fetch_mingw_sysroot.py`）；当 xlings 的 LLVM 包安装了宿主配置文件时设置
`CLANG_NO_DEFAULT_CONFIG=1`。消费者两者都不需要：他们使用已提交的产物。

## 6. Feature

每个库对应一个 feature，沿依赖边可叠加。一个 feature 的源文件是其接口，对于
需要编译的库还包括上游 `src/**` 的 TU 通配。`log` 和 `unit-test-framework` 的
TU 由 feature 门控；其余 TU 也出现在基础源文件列表中，这样 `mcpp test` 能看到
完整表面，而 `mcpp build` 只编译激活的 feature。

`mcpp build` 模式会去掉所有在 feature 中列出的源文件，再重新加入激活的那些；
`mcpp test` 保留基础列表，只排除由 feature 提供的源文件。正是这两条规则，决定
了基础列表由并集生成，也决定了 Boost.Test 框架的 TU 被刻意设为只属于某个
feature（两种 Boost.Test 形态绝不能链接在一起）。

## 7. ABI 与归属

模块 TU、库 TU 和直接使用头文件的消费者共享同一套配置：

- `BOOST_FILESYSTEM_VERSION=3`、`BOOST_THREAD_VERSION=2`——导出所依据的基线，
  固定在 `[build].defines` 中；
- `BOOST_ALL_NO_LIB`、`_MT`、`WIN32_LEAN_AND_MEAN`、`SECURITY_WIN32`；
- 对 `upstream/boost/libs/thread/src/**` 按 glob 应用 `BOOST_THREAD_BUILD_LIB`；
- `port/include` 和 `upstream/boost` 为公共目录；log/atomic 的私有 include 目录
  列在 `private_include_dirs` 中；
- `-w` 用于压制随源码树带入的头文件噪声；clang 下使用
  `-Wno-invalid-specialization`（poly_collection 特化了 `std::is_void`）。

同一份 `upstream/boost` 源码树同时为接口、库 TU 和直接头文件使用提供来源。
每个程序只有一份 Boost 实现：不存在 `compat.boost` 依赖，也不得再添加第二份
副本。

- profile 的 `NDEBUG` 状态会到达头文件，因此仅 release 生效的声明差异跟随上游条件：
  Boost.uBLAS tensor 的 `BOOST_UBLAS_INLINE` 顺序由 `patches/numeric.patch` 修正
  （宏展开为 `inline` 时该写法非法），仅在检查构建中存在的 `same_impl_ex` 导出则由
  `tools/export_policy.toml` 的 `numeric` 规则加上同样的条件。

## 8. 许可

- 包装层自身的贡献（modules、port、tools、templates、tests、docs）：
  Apache-2.0，`LICENSE` + `NOTICE`（mcpp-community）。
- `upstream/boost/` 下的 Boost 上游代码：BSL-1.0，使用其自带的 `README.md` 和
  `LICENSE_1_0.txt`，哈希记录在 `upstream/documents.json`。
- 保留在 `upstream/boost-module/` 下的 mwrap 之前的包装层贡献：BSL-1.0，
  `SOURCE.md` 记录仓库、提交和旧标识。

`runtime.deploy` 将这三者投放到 `licenses/boost/...` 下各不相同的目标位置，
因此同名文件不会互相覆盖；消费者产物包中的声明文件保持字节一致。

## 9. CI 门禁

- 在 Linux x86_64（GCC 16.1、LLVM）、Linux arm64（GCC）、macOS arm64（LLVM）、
  Windows x86_64（clang-msvc）上执行原生构建 + `mcpp test`。
- `tools/ci/verify_generated.py`：在三种宿主平台的干净代码树上跑补丁重放、
  `gen_exports.py --check`、`gen_features.py --check` 和审计，最后 `git status`
  必须干净；在同一配置上再做一次构建，记录增量 TU 数量和 mtime。
- `tools/check_mwrap_release.py`：校验标识、版本方案、tag、许可证拆分以及保留
  文档的字节。
- `tools/test_template.py`：安装隔离的 `MCPP_HOME` 测试环境，执行真实的
  `mcpp new`，解析渲染出的清单（完整选择器、版本、features），然后构建并运行
  该项目——不使用 sed 渲染。
- `tools/package-source.sh`：从受版本控制的代码树生成
  `boost-m-<version>.tar.gz`，哈希放在压缩包旁，压缩时限制线程数。发布工作流
  只为不可变的 `mwrap-<version>` tag 发布该制品。

## 10. 本地资源预算

重型构建应保持在机器有效 CPU/内存的 80% 以下：设置 `MCPP_JOBS`/`-j`，避免并行
做全树构建，并让每个库的 AST 生成保持单线程。随源码树带入的数据有 400+ MB；
文件系统支持时，本地 fixture 副本会以硬链接方式复用它。

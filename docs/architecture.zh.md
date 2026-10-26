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
├── patches/                     replayable adaptations; final/<path> = stored forms
├── port/                        wrapper TUs (system/thread/leaf extras)
├── port/include/mwrap/boost/    <mwrap/boost/macros.hpp> include entry
├── src/                         module interfaces, root module, macros module
├── generated/exports/           committed export lists (.inc) and edges (.deps)
├── generated/meta/              committed lists/maps the generators and build.mcpp read
├── templates/                   basic (default) and lib templates
├── tests/                       one standalone program per file
├── examples/                    basic, git-dep, profiles consumers
├── tools/                       import, generation, audit, packaging, CI helpers
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

- GMF 的 include 集合取自该库的公共 include DAG，并去掉无法独立解析的头文件；
  `.inc` 文件承载由 libclang AST 快照生成的
  `export namespace boost { using ...; }` 列表。
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

```sh
python3 tools/import_boost.py --tarball boost_1_92_0.tar.gz   # repopulates upstream/boost/
python3 tools/reapply_hand_edits.py        # vendored adaptations
python3 tools/gen_exports.py --emit-cppm   # generated/exports/*.inc|*.deps + draft .cppm
python3 tools/reapply_hand_edits.py        # module-side adaptations, final forms
python3 tools/gen_features.py              # mcpp.toml blocks + generated/meta/*
python3 tools/gen_features.py --check
python3 tools/gen_audit.py                 # export completeness (AST)
python3 tools/gen_audit.py --macros        # macro-surface inventory
git diff --exit-code && git status --short
```

- `import_boost.py` 是唯一的上游导入入口：URL、压缩包 SHA-256、保留的条目、
  裁剪掉的目录/文件以及做过 CRLF 规范化的文件，全部记录在
  `upstream/boost/SOURCE.md` 中。发行版自带的 `README.md` 和
  `LICENSE_1_0.txt` 按字节原样复制；`.gitattributes` 在 `upstream/**` 下禁用
  文本转换。
- `reapply_hand_edits.py` 从不读取 Git 历史。它应用 `patches/*.patch`
  （正向/反向幂等，Git 不可用时回退到 `patch -p1`），并从
  `patches/final/<path>` 重放手工维护的最终形态。锚点缺失的补丁会被视为错误，
  除非它已在 `TOLERANT` 中登记并写明原因。
- `gen_exports.py` 保留两遍处理：libclang AST 快照生成导出列表，函数体引用
  扫描补齐 `.deps` 边。缓存和 bundle TU 位于被 gitignore 的 `target/gen/` 下。
- `gen_features.py` 拼接 `mcpp.toml` 中两个带标记的块，并写出
  `generated/meta/{libs.json,features.lst,modules.lst,profiles.lst}`。它在字节和
  mtime 上都是幂等的。feature 名使用 kebab-case；`implies` 列出的是 feature
  名（绝不是内部键），由各接口的 `export import` 行加上生成的函数体边推导而来。
  `modules.lst` 是 `build.mcpp` 读取的 feature → 模块映射；`all-modules` 展开为
  117 个模块条目。
- `gen_audit.py` 是导出完整性的迁移门禁；它的报告要结合已提交的代码树一起
  审阅，失败时不能简单当成“不属于公共 API”处理。

生成过程需要 LLVM 23.1.3、匹配的 libclang 以及固定版本的 MinGW sysroot
（`tools/fetch_mingw_sysroot.py`、`tools/uv.lock`）。消费者不需要这些：他们使用
已提交的产物。

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
  （宏展开为 `inline` 时该写法非法），仅在检查构建中存在的 `same_impl_ex` 导出也由
  重放工具在 `generated/exports/numeric.inc` 中加上同样的条件。

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
- `tools/ci/verify_generated.py`：在干净代码树上跑导入/重放、生成、feature 拼接
  和审计，以 `git diff`/`git status` 作为判定依据；在同一配置上再做一次构建，
  记录增量 TU 数量和 mtime。
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

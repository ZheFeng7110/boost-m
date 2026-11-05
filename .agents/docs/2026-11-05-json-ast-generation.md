# 封装层生成原理改造：clang JSON AST + 声明式策略 + `--check`

> 日期：2026-11-05
>
> 依据：[2026-10-28 mwrap 模块封装层生成原理技术报告](2026-10-28-mwrap-module-wrapper-generation.md)
> （`llama.cpp-m` / `onnxruntime-m`）
>
> 相关：[architecture.md §5](../../docs/architecture.md) ·
> [patches/README.md](../../patches/README.md) ·
> [tools/export_policy.toml](../../tools/export_policy.toml)

## 1. 改造前的问题

报告总结的原则是"编译器算出公开面，生成器只搬运，生成物可 `--check`"。改造前的
boost-m 在三处偏离了它：

| 报告的原则 | 改造前 |
|---|---|
| `clang++ -Xclang -ast-dump=json -fsyntax-only` 由编译器给出 AST | libclang Python 绑定；xlings 的 LLVM 不带 `libclang.so`，CI 每次下载约 1 GB 的完整 LLVM；libclang 与 clang++ gate 是两套解析，曾互相不一致 |
| 生成物只由生成器写出，`--check` 逐字节比对（报告 §9.1 第 8 条：不要只靠"重新生成后 git diff"） | 35 个模块侧 `patches/*.patch` + `reapply_hand_edits.py` 中的程序化 guard + `patches/final/` 整文件回放，事后改写 `generated/exports/*.inc` 与 `src/*.cppm`；CI 以 `git diff` 为判据 |
| 手写门面 `.cppm` 文本包含生成的 using 清单 | `--emit-cppm` 覆盖 `src/*.cppm`，再由补丁/最终形态恢复 |
| 不能导出的东西显式记录（`skipped.txt`、`api-surface.json`） | 分散在补丁 hunk、Python 列表与注释里 |
| 版本从 lock/清单读取 | 生成器里硬编码 `boost=1.92.0` |

## 2. 改造后的原理

```text
upstream/boost (import_boost.py + apply_patches.py：只改 vendored 树)
        │
        ▼  每库一个 bundle TU = 门面 GMF 的 include 集合
clang++ -fsyntax-only -Xclang -ast-dump=json -Xclang -ast-dump-filter=boost
        │  （同一次编译就是语法门禁；失败时剔除诊断点名的 GMF 头并重编）
        ▼  tools/ast_json.py：流式读取 → 扁平声明表
gen_exports.py：候选 / using 注入 / 签名闭包 / first-wins 归属 / 函数体边
        │  + tools/export_policy.toml（guard / drop / extra / add / gmf ...，每条带理由）
        ▼
generated/exports/<lib>.inc|.deps  generated/meta/api-surface.json|skipped.txt
        │
        ▼  文本包含
src/<lib>.cppm（人工维护的门面；--check 校验 GMF == bundle）
```

### 2.1 JSON AST 的读取

- `-ast-dump-filter=boost` 只输出 `namespace boost` 子树；但 std 侧名字里含 `boost`
  的声明（`std::__are_same<boost::…>` 的成员）也会作为无上下文的顶层对象出现，读取器
  只接受顶层 `NamespaceDecl`。
- 体量：geometry 过滤后仍有 6.8 GB（600 万个隐式 `ClassTemplateSpecializationDecl`），
  单个块 1.8 GB，不能 `json.loads`。clang 的 JSON 是一键一行、两空格缩进，读取器按行
  解析，峰值内存约 300 MB，geometry 约 45 s。
- `loc.file` / `loc.line` 是增量编码（与上一个**打印出的**位置相同则省略），
  `includedFrom.file` 不计入状态；状态必须跨所有位置延续，包括被丢弃的 range、
  实例化与 `UsingShadowDecl` 副本。这正是报告 §4.4 "不要信 `loc.file`" 的根因。
- 丢弃：`*TemplateDecl` 中模式之后的所有声明（隐式实例化）、`UsingShadowDecl` 的
  inner 副本、`FullComment`、`range`、`definitionData`、空子节点 `{}`。被丢弃实例化内
  每个节点 id 记入别名表，引用 `foo<int>` 及其成员时解析回模板 `foo`。
- 非模板声明带 Itanium `mangledName`：既是跨 TU 的稳定身份（替代 libclang USR），
  `L` 标记又直接给出内部链接。

### 2.2 与 libclang 语义对齐的规则

逐库与旧输出（把模块侧补丁与 guard 反向剥离后的原始生成物）比对，确定了这些规则：

- 词法父为类的声明不是候选；仅以 friend 出现的声明不入索引。
- 语义父优先取 `parentDeclContextId`，经别名表解析；无法解析时视为未知作用域，
  绝不退回词法命名空间（否则 `call_stack<…>::top_` 的类外定义会变成
  `boost::asio::detail::top_`）。
- `typedef enum { … } mode_t;`：typedef 名即枚举的链接名，枚举项拼写为
  `mode_t::read_only`。
- 任一重声明位于 `extern "C"` 中的函数是 C 语言链接，不作为命名空间 API。
- `using namespace X;` 只在 X 嵌套于该指令所在作用域时注入（ADL 屏障形态）；
  引入兄弟 `detail` 命名空间只是实现便利。
- 跨 TU 的身份取重声明链的首个声明，避免同一实体在不同 TU 得到不同键。
- 导出块内首段被遮蔽的名字（`boost::hana::ext::{boost,std}`）写成 `::boost::…`
  （原先由 `hana.patch` 手改）。

### 2.3 有意的行为变化

相对旧生成物，以下差异是 JSON AST 更准确的结果，保留：

- 宏拼写的 using 声明现在可见：`using BOOST_FILESYSTEM_VERSION_NAMESPACE::absolute;`
  使 `boost::filesystem::{absolute,canonical,equivalent,weakly_canonical}` 以文档拼写
  导出（此前缺失）；`BOOST_MPL_AUX_ADL_BARRIER_DECL` 生成的 `mpl_::*` 同理。
- 首个声明是类内 elaborated 前置声明的类型（`cobalt::as_tuple_tag` 等）不再被误判为
  类成员。
- 签名闭包改为基于类型字符串的名字解析（JSON 不输出 TypeLoc），不再把已被其他库
  认领的实体以"特化 USR"的形式在非属主模块重复导出；`.deps` 随之变化，
  `gen_features.py` 推导的 implies 跟着变（默认闭包增加 `bind`、`function`）。
  门面的 `export import` 块保持原样：人工定稿的门面本来就少于 `.deps`
  （range 不导入 regex，gcc abi tag 不匹配），曾尝试强制 imports ⊇ `.deps`，
  gcc 构建即失败，故不作为检查项。
- `flyweight.inc`、`algorithm.inc|.deps` 不再是手工最终形态，分别由 `drop.namespaces`
  与 `gmf` 策略生成；`version.deps` 由策略 `[version.deps]` 生成。

### 2.4 策略文件

`tools/export_policy.toml` 吸收了原 `tools/curated/*.txt`、`GMF_OVERRIDE`、
`EXTRA_DEFINES`、全部 `.inc` 补丁与程序化 guard。guard 的理由来自原补丁注释与
`reapply_hand_edits.py` 注释；`generated/meta/skipped.txt` 逐名列出受策略影响的
导出，`api-surface.json` 记录生成器身份（clang 版本、目标、宏）与每库 GMF/导出数/边。

## 3. 验证

- 旧原始生成物对照：全局只丢失 15 个名字（均为 typedef 命名的匿名枚举项，已修复）。
- `gen_exports.py --check`：连续两次生成逐字节一致，门面检查通过。
- 工具单元测试 19 项通过（新增 `test_ast_json.py` 覆盖增量位置、实例化别名、
  空子节点、顶层过滤、using/shadow 细节）。
- 冷生成约 12 分钟（geometry 单库约 1.5 分钟），全命中缓存的热生成约 50 秒。
- 本地构建（x86_64 Linux）：gcc 16.1 `mcpp build --features all-modules` 全部模块接口
  通过，`mcpp test --features all-modules` 144/144；LLVM 23.1.3 按 CI 的 A/B 分组
  `mcpp test` 各 144/144（clang 整个 all-modules 闭包会触及已知的 source-location 上限）。
  构建中发现 process 新引用到的 `boost::asio::windows::basic_object_handle` 需要
  `_WIN32` guard，已加入策略。macOS、Windows 未在本地验证，依赖 CI。

## 4. 已知限制

- 快照仍是单一平台风味（MinGW x86_64、libstdc++），平台条件仍需在策略中人工声明
  （报告 §9.1 第 10 条、§11 第 3 条）；可以改为多风味生成后求差自动推导 guard，
  但 macOS libc++ 与 MSVC 头文件目前无法在生成环境中提供。
- `extern "C"` 块内的 `namespace boost` 在过滤后的 dump 中无法识别为链接说明作用域，
  个别名字（winapi `winsdk_*`）以策略 drop 处理。
- 签名闭包与函数体边的类型名解析是文本级的近似（JSON 不输出类型位置），
  `.deps` 边与旧值有差异（608 → 633 条，丢 32 增 57），已确认合并后无 import 环。

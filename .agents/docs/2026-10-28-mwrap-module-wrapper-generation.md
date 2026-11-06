# mcpplibs `mwrap` 模块封装层生成原理技术报告

> 对象：GitHub 组织 [`mcpplibs`](https://github.com/mcpplibs) 下的
> [`llama.cpp-m`](https://github.com/mcpplibs/llama.cpp-m) 与
> [`onnxruntime-m`](https://github.com/mcpplibs/onnxruntime-m)
>
> 日期：2026-10-28
>
> 基线：`llama.cpp-m@c8b41c67cc6a7767797ccb05d637a7936c3d22ea`、
> `onnxruntime-m@baf37fe63375021e937fb6e80ecc0214242a24e2`、
> `mcpp-index@e4ee62e06db008b3cbf37744b13fe3c86264e5f9`
>
> 目标：说明“用脚本生成 C++23 模块封装层”的完整流程、背后的语言机制、
> 两个仓库各自的具体实现，以及复制这套做法时必须注意的坑。

---

## 0. 一句话结论

这两个仓库的“模块封装层生成器”**不是绑定生成器（binding generator），也不是转译器**。
它不生成任何代理函数、不复制上游实现、不改变类型和命名空间，而只做三件事：

1. **用编译器读上游公开头文件**，得到“有哪些公开名字、属于哪个作用域、值是多少”的确定清单；
2. 把清单写成**纯 `using` 声明**（`export using ::name;` 或
   `export namespace N { using ::N::name; }`），由手写的 `.cppm` 门面文本包含；
3. 对**模块机制覆盖不到的东西**（预处理宏、内部链接变量、构建期产物）
   用有限的、显式的替代物补齐：`inline constexpr` 常量、兼容头文件、
   `build.mcpp` 声明的构建图动作。

因此封装层本身几乎没有“代码”，只有**声明搬运 + 等价替换 + 构建编排**。
上游头文件始终是唯一事实来源（single source of truth）；生成物的价值在于
**规模、确定性、可审计和可重复**，而不是“把 C API 翻译成 C++”。

| 维度 | `onnxruntime-m` | `llama.cpp-m` |
|---|---|---|
| 上游形态 | 官方预编译 CPU SDK | 源码 vendored（随包编译） |
| 生成输入 | SDK 头文件 + 少量补充头 | vendored `llama.h`/`ggml*.h` + 源码 |
| 生成器 | `tools/generate_exports.py` | `tools/gen_exports.py` |
| 主要产物 | `src/exports.inc`(411 条 using)、`src/config_exports.inc`(69 个常量)、`upstream/api-surface.json` | `src/gen_exports/llama.inc`(388)、`required_ggml.inc`(99)、`typed_constants.inc`(13)、`llama.skipped.txt`、`snapshots/<tag>.json` |
| 宏策略 | `.macros` 模块 + `port/include/mwrap/onnxruntime/macros.hpp` | `.macros` 模块 + `#undef` + 跳过清单 |
| 构建脚本 | 无（声明式 `mcpp.toml` + `compat` 提供方） | `build.mcpp`（Metal 内嵌、Vulkan 着色器图） |
| 额外审计 | Clang AST `--check`、SDK 头哈希 | 导入锁、AST `--check`、源码/ABI 快照审计 |
| 版本 | `1.25.1`（上游版本） | `b10069`（上游 build 号，可加 `.1` 修订） |

---

## 1. 背景：为什么需要“生成的封装层”

### 1.1 目标形态

`mcpp` 生态要求消费者写：

```cpp
import mcpplibs.mwrap.onnxruntime;   // 而不是 #include <onnxruntime_cxx_api.h>
import mcpplibs.mwrap.llamacpp;

Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "example");
llama_backend_init();
```

但上游是传统头文件库，公开 API 动辄几百个名字、几十个头文件、
大量平台条件编译和预处理宏。手写一个模块门面会遇到四个问题：

1. **规模**：ONNX Runtime 公开面有 287 个 C 声明/枚举项、107 个 `Ort::` 声明，
   另有 `Ort::Custom` 14 个、`onnxruntime` 3 个（合计 411 条 `using`）；
   llama.cpp 有 388 个 `llama_*`/`LLAMA_*` 名字 + 99 个所需 ggml 名字。
   手抄必然漏、必然拼错。
2. **漂移**：上游升级后，新增/删除/改签名必须能被发现，而不是靠人眼 diff。
3. **一致性**：封装层绝不能“顺手改 API”。类型、重载、默认参数、ABI 都必须
   与上游逐字一致，否则消费者代码不能无缝迁移。
4. **可复现**：生成结果必须能 `--check`，在 CI 中证明“提交的封装层就是
   当前上游头文件生成的”，且不同机器生成结果字节一致。

### 1.2 `mwrap` 的组织约束

`mcpp-index/docs/mwrap.md` 定义了这个类别的硬约束，两个仓库都遵守：

- 仓库名 `<upstream>-m`，包命名空间固定 `mcpplibs.mwrap`；
- 根模块是完整包标识：`mcpplibs.mwrap.llamacpp`；
- **公开 API 保留上游原始拼写与命名空间**，不搬家、不加 `-m` 后缀；
- 优先用 `export using` 导出上游实体，每个实体只有一个导出属主；
- 版本跟上游走，最多追加一个数字修订（`5.0.0`、`5.0.0.1`）；
- 必须有可运行的默认模板、双语 README、许可证部署、CI 证据。

这解释了为什么生成器输出的是“using 清单”而不是“包装类”：
**策略上不允许发明新 API，技术上 `using` 恰好是零成本地引用同一实体。**

---

## 2. 语言机制：这样生成的模块为什么成立

理解生成器之前必须先理解四个 C++20/23 模块事实。两个仓库的所有设计都由此推导。

### 2.1 全局模块片段（global module fragment）

两个根模块都是这个形状：

```cpp
module;                     // 全局模块片段开始
#include <llama.h>          // 上游头文件在这里展开
#include <ggml-backend.h>
#undef LLAMA_DEFAULT_SEED   // 清掉会干扰模块体的对象宏
...

export module mcpplibs.mwrap.llamacpp;   // 具名模块开始
export import mcpplibs.mwrap.llamacpp.macros;
#include "gen_exports/required_ggml.inc"
#include "gen_exports/llama.inc"
```

关键点：`module;` 与 `export module` 之间的声明属于**全局模块**，
而不是这个具名模块。于是：

- 上游头文件里的类、函数、模板仍然属于全局模块，消费者**同时** `#include`
  同一个头文件不会产生“同一实体属于两个模块”的 ODR 冲突；
- 模块体里的 `export using ::llama_model_load_from_file;`
  只是把全局模块中的实体**引入并导出**，不产生新实体、不产生新符号；
- 头文件里的 `inline` 函数、模板、默认参数、`constexpr` 定义都在编译该模块时
  已完成语义分析，消费者导入后可以直接使用。

如果把 `#include` 放进模块体（`export module` 之后），声明的模块归属就会改变，
头文件/模块混用会变得非常脆弱。两个仓库都刻意把 include 放在全局模块片段，
这是“头文件与模块可共存”的基础。

### 2.2 `export using` 保留实体同一性

```cpp
export using ::llama_model_load_from_file;   // 自由函数（含全部重载）
export namespace Ort { using ::Ort::Session; } // 类（含全部成员、重载、默认参数）
```

`using` 声明引用的是**同一个实体**，因此：

- `&llama_model_load_from_file` 得到的就是上游函数地址；
- `Ort::Session` 的类型身份、布局、ABI、异常规格完全不变；
- 重载集、模板参数、默认实参原样保留；
- 不引入调用开销、不引入生命周期包装、不做异常翻译。

这正是“封装层”可以只有几百行 `using` 的原因：**编译器已经把语义做完了，
生成器只负责把名字列出来。**

### 2.3 具名模块不能导出预处理宏

`#define` 在预处理阶段消失，模块导出的是声明，不是宏。所以：

- 对象式宏（`LLAMA_DEFAULT_SEED`、`ORT_API_VERSION`）→ 映射成
  `inline constexpr` 常量，放在 `<root>.macros` 子模块；
- 函数式/控制流/字符串化宏（`ORT_TSTR`、`LLAMA_API`）→ 无法等值映射，
  保留在兼容头 `<mwrap/<lib>/macros.hpp>`，要求在 `import` **之前**包含；
- 宏在限定名里也会展开（例如 `LLAMA_DEFAULT_SEED` 作为标识符出现时），
  所以模块体里必须先 `#undef`，再用自己的常量名。

### 2.4 内部链接实体不能直接导出

ONNX Runtime 的配置键头文件里是：

```cpp
static const char* const kOrtSessionOptionsConfigDisablePrepacking = ...;
```

`static`（内部链接）变量在 GCC 下不能被模块导出（导出后消费者无法链接到它）。
`onnxruntime-m` 的处理是：在全局模块片段里把这些头文件塞进一个**私有命名空间**
`mwrap_onnxruntime_config_detail`，然后在模块体里定义**同名、同类型的
`export inline constexpr` 指针**，值来自上游常量。

```cpp
// src/config.cppm
module;
namespace mwrap_onnxruntime_config_detail {
#include <onnxruntime_env_config_keys.h>
...
}
export module mcpplibs.mwrap.onnxruntime.config;
#include "config_exports.inc"

// config_exports.inc（生成）
export inline const char* const kOrtSessionOptionsConfigDisablePrepacking =
    mwrap_onnxruntime_config_detail::kOrtSessionOptionsConfigDisablePrepacking;
```

代价是：**`.config` 模块与同名原始配置头不能在同一 TU 混用**，否则会出现两个
同名实体。文档明确写出了这个限制，并用测试覆盖“只用模块”和“原始头 + 根模块”
两种组合。

---

## 3. 总体流水线

两个仓库共享同一条主流水线，只是“上游从哪来”和“额外生成什么”不同。

```text
┌──────────────────────┐
│ 上游：SDK 头文件      │   onnxruntime: 官方预编译 SDK（compat 提供方下载）
│ 或 vendored 源码      │   llama.cpp: 按 upstream.lock 导入的官方 tag 归档
└──────────┬───────────┘
           │ 1. 锁定版本/哈希（lock / sources.lock / upstream.lock）
           ▼
┌──────────────────────┐
│ 探针翻译单元 probe.cpp│   只写 #include，不写业务代码
└──────────┬───────────┘
           │ 2. clang -Xclang -ast-dump=json -fsyntax-only
           │    （llama 另跑 clang -dM -E 取宏）
           ▼
┌──────────────────────┐
│ 语义清单             │   namespaces / kinds / 签名 / 枚举值 / 宏 / 布局指纹
└──────────┬───────────┘
           │ 3. 过滤、归属判定、排序、去重
           ▼
┌──────────────────────┐
│ 生成物（.inc/.json） │   export using ... / inline constexpr ... / 快照
└──────────┬───────────┘
           │ 4. 手写 .cppm 文本包含（不是单独编译）
           ▼
┌──────────────────────┐
│ 模块 BMI             │   mcpp 用消费者工具链编译一次
└──────────┬───────────┘
           │ 5. import
           ▼
        消费者代码
```

三条“生成线”要分开看：

1. **声明导出线**：头文件 → AST → `using` 清单（两个仓库都有）。
2. **宏/常量线**：`-dM` 宏 dump → 类型化常量 + 兼容头 + 跳过清单（两个仓库都有，
   llama 更细）。
3. **构建动作线**：只有 llama.cpp-m 需要。`build.mcpp` 不生成声明，而是把
   “Metal shader 内嵌”“Vulkan 134 组 shader 生成”声明成构建图边。

---

## 4. 原理一：AST 驱动的声明导出

### 4.1 为什么用编译器而不是正则/头文件解析器

C++ 头文件不是上下文无关文本：宏、模板、`extern "C"`、条件编译、命名空间别名、
typedef 重定向都会让正则失效。两个仓库都选择**让编译器把答案算出来**：

```bash
clang++ -std=c++23 -I<sdk include> -I<补充头> \
        -Xclang -ast-dump=json -fsyntax-only probe.cpp > ast.json
```

得到的是完整语义 AST：每个节点有 `kind`、`name`、`type.qualType`、
`loc`、`isImplicit`、`parentDeclContextId`、`completeDefinition`、`inner` 等。
生成器只做**遍历和筛选**，不重新解析 C++。

这也带来一个必须正视的事实：**AST JSON 是编译器实现细节**，不同 Clang 版本
的字段可能变化（例如 `desugaredQualType` 的出现），所以快照要记录生成它的
编译器，比较时要能识别“换编译器”而不是“API 变了”（见 7.4）。

### 4.2 探针 TU

ONNX 的探针（`tools/generate_exports.py`）：

```cpp
#include <onnxruntime_cxx_api.h>
#include <cpu_provider_factory.h>
#include <provider_options.h>
#include <onnxruntime_lite_custom_op.h>
#include <core/providers/custom_op_context.h>
#include <core/providers/resource.h>

namespace mwrap_onnxruntime_config_detail {
#include <onnxruntime_env_config_keys.h>
#include <onnxruntime_run_options_config_keys.h>
#include <onnxruntime_session_options_config_keys.h>
#include <onnxruntime_ep_device_ep_metadata_keys.h>
}
```

这个探针**就是公开面的定义**：想导出的头必须出现在这里；配置头必须放进私有
命名空间，才能把内部链接变量“困”在一个可引用的作用域里。

llama 的探针更简单：

```cpp
#include "llama.h"
#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-alloc.h"
```

因为它只导出 `llama_*` 和一份**显式白名单**里的 ggml 名字，不需要把整个
ggml 公开面都列进来。

### 4.3 遍历与过滤（ONNX 版）

`generate(ast)` 的核心规则：

```python
namespaces = {name: [] for name in ("Ort", "Ort::Custom", "onnxruntime",
                                    "mwrap_onnxruntime_config_detail")}
# 递归收集目标命名空间节点（可能多次出现：namespace 可以重开）
find_namespaces(ast["inner"])

kinds = {"CXXRecordDecl", "RecordDecl", "ClassTemplateDecl",
         "FunctionTemplateDecl", "FunctionDecl", "TypeAliasDecl",
         "TypeAliasTemplateDecl", "TypedefDecl", "EnumDecl", "VarDecl"}

for namespace in extensions:
    namespace_ids = {node["id"] for node in extensions}
    for node in namespace["inner"]:
        if node["kind"] not in kinds: continue
        if not node["name"].isidentifier(): continue   # 跳过匿名/运算符等
        if node.get("isImplicit"): continue            # 跳过编译器合成
        owner = node.get("parentDeclContextId")
        if owner and owner not in namespace_ids: continue  # 跳过类外定义
        if node["kind"] == "FunctionTemplateDecl" and any(
               x["kind"] == "CXXMethodDecl" for x in node["inner"]):
            continue                                    # 跳过成员函数模板
        names.add(node["name"])
```

这几条过滤都对应真实坑：

| 过滤 | 不做的后果 |
|---|---|
| `isidentifier()` | 生成 `using ::operator<<;` 之类的非法行 |
| `isImplicit` | 把编译器生成的拷贝构造、`__va_list_tag` 等当公开 API |
| `parentDeclContextId` | 把类外定义的成员、私有命名空间里的重载误当顶层名字 |
| 成员函数模板 | 在命名空间作用域导出类成员模板，语义错误/编译失败 |

C API 走另一条路：遍历 `LinkageSpecDecl`（`extern "C"`），按名字前缀
`Ort`/`ONNX` 或固定白名单收集，并为收集到的枚举补上枚举项：

```python
public_name = name.startswith(("Ort", "ONNX")) or name in {
    "GraphOptimizationLevel", "ExecutionMode", "ExecutionOrder",
    "RunAsyncCallbackFn", "CreateEpApiFactoriesFn", "ReleaseEpApiFactoryFn",
    "CustomOpContext", "ResourceOffset"}
```

C API 的枚举项（`ONNX_TENSOR_ELEMENT_DATA_TYPE_*`、`ORT_*`）必须单独收集，
因为它们在 AST 里是 `EnumConstantDecl`，不是类型。

### 4.4 遍历与过滤（llama 版）

llama 的判定更“按上游意图”：

- 函数：名字以 `llama_` 开头，**且声明所在源码行包含 `LLAMA_API`**。
  这条“源码行检查”用于区分公开导出函数与头文件里的 `static inline` 辅助函数；
  上游用 `LLAMA_API` 标记可见性，生成器复用同一标记，不另造白名单。
- 类型：`llama_*` 直接导出；ggml 只导出 `REQUIRED_GGML_TYPES` 白名单
  （`ggml_context`、`ggml_tensor`、`ggml_backend_t`、`ggml_cgraph` 等）。
- ggml 函数：只导出后端生命周期/图执行/分配所需的
  `REQUIRED_GGML_FUNCTIONS`（`ggml_init`、`ggml_backend_graph_compute` 等）；
  这是“够 llama 消费者用”的最小面，不是整个 ggml。
- 枚举：只对 `GGML_ENUM_TYPES` 里的枚举导出全部枚举项。
- 生成完必须自检：**白名单里任何一个名字在 AST 中缺失，直接报错退出**，
  防止上游改名后静默少导出。

llama 代码里有一段很重要的注释：Clang JSON AST 的 `loc.file` 不可靠，
可能指向探针 `.cpp` 而不是真实头文件，所以**用命名前缀做归属判断**，
不要依赖 `loc.file`。这是一个很容易踩的坑。

### 4.5 代码生成

ONNX 的输出是三个文本块：

```cpp
// src/exports.inc
export {
using ::OrtGetApiBase;
using ::ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
...
}
export namespace Ort {
using ::Ort::Session;
...
}
export namespace Ort::Custom { ... }
export namespace onnxruntime { ... }
```

llama 的输出是一行一个：

```cpp
export using ::llama_model_load_from_file;
export using ::ggml_backend_t;
```

排序规则都是**按名字排序后输出**（llama 用 `x.rsplit("::", 1)[-1]` 作为键），
保证同一输入永远得到同一字节序列——这是 `--check` 能工作的前提。

### 4.6 `using` 清单为什么能覆盖模板和内联实现

因为头文件在**全局模块片段**里被完整包含，模板定义、`inline` 函数体、
`constexpr` 变量都在编译模块时存在；`using` 只是把名字导出。消费者导入模块后：

- 实例化模板时，定义已经在 BMI 中；
- 内联函数不会产生新的外部符号；
- 默认参数在声明里，原样可见；
- `dllimport`/`visibility` 属性来自上游声明，不会被 `using` 抹掉。

反过来说，**如果上游把实现放在 `.cpp` 里，`using` 不会把它带进来**；
消费者仍然要链接上游库。模块只改变“如何声明/导入”，不改变“如何链接”。

### 4.7 和 Header Unit、手写门面的对比

| 方案 | 问题 |
|---|---|
| 手写 `using` | 规模大、易漏、升级不可审计 |
| Header Unit（`import <header>`） | 消费者要知道每个头名；宏仍不可导出；无法统一成稳定模块名；BMI 数量多 |
| 生成代理函数 | 破坏重载/默认参数/ABI，增加调用层，等于重写 API |
| 生成 `using` 清单（本方案） | 零运行时成本、保留同一性、可用 `--check` 锁死 |

---

## 5. 原理二：宏与常量的模块化

### 5.1 三类宏，三种处理

| 宏类型 | 例子 | 处理 |
|---|---|---|
| 对象式数值常量 | `LLAMA_DEFAULT_SEED`、`LLAMA_TOKEN_NULL`、`ORT_API_VERSION` | 生成带类型的 `export inline constexpr` |
| 对象式但类型敏感 | `LLAMA_STATE_SEQ_FLAGS_*`（枚举标志） | `static_cast<llama_state_seq_flags>(...)` |
| 函数式/控制流/字符串化 | `ORT_TSTR`、`LLAMA_API`、`LLAMA_H` | 留在兼容头或跳过清单，要求 `#include` 在 `import` 前 |

### 5.2 llama：先 `#undef`，再定义

`src/macros.cppm`：

```cpp
module;
#include <llama.h>
#undef LLAMA_DEFAULT_SEED
#undef LLAMA_TOKEN_NULL
...
export module mcpplibs.mwrap.llamacpp.macros;

namespace mcpplibs::mwrap::llamacpp::macros {
#include "gen_exports/typed_constants.inc"
}
```

生成的 `typed_constants.inc`：

```cpp
export inline constexpr uint32_t LLAMA_DEFAULT_SEED = 0xFFFFFFFF;
export inline constexpr llama_token LLAMA_TOKEN_NULL = -1;
export inline constexpr llama_state_seq_flags LLAMA_STATE_SEQ_FLAGS_SWA_ONLY =
    static_cast<llama_state_seq_flags>(1);
```

`#undef` 不是洁癖：宏名在模块体里出现时会被预处理展开，若不 `#undef`，
`LLAMA_DEFAULT_SEED` 这一行会先被替换成 `0xFFFFFFFF`，定义就变成语法错误。

生成器 `_expand_macro()` 会递归展开宏依赖，并检测循环引用；
`TYPED_LLAMA_CONSTANTS` 是显式表，只有表里的宏才会被转成常量，
其余宏进入 `llama.skipped.txt`（含 `LLAMA_API`、`LLAMA_H` 这类非 API 宏），
形成“为什么没导出”的可审计记录。

### 5.3 ONNX：`ORT_API_VERSION` 的类型化映射

```cpp
module;
#include <onnxruntime_c_api.h>
namespace mwrap_onnxruntime_detail {
inline constexpr unsigned api_version = ORT_API_VERSION;
}
#undef ORT_API_VERSION

export module mcpplibs.mwrap.onnxruntime.macros;

export namespace mcpplibs::mwrap::onnxruntime::macros {
inline constexpr unsigned ORT_API_VERSION = mwrap_onnxruntime_detail::api_version;
}
```

消费者用 `mcpplibs::mwrap::onnxruntime::macros::ORT_API_VERSION`。
原始 `ORT_TSTR` 等仍通过：

```cpp
#include <mwrap/onnxruntime/macros.hpp>   // 必须在 import 之前
import mcpplibs.mwrap.onnxruntime;
```

因为宏是预处理概念，模块无法“导出”它；任何声称模块能导出宏的方案都是错的。

### 5.4 配置键：内部链接的等价替换

见 2.4。这里补三个注意点：

1. 类型必须逐字保留：测试用
   `static_assert(std::is_same_v<decltype(k...), const char* const>)` 锁死；
2. 值必须逐字保留：测试比较 `std::string_view(k...) == "session.disable_prepacking"`；
3. **不能和原始配置头混用**，否则同名实体冲突；根模块也刻意不 `export import`
   `.config`，让消费者显式选择。

### 5.5 宏的“展开环境”是模块构建环境

模块 BMI 是在**构建模块时**的环境下生成的：平台宏、`ORT_API_VERSION`、
特性宏、`NDEBUG` 等都固定了。消费者之后 `#define` 什么，都不会改变模块里
已经展开的声明。ONNX 文档因此明确说：默认模块使用 SDK 的正常 API 初始化模式，
不响应调用方后来的预处理改动。这是所有“头文件 → 模块”方案的固有限制，
必须在文档和测试里写清楚。

---

## 6. `onnxruntime-m` 完整流程

### 6.1 仓库定位

```text
onnxruntime-m/
  mcpp.toml                 # 包身份 + 模块源 + compat 依赖 + 运行时部署
  src/onnxruntime.cppm      # 根模块：include 头 + 包含 exports.inc
  src/config.cppm           # 配置键模块
  src/macros.cppm           # 版本宏映射
  src/exports.inc           # 生成：C/C++ using 清单
  src/config_exports.inc    # 生成：69 个配置常量
  port/include/mwrap/onnxruntime/macros.hpp  # 宏兼容头
  upstream/
    api-surface.json        # 生成：API 清单（287 C + 107 Ort + 14 Custom + 3 onnxruntime + 69 config）
    api-coverage.md         # 头文件 → 模块面覆盖说明
    sources.lock.toml       # SDK/补充头的 URL 与 SHA256
    include/core/providers/ # Windows SDK 缺失的两个头（逐字副本）
  tools/
    generate_exports.py     # AST 生成器
    generate_model.py       # 生成测试用 Identity ONNX 模型
    check_mwrap_release.py  # mwrap 身份/README/许可证/哈希检查
    test_candidate.py       # 候选 SDK 索引下的真实构建/运行/审计
  tests/
    inference.cpp           # 真实 CPU 推理
    public_surface.cpp      # 类型/常量/工厂 API 的 static_assert
    headers_mix.cpp         # 模块与原始头共存
  templates/inference/      # mcpp 模板（内嵌模型，无需下载）
```

包声明：

```toml
[package]
namespace = "mcpplibs.mwrap"
name = "onnxruntime"
version = "1.25.1"

[build]
sources = ["src/onnxruntime.cppm", "src/macros.cppm", "src/config.cppm"]
include_dirs = ["port/include", "upstream/include"]

[dependencies.compat]
onnxruntime = "1.25.1"

[target.x86_64-linux-gnu]
cxx_runtime = "toolchain-coupled"
```

### 6.2 上游从哪来：`compat` 提供方，而不是 vendored 源码

ONNX 不在仓库里保存实现，而是依赖索引里的
`compat.onnxruntime`（`mcpp-index/pkgs/c/compat.onnxruntime.lua`）。这个提供方：

- 下载官方 CPU SDK（Linux `.tgz`、macOS `.tar.gz`、Windows `.zip`），带 SHA256；
- 声明平台运行时闭包（Linux：`onnxruntime` + `stdc++` + `gcc-runtime`/`glibc` 兼容桩）；
- 用 `generated_files` 生成一个 `sdk.cpp`，其中 `static_assert(ORT_API_VERSION == 25)`
  并检查平台 ABI（macOS arm64 / Linux-Windows x86_64 / glibc / MSVC ABI）；
- 部署 SDK 的 `LICENSE` 与 `ThirdPartyNotices.txt`。

**职责边界很清晰**：`compat` 拥有头文件和 `libonnxruntime.so`；
`mcpplibs.mwrap.onnxruntime` 拥有模块接口。二者版本必须一一对应（`1.25.1`）。

这样做的好处是：模块仓库极小（约 62KB 的树，不含 SDK），
不需要在仓库里放几百 MB 的二进制；代价是构建/发布必须同时保证提供方可用，
并且 SDK 的平台限制就是模块的平台限制（CPU only、glibc、MSVC ABI、macOS arm64）。

### 6.3 生成器逐步拆解

`tools/generate_exports.py` 的入口：

```bash
python3 tools/generate_exports.py <sdk_include_dir> [--clang clang++] [--check]
```

**第 1 步：校验补充头**

```python
lock = tomllib.loads((root / "upstream/sources.lock.toml").read_text())
for header in lock["onnxruntime"]["supplemental-headers"].values():
    digest = sha256((root / header["path"]).read_bytes())
    if digest != header["sha256"]:
        raise SystemExit(...)
```

Windows 官方 SDK 缺少 `core/providers/custom_op_context.h` 与 `resource.h`，
仓库在 `upstream/include` 放了官方 v1.25.1 源码的逐字副本，并把哈希锁死。
这属于“平台头文件补齐”，不是发明 API。

**第 2 步：构造探针并跑 Clang**

```python
probe.write_text("\n".join(f"#include <{h}>" for h in (
    "onnxruntime_cxx_api.h", "cpu_provider_factory.h", "provider_options.h",
    "onnxruntime_lite_custom_op.h", "core/providers/custom_op_context.h",
    "core/providers/resource.h")) +
    "\nnamespace mwrap_onnxruntime_config_detail {\n" +
    "\n".join(f"#include <{h}>" for h in config_headers) + "\n}\n")

subprocess.run([clang, "-std=c++23", "-I", sdk_include, "-I", upstream_include,
                "-Xclang", "-ast-dump=json", "-fsyntax-only", probe],
               stdout=out, check=True)
```

**第 3 步：`generate(ast)`**

- 收集 `Ort`、`Ort::Custom`、`onnxruntime`、`mwrap_onnxruntime_config_detail`
  四个命名空间的所有重开节点；
- 按 4.3 的规则过滤；
- C API 走 `LinkageSpecDecl` + 前缀/白名单；
- 返回 `(sorted(capi), cpp_dict)`。

**第 4 步：写文件（或校验）**

```python
outputs = {
  "src/exports.inc": text,
  "src/config_exports.inc": config_text,
  "upstream/api-surface.json": json.dumps({
      "version": "1.25.1",
      "c": capi,
      "cpp": cpp["Ort"],
      "config-keys": sorted(config_keys),
      "cpp-namespaces": {...}}, indent=2) + "\n",
}
```

`--check` 模式下逐字节比较，任何差异都以非零退出；不写文件。
生成物没有时间戳、没有绝对路径、没有随机顺序，因此可复现。

**第 5 步：人工评审**

生成器只保证“名字来自 AST”，不保证“这个面是对的”。所以仓库还有
`upstream/api-coverage.md`：逐个头文件说明它对应模块的哪一部分、
哪些是内部实现、哪些平台缺失、哪些不能导出。这是生成器无法替代的
**人工边界声明**。

### 6.4 三个模块的编译关系

```text
src/onnxruntime.cppm
  module; #include 6 个 SDK 头
  export module mcpplibs.mwrap.onnxruntime;
  export import mcpplibs.mwrap.onnxruntime.macros;
  #include "exports.inc"          // using 清单进入模块体

src/config.cppm
  module; namespace detail { #include 4 个配置头 }
  export module ...config;
  #include "config_exports.inc"   // inline constexpr 常量

src/macros.cppm
  module; #include onnxruntime_c_api.h; 捕获 ORT_API_VERSION; #undef
  export module ...macros;
  export namespace ...::macros { inline constexpr ... }
```

注意 `exports.inc`、`config_exports.inc` **不是独立编译单元**，也不在
`[build].sources` 里；它们被文本包含，实体属于所在模块。把它们误列为源文件
会导致重复定义。

### 6.5 平台与运行时适配

- **Linux**：官方 SDK 是共享库，自身依赖 libstdc++；GCC 消费者若嵌入另一份
  静态 libstdc++，会出现“SDK 抛出的异常无法被消费者捕获”的跨运行时问题。
  因此 `[target.x86_64-linux-gnu] cxx_runtime = "toolchain-coupled"`，
  让可执行文件共享同一份 C++ 运行时。提供方还把声明的 `gcc-runtime`
  中的 `libstdc++.so.6`、`libgcc_s.so.1` 软链到 SDK 的 `lib/`，不扫描宿主库。
- **macOS**：SDK 安装名是 `@rpath/libonnxruntime.1.25.1.dylib`，消费者需要
  `-Wl,-rpath,@loader_path`，让部署到可执行文件旁的 dylib 能被找到。
- **Windows**：MSVC ABI；官方 SDK 缺两个 helper 头，用逐字副本补齐。
- **不支持**：GPU EP、musl、MinGW。CPU SDK 里有 EP API 的声明，但**没有 GPU
  提供方二进制**；文档明确说“API 可用 ≠ 能执行该 EP”，避免过度承诺。

### 6.6 验证链

- `tests/public_surface.cpp`：`sizeof(OrtEpApi)`、`CreateEpApiFactoriesFn` 是指针、
  `Ort::Custom::Tensor<float>` 是类、配置键类型是 `const char* const`、
  `cpu_resource_offset == 0`、`ProviderOptions` 行为、CPU provider 注册；
- `tests/inference.cpp`：真实 Identity 模型推理，校验输出形状和值；
- `tests/headers_mix.cpp`：模块与原始头共存；
- `tools/generate_model.py`：可复现地生成测试模型（不下载外部数据）；
- `tools/test_candidate.py`：在临时副本里追加候选索引，真实 `mcpp test`，
  LLVM job 还会从 `compile_commands.json` 里找到**实际解析到的 SDK include
  路径和 clang**，再跑 `generate_exports.py --check`；可选 `--cold-sdk`
  在空 `MCPP_HOME` 中验证 SDK 能安装；
- CI 矩阵：Linux LLVM、Linux GCC、macOS arm64、Windows MSVC ABI；
- `check_mwrap_release.py`：命名空间、版本规则、Apache-2.0、NOTICE、
  README 双语/长度/互链、`upstream/documents.json` 哈希、
  `runtime.deploy` 必须包含 LICENSE/NOTICE 与上游文档。

---

## 7. `llama.cpp-m` 完整流程

### 7.1 仓库定位

```text
llama.cpp-m/
  mcpp.toml                 # 16KB 清单：源文件、按架构/OS 的 flags、feature、生成文件
  build.mcpp                # 构建程序：Metal 内嵌 + Vulkan shader 图
  src/llamacpp.cppm         # 根模块
  src/macros.cppm           # 类型化常量模块
  src/gen_exports/
    llama.inc               # 生成：388 条 llama 导出
    required_ggml.inc       # 生成：99 条 ggml 导出
    typed_constants.inc     # 生成：13 个常量
    llama.skipped.txt       # 生成：宏跳过清单
  generated/                # 清单内联生成的稳定构建输入（shim + build_info.h）
  snapshots/b10069.json     # 生成：源码/注册表/ABI/API 全量快照
  third_party/llama.cpp/    # 官方 tag 归档，逐字 vendored
  upstream.lock             # tag/commit/archive URL/SHA256
  tools/
    import_upstream.py      # 确定性导入
    gen_exports.py          # AST + 宏导出生成
    audit_snapshot.py       # 源码/API/布局审计
    generate_build_files.py # 把 [generated_files] 落盘并 --check
    check_upstream.py       # 每周上游检查
    check_release.py        # 发布门禁
    test_template.py        # 真实模板渲染/构建/运行
  tests/unit/               # 生成器、清单契约、审计、导入的单元测试
  tests/cpu_decode.cpp ...  # 真实推理/后端测试
```

### 7.2 确定性导入：`import_upstream.py` + `upstream.lock`

`upstream.lock`（b10069）：

```toml
[upstream]
repository = "https://github.com/ggml-org/llama.cpp"
tag = "b10069"
commit = "178a6c44937154dc4c4eff0d166f4a044c4fceba"
archive_url = "https://github.com/ggml-org/llama.cpp/archive/refs/tags/b10069.tar.gz"
archive_sha256 = "293a7c65..."
imported_at_utc = "2026-07-29T00:00:00Z"
```

导入器强制：

- 仓库必须是官方 `ggml-org/llama.cpp`；
- archive URL 必须是**官方 tag 归档**的规范形式，不能是任意镜像；
- commit 40 位、SHA256 64 位的格式校验；
- 下载有大小上限、临时文件 + 原子替换、对 408/429/5xx 和网络错误做
  **有界指数退避**（最多 3 次），对稳定 4xx、摘要不匹配、源码漂移立即失败；
- tar 解包前校验成员路径（防目录穿越），只接受预期前缀；
- `--check` 模式只比对，不写；`--verify-tag` 额外核对 tag 与 commit。

原则：**消费者构建时不联网、不拉 submodule**；发布归档里已经包含
`third_party/llama.cpp`。vendored 源码默认零补丁；不可避免的补丁要单独存放、
在发布说明中记录、并有回归测试。

### 7.3 导出生成：`gen_exports.py`

#### 输入与命令

```bash
python3 tools/gen_exports.py [--upstream <dir>] [--output-dir src/gen_exports] [--check]
```

默认对 `third_party/llama.cpp` 的 `include/llama.h`、`ggml/include/ggml.h`、
`ggml-backend.h`、`ggml-alloc.h` 做两次 Clang 调用：

```python
# 1) 语义 AST（注意：这里用 C++20 语法模式即可，模块本身仍是 C++23）
clang -std=c++20 -fsyntax-only -Xclang -ast-dump=json -I... probe.cpp

# 2) 宏定义（-dM -E）
clang -dM -E -I... -   <<< '#include "llama.h"'
```

#### 导出判定

| 对象 | 条件 | 输出 |
|---|---|---|
| `llama_*` 函数 | 名字前缀 + 源码行含 `LLAMA_API` | `export using ::...;` |
| `llama_*` 记录/typedef/类模板/枚举 | 名字前缀 | `export using ::...;` |
| ggml 类型 | 在 `REQUIRED_GGML_TYPES` 白名单 | 同上 |
| ggml 函数 | 在 `REQUIRED_GGML_FUNCTIONS` 白名单 | 同上 |
| ggml 枚举项 | 属于 `GGML_ENUM_TYPES` 或显式成员白名单 | 同上 |
| `LLAMA_*` 枚举项 | 前缀 | 同上 |
| 对象式 `LLAMA_*` 宏 | 在 `TYPED_LLAMA_CONSTANTS` 表 | `inline constexpr`（见 5.2） |
| 其他 `LLAMA_*` 宏 | 一律 | `llama.skipped.txt` |
| 非白名单 ggml | 一律 | 不导出 |

#### 必须通过的内部一致性检查

```python
missing_ggml = sorted((REQUIRED_GGML_TYPES | REQUIRED_GGML_ENUM_MEMBERS |
                       REQUIRED_GGML_FUNCTIONS) - exported_ggml)
if missing_ggml:
    raise RuntimeError("required GGML API missing from Clang AST: " + ...)
```

这防止“白名单写了但上游删了/改名了”导致静默缺失。同理，typed constants
的宏依赖必须存在，缺失或循环引用直接失败。

#### 输出

- `llama.inc`：388 条 `export using`（含 deprecated API，测试明确断言保留）；
- `required_ggml.inc`：99 条；
- `typed_constants.inc`：13 条带类型常量；
- `llama.skipped.txt`：13 个宏 + `LLAMA_API`/`LLAMA_H`，每条都有分类前缀
  （`macro ...`），便于人审。

所有输出排序、无时间戳；`--check` 逐字节比较，不写文件。

### 7.4 API 快照与审计：为什么不止“导出清单”

`export using` 清单能发现**名字**的增删，但发现不了：

- 函数签名/参数类型变化（同名不同型）；
- 结构体字段增删、顺序、位宽、对齐属性变化（ABI 破坏）；
- 枚举值变化；
- 宏值变化；
- typedef 背后真实类型变化（如 `int64_t` 在 macOS 是 `long long`、
  Linux 是 `long`）。

所以 `collect_api_snapshot()` 为每个声明记录“指纹”：

```python
"llama_model":   "CXXRecordDecl:{...fields, bit_width, attributes, bases...}"
"llama_token":   "TypedefDecl:int32_t"
"LLAMA_DEFAULT_SEED": "EnumConstantDecl:..."
```

记录布局时：

- `_type_fingerprint()` 规范化匿名结构体地址
  `(anonymous struct at file:line:col)` → `(anonymous struct)`；
- 对 `int8_t/uint64_t` 等定宽整数，把 `desugaredQualType` 统一成
  `signed 64-bit integer`，消除平台 typedef 差异；
- `_record_layout()` 记录字段名、类型、位域宽度、属性、嵌套记录、基类；
- `_resolved_enum_values()` 手动补全 Clang JSON 省略的隐式枚举值（+1 递增）。

`snapshots/b10069.json` 还记录：

- `upstream`：tag/commit/URL/SHA256；
- `sources`：按 `ggml_base`、`ggml_registry`、`ggml_cpu_common`、
  `ggml_cpu_x86`、`ggml_cpu_arm`、`llama_core`、`models`、`ggml_metal` 分组；
- `registry`：`GGML_USE_*` → 注册函数名；
- `metal`、`dialect_exceptions`（哪些模型文件强制 C++20）、
  `platform_links`（Windows `advapi32`）、
  `public_header_sha256`；
- `api`：declarations/macros/manual_decisions/typed_constants。

`audit_snapshot.py` 负责：

- 从上游 CMakeLists 解析源文件选择，与 `mcpp.toml` 的 source 列表比对
  （确保“清单选的文件 = 上游 CMake 在这种配置下会选的文件”）；
- 校验公开头 SHA256；
- 比对 API 快照，输出 `added/removed/changed/breaking` 分类；
- 对 breaking 变更要求显式 `--accept-breaking-api <report.json>`，
  且接受文件必须与报告完全相同，不能“一键刷新”掩盖破坏；
- 当快照记录的 Clang 与当前 Clang 不同，报告会提示“这可能是 AST 表示差异，
  不是 API 变化”，避免误导排查方向。

这是两个仓库里 llama 独有的深度：**它不仅锁“导出什么”，还锁“源码选择、
头文件字节、结构体布局、枚举值、宏值”。**

### 7.5 构建层生成：`build.mcpp`

`onnxruntime-m` 是声明式清单就够了；`llama.cpp-m` 有两个后端需要“构建期
生成”，所以有 `build.mcpp`（`import std; import mcpp;` 的 C++ 程序）。

#### Metal（macOS arm64，`backend-metal`）

1. 校验 `target_os=macos`、`target_arch=aarch64`；
2. 读 `ggml-common.h`、`ggml-metal.metal`、`ggml-metal-impl.h`；
3. 用 `replace_once()` 把 `__embed_ggml-common.h__` 和
   `#include "ggml-metal-impl.h"` 替换成实际内容（必须恰好出现一次，
   否则报错，防止上游改结构后静默生成错误文件）；
4. 生成 `ggml-metal-embed.metal`；
5. 生成汇编：

   ```asm
   .section __DATA,__ggml_metallib
   .globl _ggml_metallib_start
   _ggml_metallib_start:
   .incbin ".../ggml-metal-embed.metal"
   .globl _ggml_metallib_end
   _ggml_metallib_end:
   ```

6. `mcpp::generated(assembly)` 把它加入构建；`mcpp::define("GGML_METAL_EMBED_LIBRARY")`；
7. `mcpp::rerun_if_changed()` 声明三个输入。

结果：Metal shader 库被**静态内嵌**进产物，运行时不需要外部 `.metallib`。
测试要求看到 “using embedded metal library”、正的 offload 层数、正的 Metal
buffer 大小，编译成功不算覆盖。

#### Vulkan（Linux，`backend-vulkan`）

这里体现了“**声明构建图，而不是自己跑循环**”的原则：

- 先用 `glslc` 做 7 个扩展能力探针（cooperative matrix、bfloat16、e2m1/e4m3 等）。
  探针**以编译退出码为准**，不是解析错误消息；编译失败但没有明确说
  “extension not supported” 时，按“不支持”处理并告警，避免把“工具没跑起来”
  误判为“支持”。
- 探针结果同时用于生成器的 `-D` 和 `ggml-vulkan.cpp` 的 `-D`：
  **一次探测，两个读者**，防止两份判断漂移。
- 构造 `mcpp::action`：
  - `vulkan:shaders-gen`：用工具链编译器编译 vendored 的
    `vulkan-shaders-gen.cpp`，产出生成器；
  - `vulkan:shaders-hpp`：生成共享头 `ggml-vulkan-shaders.hpp`；
  - 每个 `.comp` 一条 action：输入生成器 + 自身 + 所有 `.glsl` 片段，
    输出 `<name>.comp.cpp`。
- 这样每个 shader 都是**独立、可增量、可并行、失败可归因**的图边；
  测试断言 action 数 = shader 数 + 2，且程序自己**不写任何生成源文件**。
- 拒绝的组合都显式报错，而不是让编译在一个很远的地方失败：
  - 非 Linux（macOS 用 Metal；Windows loader 不同）；
  - libc++ 工具链（上游 `ggml-vulkan.cpp` 对不完整类型
    `std::unique_ptr<vk_memory_logger>` 的析构触发 libc++ 静态断言；
    libstdc++ 接受。这是上游源码问题，仓库不打补丁，只拒绝）；
  - 交叉编译（生成器必须在构建机运行）；
  - 缺少 `xim:shaderc` / `glslc`。
- 第二个编译器必须被告知 `--sysroot` 和 `-B`（`mcpp::toolchain_sysroot()`、
  `mcpp::toolchain_binutils_dir()`），否则会读宿主 `/usr/include`，
  在干净 CI 上暴露 `features.h: No such file or directory`。
- `vulkan-shaders-gen` 链接成自包含产物：g++ 用 `-static`，
  clang++ 用 `-static-libstdc++ -lc++abi`（实测 `-static` 在 libc++ 下
  链接失败或启动段错误）。

#### `[generated_files]` 与小 shim

清单里内联了四个稳定生成文件：

```toml
[generated_files]
"generated/ggml_cpp.cpp" = "#include \"ggml.cpp\"\n"
"generated/ggml-cpu_cpp.cpp" = "#include \"ggml-cpu.cpp\"\n"
"generated/ggml_metal_device_m.m" = "#include \"ggml-metal-device.m\"\n"
"generated/ggml_build_info.h" = """
#pragma once
#define GGML_VERSION "b10069"
#define GGML_COMMIT "178a6c..."
"""
```

`tools/generate_build_files.py --check` 负责把这些内联内容与磁盘文件逐字比对。
`tests/unit/test_build_contract.py` 里有一张 `GENERATED_SOURCE_MAP`，把
`generated/ggml_cpp.cpp` 映射回 `ggml/src/ggml.cpp`，从而让“源码选择审计”
仍能按上游文件集合比对。

> 说明：这层 shim 的直接效果是给构建系统一个**稳定、可寻址的翻译单元名**，
> 并让 `[build.flags]` 能按 glob 精确附加 flags（如 `GGML_USE_LLAMAFILE`）。
> 具体动机应以 mcpp 引擎的对象命名规则为准；阅读时应把它理解为
> “构建输入适配”，而不是模块导出的一部分。

### 7.6 清单里的源码选择

`mcpp.toml` 不是 `sources = glob("**/*.cpp")`，而是**逐文件列表**：

- ggml 基础、registry、CPU common、llama core、137 个 `src/models/*.cpp`；
- `[target.'cfg(arch = "x86_64")'.build]` 加 x86 quants/repack；
- `[target.'cfg(arch = "aarch64")'.build]` 加 ARM 版本；
- 每个平台单独的 `cflags/cxxflags/ldflags`（Linux `-ldl -lpthread -lm`，
  macOS Metal/Foundation frameworks，Windows `advapi32`）；
- third-party 代码统一 `-w -include ggml_build_info.h`，不把上游警告
  变成消费者警告；
- 少数模型文件强制 `-std=c++20`（`dialect_exceptions`，快照里有记录）。

**为什么不用 glob**：上游源码树里有很多不参与该配置的文件（其他后端、
测试、工具），glob 会把它们拉进来；逐文件列表才能和上游 CMake 的选择
做审计比对，也才能让“这个文件为什么在/不在”可回答。

### 7.7 特性与运行时

```toml
[features]
default = { implies = ["backend-cpu"] }
backend-cpu = {}
[features.backend-metal] ...   # macOS arm64
[features.backend-vulkan] ...  # Linux + libstdc++ + 134 shaders
[feature-deps.backend-vulkan]
compat.vulkan = "1.4.357.0"
compat.spirv-headers = "1.4.357.0"
compat.vulkan-runtime = "2026.09.11"
[feature-xlings.backend-vulkan]
"xim:shaderc" = "2026.3"
"xim:mesa-lavapipe" = { version = "26.2.1", when = "dev" }
```

要点：

- 未启用的 backend 不引入任何依赖（CPU-only 消费者不装 Vulkan loader）；
- `GGML_USE_VULKAN` 必须同时定义在 `ggml-backend-reg.cpp` 上，否则库能编能链，
  但运行时不注册 Vulkan 设备——**一个没有报错的失败**；
- `mesa-lavapipe` 是 `when = "dev"`，只给本仓库测试用，不进消费者安装；
- `runtime.deploy` 部署 wrapper 与上游的 LICENSE/NOTICE/README。

### 7.8 验证链

CI（`.github/workflows/ci.yml`）的 `unit-and-import` 顺序是有意的：

```text
check_mwrap_release.py
→ 安装 pinned mcpp
→ generate_build_files.py --check
→ mcpp build --strict --no-cache        # 先构建，产出 build.mcpp 二进制
→ unittest discover tests/unit          # 其中会运行 build.mcpp 做契约测试
→ test_template.py                      # 真实模板渲染/构建/运行
→ import_upstream.py --check --verify-tag
→ gen_exports.py --check
→ audit_snapshot.py --check snapshots/b10069.json
```

为什么先 `mcpp build`：`build.mcpp` 是模块程序，普通 `c++` 编不了；
单元测试要运行 mcpp 编出来的二进制。`LLAMACPP_REQUIRE_BUILD_HELPER=1`
把“二进制缺失”从 skip 变成 fail，防止某个 job 什么都没测却绿。

另外：

- 多平台 CPU 构建/推理（Linux x86_64/ARM64、Windows、macOS）；
- Metal：注册、分配、offload、decode、sample 都要有正证据；
- Vulkan：同一 prompt 分别全 CPU/全 offload，贪心采样必须得到同一 token；
  且先从 llama.cpp 日志确认真的 offload，否则“设备没跑但 token 合法”会骗过测试；
- 模板测试走真实 `mcpp new`，不是手写 manifest；
- 发布前 `check_release.py --tag mwrap-<version>` 校验 tag/版本/lock/
  vendored 源码/exports/snapshot 一致。

---

## 8. 两个仓库的对比与分工

| 问题 | onnxruntime-m | llama.cpp-m |
|---|---|---|
| 上游交付形式 | 预编译 SDK（共享库） | 源码（随包编译成静态库） |
| 谁拥有实现 | `compat.onnxruntime` 提供方 | 本仓库 `third_party/llama.cpp` |
| 模块面 | C API + `Ort::` C++ API + `Ort::Custom` + `onnxruntime::ProviderOptions` | llama C API + 最小 ggml 面 |
| 生成器复杂度 | 中（命名空间/白名单/AST） | 高（AST + 宏 + 布局指纹 + CMake 源选择审计） |
| 宏处理 | `.macros` + 兼容头 + 69 个 config 常量 | `.macros` + `#undef` + typed constants + skipped 清单 |
| ABI 审计 | 依赖 SDK 版本固定 + 类型 static_assert | 结构体布局/枚举值/头哈希快照 |
| 构建脚本 | 无 | `build.mcpp`（Metal/Vulkan） |
| 平台策略 | 官方 SDK 支持什么就支持什么 | 自己选源文件、自己控 flags/后端 |
| 版本语义 | 上游语义版本 `1.25.1` | 上游 build 号 `b10069`（非 semver，精确 pin） |
| 主要风险 | C++ 运行时耦合、SDK 平台限制、内部链接变量 | 上游源码/ABI 漂移、shader/工具链、源选择偏差 |

共同点：

- 都是 `mcpplibs.mwrap` 身份，保留上游命名；
- 都用 Clang AST 生成 `using`，都用 `--check` 锁死；
- 都把“不能导出的东西”显式记录（config 常量 / skipped 宏）；
- 都有真实消费者测试、模板、双语 README、许可证部署、CI 门禁；
- 都拒绝“静默降级”：不可用平台/后端/工具链直接报错。

---

## 9. 注意事项与常见坑

下面按“会出什么问题 → 为什么 → 对策”组织。这些不是理论风险，多数在两个
仓库的代码注释/测试里有对应记录。

### 9.1 AST 与生成器

1. **不要依赖 `loc.file` 判断声明来自哪个头。**
   Clang JSON AST 可能把位置指向探针 `.cpp`；用命名前缀/命名空间/白名单。
   llama 的代码里明确写了这一点。

2. **必须过滤隐式声明和类外定义。**
   `isImplicit`、`parentDeclContextId not in namespace_ids` 两条缺一不可；
   否则会把编译器合成成员、私有实现细节导出成公开 API。

3. **不要导出成员函数模板到命名空间作用域。**
   对 `FunctionTemplateDecl` 里含 `CXXMethodDecl` 的节点要跳过；
   类的成员模板会随类一起导出。

4. **AST JSON 随 Clang 版本漂移。**
   同一份头文件，不同 Clang 可能写出 `desugaredQualType` 与否、匿名类型地址不同。
   对策：规范化指纹（匿名结构体、定宽整数）、记录 `generator` 身份、
   审计报告区分“换编译器”与“API 变化”。

5. **平台 typedef 差异会制造假 diff。**
   `int64_t` 的 desugared 类型在 macOS/Linux 不同；不规范化就会出现
   “什么都没改但快照变了”。llama 用 `signed N-bit integer` 规则解决。

6. **隐式枚举值不能假设 AST 一定给出。**
   `enum { A = 1, B, C }` 中 B/C 可能没有 `value` 字段；要按前值 +1 补全，
   否则快照里的值会缺失/错误。

7. **生成顺序必须稳定。**
   所有输出排序、JSON 固定缩进、不写时间戳/绝对路径/随机 ID；
   否则 `--check` 永远失败，或 CI 因机器不同而红。

8. **`--check` 必须逐字节比较且不写文件。**
   只比较“名字集合”会漏掉类型/值变化；只“重新生成后 git diff”会让
   失败信息不可读，还可能意外改写工作区。

9. **生成器不是正确性证明。**
   AST 只能说明“头文件里有这个名字”，不能说明“这个 API 在目标平台可用”。
   需要 `api-coverage.md` 这类人工边界文档 + 真实运行测试。

10. **条件编译只覆盖探针环境。**
    模块 BMI 里只有探针那次预处理的结果；平台/宏条件分支的其他声明不会
    自动出现。跨平台导出集要么分别审计，要么保持头文件平台中立并测试。

### 9.2 宏

11. **不要承诺“模块导出宏”。**
    具名模块没有宏导出机制；对象式宏只能映射成常量，函数式宏只能留在头文件。
    文档必须写清“先 `#include` 再 `import`”的顺序要求。

12. **映射宏前必须 `#undef`。**
    否则宏名在模块体里被展开，`inline constexpr` 定义直接语法错误。

13. **类型要跟着宏走。**
    `LLAMA_TOKEN_NULL` 应该是 `llama_token` 而不是 `int`；
    枚举标志要 `static_cast` 到枚举类型；否则重载解析/比较会出现细微差异。

14. **宏展开可能改变调用点语义。**
    `ORT_TSTR` 涉及 `L""`/`u""` 前缀；函数式宏可能有参数多次求值、
    `do { } while(0)`、`#`/`##`。这些**不能**用普通函数等价替换，
    必须保留原始头文件。

15. **内部链接变量不能直接导出。**
    GCC 会拒绝；用“私有命名空间 + 同名同类型 inline 常量”替换，
    并明确“不能与原始头混用”。

16. **宏名冲突是双向的。**
    消费者若同时包含原始配置头和 `.config` 模块，会出现两个同名实体；
    根模块刻意不 re-export `.config`，把选择权交给消费者。

### 9.3 链接、ABI 与运行时

17. **模块不改变链接方式。**
    `import` 只带来声明和 BMI；消费者仍要链接 `onnxruntime`/`llama`。
    忘记链接会出现“编译通过、链接失败”。

18. **C++ 运行时必须只有一份。**
    ONNX SDK 是共享库且自身依赖 libstdc++；消费者若静态嵌入另一份，
    跨边界抛出的 `Ort::Exception` 可能无法被捕获。
    对策：`cxx_runtime = "toolchain-coupled"`，并在模板里固化。

19. **共享库的运行时搜索路径要显式声明。**
    macOS 的 `@rpath`、Linux 的 `RUNPATH=$ORIGIN`、Windows 的 DLL 同目录，
    都需要 `runtime.deploy`/`ldflags` 配合；只把 `.so` 放在旁边不够。

20. **不要同时链接 vendored 实现和 compat 实现。**
    同一个库在一个程序里只能有一个属主；重复实现会导致符号/ABI 冲突。
    `mwrap.md` 明确写了这一点。

21. **导出 `using` 不会改变可见性属性。**
    上游的 `LLAMA_API`/`ORT_API2_STATUS` 已经展开成 `visibility`/`dllimport`；
    如果上游漏了标记，模块也救不了。这是上游问题，不要用 wrapper 补。

22. **deprecated API 也要导出。**
    llama 的测试明确断言保留 deprecated C API；删除它们会破坏“无缝迁移”，
    应通过 release notes 而不是静默裁剪来处理。

23. **异常/错误码语义不能包装掉。**
    ONNX 的 `Ort::Exception`、llama 的返回码/NULL 都要原样保留；
    生成器不生成 try/catch，也不做返回值转换。

### 9.4 构建图与增量

24. **构建脚本不要自己循环生成几百个文件。**
    那会串行、不可增量、失败只能报“build.mcpp exited 1”；
    应该像 llama 一样为每个产物 `submit()` 一条 action，声明 input/output。

25. **生成器自身也要是图里的一条边。**
    `vulkan-shaders-gen` 编译、共享头生成、每个 shader 生成，三者分开；
    否则改一个 shader 会重跑全部，或改了生成器不重跑。

26. **第二个编译器要显式给 sysroot/binutils。**
    构建程序启动的子编译器不知道 mcpp 的 sysroot，默认会读宿主 `/usr/include`；
    本地可能“碰巧能编”，干净 CI 立刻失败。

27. **能力探测以编译结果为准。**
    解析 glslc 的错误文本会把“工具没启动/磁盘满/测试 shader 改名”误判为
    “扩展支持”；必须以退出码为准，异常情况按不支持处理并告警。

28. **一次探测，多处使用。**
    生成器和 `ggml-vulkan.cpp` 必须用同一组 `-D`；探测两次必然漂移。

29. **不要跨平台“试试看”。**
    Vulkan 在 macOS 需要 MoltenVK、Windows loader 语义不同；
    显式拒绝比在深处报一个无关错误好。

30. **生成文件要保持时间戳稳定。**
    `mwrap.md` 明确要求：内容未变时不要 touch，否则一个公共头会让整棵
    vendored 源码重编。CI 要同时验证“字节幂等”和“增量行为”。

31. **构建期不要写源码树。**
    生成物写到 `MCPP_OUT_DIR`，不要在 `build.mcpp` 里改 `third_party/` 或
    `src/`；否则并发配置互相覆盖，且工作区变脏。

### 9.5 供应链与可重复性

32. **锁定一切。**
    `.xlings.json` 锁 mcpp 版本；`upstream.lock`/`sources.lock.toml` 锁上游
    tag/commit/归档 SHA256；补充头单独锁哈希；CI 用 pinned 工具链。

33. **归档 URL 要规范化。**
    只接受官方 tag 归档形式，防止镜像/分支内容漂移；
    下载做大小上限、原子替换、有界重试。

34. **vendored 源码默认零补丁。**
    必须打补丁时单独存放、记录理由、加回归测试；不要“顺手改一下”。

35. **许可证要随产物部署。**
    共享 SDK 的 `LICENSE`/`ThirdPartyNotices.txt` 不会被自动发现；
    用 `runtime.deploy` 显式声明目标路径，并在重定位后的消费者包里验证字节。

### 9.6 版本与发布

36. **版本就是上游版本，不另造 0.x。**
    `llama.cpp-m` 用 `b10069`，修订用 `b10069.1`；
    `onnxruntime-m` 用 `1.25.1`，修订只能是 `1.25.1.N`（N≥1）。
    不要同时发布 `5.0.0` 和 `5.0.0.0`（比较相等）。

37. **非 semver 版本只能精确 pin。**
    `b10069` 不能表达范围；消费者要用新 feature 必须改版本号。
    这是已知代价，发布说明要写清。

38. **发布不可变。**
    tag、归档、索引条目一旦发布不改；修复发新修订。
    首次发布前的替换是显式授权的例外。

39. **breaking 变更不能靠“重新生成快照”通过。**
    llama 的审计要求 `--accept-breaking-api` 且接受内容与报告完全一致；
    删除/改签名必须在 release notes 里解释。

### 9.7 测试与资源

40. **编译成功不等于后端可用。**
    Metal 要证明注册/分配/offload；Vulkan 要证明真的 offload 且结果与 CPU 一致；
    ONNX 要跑真实推理，不是只 `sizeof`。

41. **模板要走真实渲染器。**
    手写一个等价 manifest 测试不到 `{{self.name}}`、feature 转发、模板归档等
    真实路径；两个仓库都有 `test_template.py`。

42. **cold consumer 测试。**
    在空 `MCPP_HOME`/隔离索引里真正下载、安装、构建、运行；
    否则“本机已装好”会掩盖依赖缺失。

43. **本地测试资源有预算。**
    `mwrap.md` 要求本地测试总量不超过机器可用 CPU/内存的 80%；
    AST 子进程、代码生成、编译都要计入，必要时用 cgroup 限制。

44. **skip 要能变成 fail。**
    CI 专用 job 用 `LLAMACPP_REQUIRE_*` 把“工具缺失导致 skip”变成失败，
    防止关键 job 什么都没测却绿。

---

## 10. 复刻清单：新增一个 `xxx-m` 的最小步骤

1. **确认上游形态**
   - 有稳定公开头 + 预编译 SDK → 走 onnx 模式（`compat` 提供方 + AST 导出）；
   - 只有源码/需要自己选源文件 → 走 llama 模式（vendored + lock + 快照）。

2. **建立身份**
   - 仓库 `xxx-m`，`namespace = "mcpplibs.mwrap"`，`name = "xxx"`；
   - 根模块 `mcpplibs.mwrap.xxx`；
   - `[package.metadata.mwrap]` 记录 upstream-version/repo。

3. **锁定上游**
   - 源码：`upstream.lock`（repo/tag/commit/archive_url/sha256）；
   - SDK：`sources.lock.toml` + 头文件哈希；
   - `.xlings.json` 锁 mcpp。

4. **写探针**
   - 列出所有公开头；需要特殊作用域的（内部链接变量）放进私有命名空间；
   - 确保探针在目标平台能编译。

5. **写生成器**
   - `clang -Xclang -ast-dump=json -fsyntax-only`；
   - 明确命名空间/前缀/白名单/排除规则；
   - 排序输出、无时间戳；
   - 生成 `--check` 模式；
   - 生成一份人类可读的 `api-surface.json`/`skipped.txt`。

6. **写门面**
   - `src/xxx.cppm`：`module;` + include + `export module` + `#include "exports.inc"`；
   - `src/macros.cppm`：对象宏 → typed constexpr；
   - 必要时 `src/config.cppm`：内部链接变量 → inline 常量；
   - 兼容头 `port/include/mwrap/xxx/macros.hpp`。

7. **写清单**
   - `[build].sources` 精确列出；
   - `include_dirs`、`cflags/cxxflags/ldflags` 按平台；
   - `[features]` 与 `[feature-deps]`；
   - `[[runtime.deploy]]` 部署许可证。

8. **写测试**
   - 模块-only 编译；
   - 原始头与模块混用；
   - API 表面 `static_assert`；
   - 真实功能/推理；
   - 模板真实渲染 + 构建 + 运行。

9. **写 CI**
   - `--check` 生成物；
   - 上游导入/哈希校验；
   - 多平台构建/运行；
   - 发布 tag 审计。

10. **写文档**
    - README（英文）+ README.zh.md；
    - `docs/usage.md` 写清支持边界、不能混用的组合、宏包含顺序；
    - `upstream-update-policy` 写清触发条件、版本映射、失败归属。

---

## 11. 局限与可改进点

1. **单一大模块的编译成本。**
   所有头进一个模块 TU，消费者侧导入很快，但模块本身编译内存/时间高；
   上游头越多越明显。可按上游子模块拆分（`mwrap.xxx.core` 等），
   但会增加 BMI 管理和交叉依赖。

2. **AST 只覆盖“声明”，不覆盖“语义契约”。**
   线程安全、所有权、异常保证、生命周期、回调线程模型都不在 AST 里；
   必须靠文档和真实测试。生成器不能替代 API 评审。

3. **平台条件声明的清单偏差。**
   单平台生成的 `exports.inc` 不包含其他平台独有名字；
   要么分别生成并合并（需保证各平台 using 在别的平台也能编译），
   要么在 CI 里做“头文件声明集合 ⊆ 导出集合”的检查。

4. **宏覆盖天然不完整。**
   函数式宏、`#if` 控制流、`#`/`##` 宏只能留在头文件；
   `.macros` 模块永远只是“常用对象常量”的子集，文档必须诚实。

5. **快照对编译器敏感。**
   llama 已用规范化 + 记录生成器缓解，但换 Clang 仍可能需要重新生成；
   这不是 API 变化，CI 报错信息要能区分。

6. **版本硬编码。**
   ONNX 生成器里 `1.25.1`、llama 的 `GGML_VERSION` 都写在脚本/清单里；
   升级时要同步改多处，最好从 lock/清单读取，减少漏改。

7. **构建脚本的可移植性。**
   Vulkan 分支依赖 `std::system`、shell 重定向、`glslc` 名字、工具链布局；
   换平台/换 shell 就可能失效。llama 通过“显式拒绝 + CI 覆盖”控制风险，
   但仍是维护成本最高的部分。

8. **工具链耦合。**
   ONNX 的 `toolchain-coupled` 是正确修复，但意味着消费者不能用任意
   静态 C++ 运行时组合；这是 SDK 共享库模型的固有限制。

---

## 12. 参考文件（基线 commit 永久链接）

### `llama.cpp-m`

- 导出生成器：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/tools/gen_exports.py>
- 源码/ABI 审计：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/tools/audit_snapshot.py>
- 上游导入：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/tools/import_upstream.py>
- 构建程序：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/build.mcpp>
- 模块门面：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/src/llamacpp.cppm>
- 宏模块：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/src/macros.cppm>
- 清单：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/mcpp.toml>
- 更新策略：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/docs/upstream-update-policy.md>
- 生成器单元测试：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/tests/unit/test_gen_exports.py>
- 清单/构建契约测试：<https://github.com/mcpplibs/llama.cpp-m/blob/c8b41c67cc6a7767797ccb05d637a7936c3d22ea/tests/unit/test_build_contract.py>

### `onnxruntime-m`

- 导出生成器：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/tools/generate_exports.py>
- 根模块：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/src/onnxruntime.cppm>
- 配置模块：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/src/config.cppm>
- 宏模块：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/src/macros.cppm>
- 覆盖说明：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/upstream/api-coverage.md>
- API 清单：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/upstream/api-surface.json>
- 清单：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/mcpp.toml>
- 使用说明：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/docs/usage.md>
- 发布检查：<https://github.com/mcpplibs/onnxruntime-m/blob/baf37fe63375021e937fb6e80ecc0214242a24e2/tools/check_mwrap_release.py>

### 生态政策

- `mwrap` 规范：<https://github.com/mcpplibs/mcpp-index/blob/e4ee62e06db008b3cbf37744b13fe3c86264e5f9/docs/mwrap.md>
- 许可证策略：<https://github.com/mcpplibs/mcpp-index/blob/e4ee62e06db008b3cbf37744b13fe3c86264e5f9/docs/mwrap-licensing.md>
- README 模板：<https://github.com/mcpplibs/mcpp-index/blob/e4ee62e06db008b3cbf37744b13fe3c86264e5f9/docs/mwrap-readme.md>
- ONNX SDK 提供方：<https://github.com/mcpplibs/mcpp-index/blob/e4ee62e06db008b3cbf37744b13fe3c86264e5f9/pkgs/c/compat.onnxruntime.lua>

---

## 13. 最终总结

`llama.cpp-m` 与 `onnxruntime-m` 的封装层生成，本质上是把
**“上游公开声明”这一事实**从编译器里取出来，转成两种确定的形式：

- 对声明：`export using`，零成本、保身份、可 `--check`；
- 对宏/内部链接/构建产物：显式的、有限范围的等价替换与构建图声明。

真正困难的不是写 `export using`，而是：

1. 定义**什么算公开面**（命名空间、前缀、白名单、人工覆盖文档）；
2. 定义**什么不能直接导出**（宏、内部链接变量、平台条件声明）；
3. 用**快照/哈希/审计**把“上游变了”与“生成器换了”区分开；
4. 用**真实运行测试**证明模块不是“能编译但不可用”；
5. 用**版本/锁/不可变发布**保证消费者拿到的是可复现的字节。

这套方法可以复制到任何“公开头文件 + 稳定 ABI”的 C/C++ 库上；
代价是必须接受“生成器只搬运、不翻译”的边界，并把边界之外的东西
（宏、运行时、平台、构建）逐个显式处理，而不是假装它们不存在。

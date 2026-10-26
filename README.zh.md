# boost

> Boost 1.92.0 的 C++23 命名模块封装 —— 用 `import` 引入，继续写 `boost::`。

**[English](README.md)** · [简体中文](README.zh.md) · [繁體中文](README.zh.hant.md) · [上游](https://github.com/boostorg/boost) · [发布](https://github.com/mcpplibs/boost-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [索引](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![ci-linux](https://github.com/mcpplibs/boost-m/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-linux.yml)
[![ci-macos](https://github.com/mcpplibs/boost-m/actions/workflows/ci-macos.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-macos.yml)
[![ci-windows](https://github.com/mcpplibs/boost-m/actions/workflows/ci-windows.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-windows.yml)

116 个 Boost 库可以按命名模块使用：封装在模块的全局模块片段中 `#include` 上游头文件，
再用 `export using` 重新导出上游实体，因此类型、重载、ADL 和拼写都仍是 Boost 的。
纯头库、编译库与纯宏库各自保留既有消费方式；程序里只有 vendored Boost 这一份实现，
不会重复链接第二份。

## 模块

- `import mcpplibs.mwrap.boost;`：当前启用的公共模块闭包（汇总模块）
- `import mcpplibs.mwrap.boost.optional;`：Boost.Optional
- `import mcpplibs.mwrap.boost.filesystem;`：Boost.Filesystem（编译库）
- `import mcpplibs.mwrap.boost.json;`：Boost.JSON（编译库）
- `import mcpplibs.mwrap.boost.type_traits;`：Boost.TypeTraits（feature `type-traits`）
- `import mcpplibs.mwrap.boost.macros;`：可映射的宏常量（`BOOST_VERSION`、`BOOST_LIB_VERSION`）

> 更多模块 API -> [使用与 API](docs/usage.zh.md)

## 快速开始

```sh
mcpp new boost-app --template mcpplibs.mwrap.boost@1.92.0
cd boost-app
mcpp run
```

在已有项目中：

```toml
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, features = ["optional"] }
```

```cpp
import std;
import mcpplibs.mwrap.boost.optional;

int main() {
    boost::optional<int> answer = 42;
    std::println("{}", *answer);
}
```

## features

feature 名一律小写 kebab-case；模块后缀保留 Boost 自己的分组拼写
（`type-traits` 启用 `mcpplibs.mwrap.boost.type_traits`）。

| Feature | 作用 |
|---|---|
| `default` | 50 库基线闭包（纯头核心 + filesystem/regex/thread/chrono/json/serialization） |
| `all-modules` | 全部模块接口（117 个）；`log`、`unit-test-framework` 仍需显式启用 |
| `macros` | 宏常量映射模块；`version` 会重新导出它 |
| `log`、`unit-test-framework` | 无模块的编译库：直接 include 上游头文件 |
| `backend-log-avx2`、`backend-log-ssse3` | 构建期 Boost.Log dump 实现（仅 x86_64，同轴互斥） |

feature 在多条依赖边上做加法并集：某条边写 `default-features = false` 只关闭这条边请求的默认集，
不会撤销另一条边启用的集合。

## 测试与平台

CI 在 Linux x86_64（GCC 16.1 与 LLVM）、Linux arm64（GCC）、macOS arm64（LLVM）和
Windows x86_64（clang-msvc）上构建并运行。请求不可用的 backend 会显式失败，不会被静默跳过。

两个汇总限制有明确诊断，不隐藏：GCC 下根模块不重新导出 `mcpplibs.mwrap.boost.sort`
（汇总模块重导出它会触发 BMI ICE，可直接单独 import），clang 构建整份 `all-modules`
闭包可能触达 source-location 上限 —— 请按分组 import。纯 include-only 库（宏 API 与
受编译器缺陷影响的库）列在[使用文档](docs/usage.zh.md)中。

## 示例与文档

- [示例](examples) —— 汇总模块、显式 feature、profile 三种消费者
- [使用与 API](docs/usage.zh.md) —— 消费方式、宏拆分、feature 与平台约定
- [架构](docs/architecture.zh.md) —— 模块布局、生成链、ABI 与实现归属

## 许可

封装贡献使用 [Apache-2.0](LICENSE)，归属见 [NOTICE](NOTICE)。vendored Boost 保留其原始
BSL-1.0 [README 与许可证](upstream/boost/)；保留在 [upstream/boost-module](upstream/boost-module/SOURCE.md)
的旧封装贡献继续适用其自身的 BSL-1.0 声明。根许可证不会重新授权任何一个上游部分。

# boost

> Boost 1.92.0 的 C++23 具名模組封裝 —— 用 `import` 引入，繼續寫 `boost::`。

**[English](README.md)** · [简体中文](README.zh.md) · [繁體中文](README.zh.hant.md) · [上游](https://github.com/boostorg/boost) · [發佈](https://github.com/mcpplibs/boost-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [索引](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![ci-linux](https://github.com/mcpplibs/boost-m/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-linux.yml)
[![ci-macos](https://github.com/mcpplibs/boost-m/actions/workflows/ci-macos.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-macos.yml)
[![ci-windows](https://github.com/mcpplibs/boost-m/actions/workflows/ci-windows.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-windows.yml)

116 個 Boost 庫可以按具名模組使用：封裝在模組的全域模組片段中 `#include` 上游標頭檔，
再用 `export using` 重新匯出上游實體，因此型別、多載、ADL 與拼寫都仍是 Boost 的。
純標頭庫、編譯庫與純巨集庫各自保留既有消費方式；程式裡只有 vendored Boost 這一實作，
不會重複連結第二份。

## 模組

- `import mcpplibs.mwrap.boost;`：目前啟用的公開模組閉包（彙總模組）
- `import mcpplibs.mwrap.boost.optional;`：Boost.Optional
- `import mcpplibs.mwrap.boost.filesystem;`：Boost.Filesystem（編譯庫）
- `import mcpplibs.mwrap.boost.json;`：Boost.JSON（編譯庫）
- `import mcpplibs.mwrap.boost.type_traits;`：Boost.TypeTraits（feature `type-traits`）
- `import mcpplibs.mwrap.boost.macros;`：可映射的巨集常數（`BOOST_VERSION`、`BOOST_LIB_VERSION`）

> 更多模組 API -> [使用與 API](docs/usage.md)

## 快速開始

```sh
mcpp new boost-app --template mcpplibs.mwrap.boost@1.92.0
cd boost-app
mcpp run
```

在既有專案中：

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

feature 名一律小寫 kebab-case；模組後綴保留 Boost 自己的分組拼寫
（`type-traits` 啟用 `mcpplibs.mwrap.boost.type_traits`）。

| Feature | 作用 |
|---|---|
| `default` | 50 庫基線閉包（純標頭核心 + filesystem/regex/thread/chrono/json/serialization） |
| `all-modules` | 全部模組介面（117 個）；`log`、`unit-test-framework` 仍需明確啟用 |
| `macros` | 巨集常數映射模組；`version` 會重新匯出它 |
| `log`、`unit-test-framework` | 無模組的編譯庫：直接 include 上游標頭檔 |
| `backend-log-avx2`、`backend-log-ssse3` | 建置期 Boost.Log dump 實作（僅 x86_64，同軸互斥） |

feature 在多條依賴邊上做加法聯集：某條邊寫 `default-features = false` 只關閉這條邊請求的預設集，
不會撤銷另一條邊啟用的集合。

## 測試與平台

CI 在 Linux x86_64（GCC 16.1 與 LLVM）、Linux arm64（GCC）、macOS arm64（LLVM）與
Windows x86_64（clang-msvc）上建置並執行。請求不可用的 backend 會明確失敗，不會被靜默跳過。

兩個彙總限制有明確診斷，不隱藏：GCC 下根模組不重新匯出 `mcpplibs.mwrap.boost.sort`
（彙總模組重新匯出它會觸發 BMI ICE，可直接單獨 import），clang 建置整份 `all-modules`
閉包可能觸達 source-location 上限 —— 請按分組 import。純 include-only 庫（巨集 API 與
受編譯器缺陷影響的庫）列在[使用文件](docs/usage.md)中。

## 範例與文件

- [範例](examples) —— 彙總模組、明確 feature、profile 三種消費者
- [使用與 API](docs/usage.md) —— 消費方式、巨集拆分、feature 與平台約定
- [架構](docs/architecture.md) —— 模組佈局、生成鏈、ABI 與實作歸屬

## 授權

封裝貢獻使用 [Apache-2.0](LICENSE)，歸屬見 [NOTICE](NOTICE)。vendored Boost 保留其原始
BSL-1.0 [README 與授權](upstream/boost/)；保留在 [upstream/boost-module](upstream/boost-module/SOURCE.md)
的舊封裝貢獻繼續適用其自身的 BSL-1.0 聲明。根授權不會重新授權任何一個上游部分。

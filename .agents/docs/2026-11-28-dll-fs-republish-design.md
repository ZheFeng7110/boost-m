---
subject: generator
status: landed
---

# `boost::dll::fs` 跟随 `boost::filesystem`：策略 `republish`

来源：issue #2 的第一条验收标准，以及"建议方向"第 1 条。

## 问题

`boost/dll/config.hpp` 中 `boost::dll::fs` 的写法是
`namespace boost { namespace dll { namespace fs { using namespace boost::filesystem; } } }`
（定义 `BOOST_DLL_USE_STD_FS` 时改为 `std::filesystem`）。导入模块的一方看不到
using 指令。生成器目前只跟随 ADL 屏障形态的指令（目标命名空间嵌套在指令自身的
命名空间里），所以这条指令被跳过。

PR #5 用 `[[dll.extra]]` 手写了 33 个名称（另有上游本来就是 using 声明的
`error_code` / `system_error`）。和 #1 之前的 main 相比，`dll::fs::` 写法仍少了
49 个，其中 28 个不是运算符（`symlink_status`、`space`、`permissions`、
`create_symlink`、`unique_path` 等）。手写列表也不会随 Boost.Filesystem 升级更新。
测试只用到了 `path`、`recursive_directory_iterator`、`is_regular_file` 三个名称。

## 决定

新增策略条目 `[[<lib>.republish]]`，字段为 `namespace`、`reason`，以及可选的 `if`。

- 生成器在该库的 TU 中找到 `namespace` 里的 using 指令，拿到它的目标命名空间
  （`boost::filesystem`）。
- 名称来源是上游模块实际写进 `.inc` 的导出，即应用 drop、guard 之后的结果，并且
  只取直接位于目标命名空间的那些（不含 `boost::filesystem::detail` 等嵌套命名
  空间）。`emit_inc` 把每个库导出的名称记进 `published`；filesystem 按拓扑序
  先于 dll 处理，所以处理 dll 时这些名称已经齐备。上游名称自带的 guard 会和条目
  的 `if` 用 `&&` 合并。
- 输出写成 `using ::boost::filesystem::X;`，放在 `export namespace boost::dll::fs`
  中，并整体包在 `#if !defined(BOOST_DLL_USE_STD_FS)` 里。拥有这些名称的库会加进
  `.deps`，保证模块 `export import` 了它。
- 缓存：dll 的条目记下所用的来源列表（`republish`），以及自己导出的
  `published`。命中条件是来源列表与上游当前的导出一致：上游的输出可能因为
  先认领者优先（first-wins）的归属而改变，dll 自己的 key 却没变。
- 策略失效时直接报错，不静默跳过：TU 中没有这条指令，或者没有上游模块在目标
  命名空间里导出名称。
- `std::filesystem` 分支仍然是 `[[dll.extra]]`，放在
  `#if defined(BOOST_DLL_USE_STD_FS)` 中。它是固定的标准接口，这次按
  [fs.filesyn] 补全为 54 个名称；path 的运算符、`swap`、`hash_value` 靠 ADL
  查找，不逐个列出。

结果：`dll::fs` 现在导出 138 个 `boost::filesystem` 名称，与 `filesystem.inc` 在
`boost::filesystem` 中直接导出的名称完全一致。重新生成后，只有 dll 的输出发生了
变化。

## 否决的做法

- **自动跟随所有"目标由别的库拥有、所在命名空间不是 detail"的指令**（issue 中
  提到的另一种写法）：会一次性改变多个库的导出面，每一条都需要单独审查。issue #2
  中其余几组名称仍按 issue 的计划逐个确认。显式的策略条目可以审查，也会写进
  `skipped.txt`。
- **在 `republish` 上提供 `else`，把 std 列表放进 `#else` 分支**：当上游的部分
  名称带有自己的 guard 时，按名称排序后，条目的 `#if` 块会被拆成几段，`#else`
  行就会重复输出。所以 std 列表留在独立的 `extra` 块里。
- **写成 `using boost::dll::fs::X;`（#1 之前 main 的写法）**：两种写法指向同一个
  实体。选用目标库的限定名，是为了让来源一目了然，也让以 `boost::filesystem::`
  开头的枚举项（`file_type::regular_file`）保持上游 `.inc` 原样的写法。

## 测试

- `tests/dll/smoke.cpp`：
  - `static_assert` 检查 `dll::fs::path` 等名称与 `boost::filesystem` 中是同一
    类型，`program_location()` 的返回类型是 `dll::fs::path`，枚举项可以写成
    `dll::fs::regular_file`；
  - 以不求值的方式检查只有 Boost 才有的名称（`unique_path`、`system_complete`、
    `portable_name`）；
  - 在临时目录中用 `error_code` 重载调用一组旧列表里没有的函数
    （`symlink_status`、`resize_file`、`hard_link_count`、`space`、
    `status_known` 等）。
  
  换回 main 的 `dll.inc` 后，这个文件在 gcc 上编译失败，报的正是这些名称。
- `tools/tests/test_gen_exports_republish.py`：覆盖来源选择、guard 合并、已被
  认领的名称、上游不可达、过期条目、缓存失效和输出形态。

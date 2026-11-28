---
subject: generator
status: landed
---

# 不跟随的 using 指令必须声明：策略 `skip-directive`

来源：issue #2 的第二条验收标准（detail 命名空间组确认可以丢弃，并在
`skipped.txt` 中说明原因），以及"建议方向"第 2 条（"不要静默丢弃"）。叠在
[`republish`](2026-11-28-dll-fs-republish-design.md)（PR #9）之上。

## 问题

`collect_injections` 只跟随 ADL 屏障形态的 `using namespace`（目标嵌套在指令自身
的命名空间里）。其余指令被直接跳过，也不留任何记录。issue #2 中有三组名称是这样
丢掉的，当时只给出了初步判断"detail，丢掉可以接受，需确认"：

| 名称数 | 命名空间 | 指令 |
|---:|---|---|
| 356 | `boost::numeric::ublas::detail` | `boost/numeric/ublas/matrix.hpp`、`triangular.hpp`：`using namespace boost::numeric::ublas;` |
| 355 | `boost::serialization::cpp_int_detail` | `boost/multiprecision/cpp_int/serialize.hpp`：`using namespace boost::multiprecision;`、`using namespace boost::multiprecision::backends;` |
| 55 | `boost::sort::blk_detail` | `boost/sort/block_indirect_sort/blk_detail/block.hpp`：`using namespace boost::sort::common;` |

## 调查

**全部被跳过的指令。** 给跳过分支临时加了记录，做一次冷的全量生成
（`--no-cache`）。116 个库中一共只有 12 处，分属 10 个（库，命名空间）：

| 库 | 指令所在命名空间 | 目标 |
|---|---|---|
| accumulators | `boost::numeric::functional` | `boost::numeric::operators` |
| accumulators | `boost::accumulators::impl` | `boost::numeric::operators` |
| cobalt | `boost::cobalt::this_coro` | `boost::asio::this_coro` |
| dll | `boost::dll::fs` | `boost::filesystem`（已由 `republish` 处理） |
| icl | `boost::icl::Interval_Set` | `boost::icl::segmental` |
| icl | `boost::icl::Interval_Map` | `boost::icl::segmental` |
| multiprecision | `boost::serialization::cpp_int_detail` | `boost::multiprecision`、`…::backends` |
| numeric | `boost::numeric::ublas::detail` | `boost::numeric::ublas`（两处） |
| poly_collection | `boost::poly_collection::common_impl` | `boost::poly_collection::detail` |
| sort | `boost::sort::blk_detail` | `boost::sort::common` |

ublas 的指令出现在 15 个模块的 TU 里，但只有 numeric 拥有那两个头文件；按
`home_of_file` 的规则，也只有 numeric 会注入它。

**有没有公开写法依赖这些指令。** 一个名字只经由指令可达，指的是它在目标命名空间
中导出、在指令所在命名空间中没有直接声明。在 `upstream/boost/boost`（头文件和宏）
和 `upstream/boost/libs`（测试、示例）中搜索每个命名空间的限定写法
（`ublas::detail::X`、`cpp_int_detail::X`、`blk_detail::X`、`Interval_Set::X`、
`Interval_Map::X`、`common_impl::X`、`accumulators::impl::X`、
`functional::operator…`），再和生成的 `.inc` 对照：

- ublas::detail：11 个不同的名字，9 个是 ublas::detail 中直接声明并已导出的成员
  （`tensor_expression`、`retrieve_extents`、`make_binary_tensor_expression` 等，
  `BOOST_UBLAS_*` 宏展开用到的也是这些）。另外两个，`recursive` 是嵌套命名空间
  （`recursive::mtv` 已导出），`make_lambda` 只出现在注释里。
- cpp_int_detail、blk_detail、Interval_Set、Interval_Map、common_impl、
  accumulators::impl：所有写法都指向该命名空间自己的成员，且全部已直接导出。
- 没有任何代码写 `using namespace …::detail;` 去打开这些命名空间。
- numeric::functional / accumulators::impl：没有经由它们写运算符的代码。
  `functional_fwd.hpp` 中 `namespace operators {}` 旁的上游注释是 "For using
  directives"。

模板实例化不受影响：GMF 中头文件里的非限定查找在定义处进行，该处能看到指令；
导出只决定导入方自己能写出的名字。

结论：三组 detail 名称可以丢弃。另外四处（icl ×2、poly_collection、
accumulators ×2）也都是实现上的便利。cobalt 的指令确实是公开写法
（`cobalt::this_coro::executor`），但已经由 `[[cobalt.extra]]` 处理。

## 决定

新增策略条目 `[[<lib>.skip-directive]]`，字段为 `namespace` 和 `reason`。

- `check_skipped_directives` 在每个重新解析的库上运行。它找出该库自身（或
  "shared"）头文件中所有指向 `boost::` 命名空间、又不是 ADL 屏障形态的 using
  指令。每个这样的命名空间都必须由 `skip-directive` 或 `republish` 声明，否则
  生成失败，报错中给出指令、目标和两种写法。
- 以下情况同样报错：
  - `skip-directive` 没有匹配到任何指令，包括 ADL 屏障形态（会被跟随的）和别的
    库头文件中的指令；
  - 同一个命名空间同时是 `republish`。
- 每个条目以 `<lib> skip-directive <namespace> -- <reason>` 写进
  `skipped.txt`，文件头的说明随之改为也包括"不跟随的 using 指令"。
- 缓存：条目是该库策略段的一部分，策略摘要在缓存 key 里；TU 不变时指令也不变。
  所以缓存命中时不需要重新检查，`skipped` 条目随缓存一起回放。
- 写入了 9 个条目（上表除 dll 外的各行）。每个 reason 写明指令的位置和用途、
  公开写法是什么，以及上游写法的核对结果。detail 三组还写明了跟随时会多出的名称数。
- 输出：只有 `skipped.txt` 多了 9 行，任何 `.inc` / `.deps` 都没有变化。

## 否决的做法

- **只给 issue 中的三组 detail 写条目，其余仍静默跳过**：下一次升级 Boost 时
  新增的指令照样会悄悄丢掉名称。全仓只有 12 处，全部声明的成本很低，此后每处新增
  都必须有人审查。
- **按命名空间名中是否含 `detail` 自动放行**：`Interval_Set`、`common_impl`、
  `functional` 都不含 `detail`。名字也说明不了某个写法是不是公开的。
- **把 cobalt 改成 `republish`**：会多导出 asio 的 `reset_cancellation_state_0_t`
  等 5 个辅助类型，改变 cobalt 的导出面，不属于本轮范围。这里记作
  `skip-directive`，reason 指向现有的 `extra`。
- **`skip-directive` 按（命名空间，目标）逐条声明**：cpp_int_detail 的两条指令
  理由相同。按命名空间声明已经能发现新增的指令所在命名空间，同一命名空间里新增的
  目标在审查 reason 时一并核对。

## 顺带发现（未处理）

accumulators 的 GMF 包含 `boost/accumulators/numeric/functional/vector.hpp` 等
头文件，但 `boost::numeric::operators` 中的运算符模板（`operator/(std::vector…)`
等）没有被任何模块导出。这与 using 指令无关，留待单独分析。

## 测试

`tools/tests/test_gen_exports_skip_directive.py`，覆盖：

- 未声明的指令报错；
- 声明后列出条目；
- `republish` 视为已声明；
- 过期条目、ADL 屏障条目报错；
- 与 `republish` 重复报错；
- 别的库头文件中的指令；
- 不跟随时不产生注入。

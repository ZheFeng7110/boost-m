---
subject: tests
status: active
---

# PR #5 第二轮评审的 [nit] 修复：覆盖率按测试程序列出的用例核对

> 日期：2026-11-21 · mcpp 2026.11.20.1
>
> 来源：[mcpplibs/boost-m#5](https://github.com/mcpplibs/boost-m/pull/5) 第二轮评审的 [nit] 意见及作者的回复；
> [模块封装层测试加固](2026-11-09-module-test-hardening-design.md) 中 `coverage.toml` 的约定。

## 0. 结论

第二轮评审有八条 [nit]。六条在本轮修复，对应的评审线程已标为已解决；另外两条（`ex_free_port`
的端口竞争、看门狗线程在静态析构期间访问静态对象）按作者回复暂缓：
[#7](https://github.com/mcpplibs/boost-m/issues/7) 打算用 mtest 取代这套 asio 测试辅助代码，上游
mtest 已适配 doctest，到时这两处随之消失。[重要] 级的 cobalt `main` 一条见 §5。

改动最大的是 `tools/ci/test_coverage.py check`：`coverage.toml` 中每个 `case` 不再用正则在源文件里
找，而是与**构建出的测试程序**列出的用例比对（§2）。CI 首轮运行即发现解析器的一处错误（§3），
修复后两条 linux-gcc x86_64 腿合起来核对了全部用例。

## 1. 六条 [nit] 的处理

| 评审意见 | 处理 |
|---|---|
| 默认特性集文档不准确 | `docs/usage.md`、`usage.zh.md` 中的闭包列表按 `mcpp.toml` 的 `default` 重写（补 `bind`、`conversion`、`function`）；“50 个库”改为 51（`mcpp.toml` 53 项，与原先口径一致不计 `version`、`macros`），README 三种语言同步。 |
| `ex_peer_async` 遇 reset 无限空转 | 连接失败与“收到数据前被 reset”共用一个 300 次的重试计数，每次重试前 sleep 10 ms。辅助代码被逐字复制进 11 个生成的测试文件，直接对它们做了同一处文本替换（结果与 `regen.py` 输出一致；重跑 `regen.py` 需要发布包与 oracle 构建，没有必要）。 |
| `check` 把注释等处的 `TEST_CASE` 计为已覆盖 | 作者选择“让测试二进制列出用例后再比对”，见 §2。 |
| `EXTRA_HEADERS` 注释称是唯一来源 | 作者选择改注释：头文件集合（GMF）的事实来源是人工维护的 `generated/meta/libs.json`，以 `config` 的 `boost/cstdlib.hpp` 为例说明只在 `libs.json` 中出现的头文件没有归属。 |
| `test_template.py` 夹具 home 残留 | `install_fixture()` 移进 `try`，失败时 `finally` 也删除 `home_root`。 |
| `BOOST_INLINE_CONSTEXPR` 在 standalone 下未定义 | 作者选择 include：`nonfinite_num_facets.hpp` 增加 `#include <boost/config.hpp>`；`patches/math.patch` 中该文件的 hunk 按原始上游文件重新生成，`patches/README.md` 补充说明。本机 gcc 验证：改前 `-DBOOST_MATH_STANDALONE` 报 `'BOOST_INLINE_CONSTEXPR' does not name a type`，改后两种模式都通过。 |

## 2. 用例名来自测试程序

### 2.1 怎样拿到程序列出的用例

考察过三条路：

- **`mcpp test --no-run --archive`**：归档带有文档化的清单 `mcpp-tests.json`（schema 1，写明每个测试的
  程序路径）。否决：归档要复制全部测试程序，约 250 个 dev 配置的 Boost 测试程序可能有数 GB，
  CI 机器磁盘有限。
- **逐个 `mcpp test <test> --print-launch`**：每次都要规划整个构建，几百次调用太慢。
- **`mcpp test --message-format json -- <参数>`**（采用）：JSON 模式下每条记录的 `run_output`
  是程序的完整输出（stderr 合并进来），参数在 `--` 之后原样传给每个程序；沿用同一次构建，
  不重编、不落盘。

两种框架用一组参数同时满足：

```
mcpp test --features ... --message-format json -- --list_content -- --list-test-cases
```

Boost.Test 解析 `--list_content`，把第二个 `--` 之后的内容留给程序；doctest 忽略不认识的参数，
读到 `--list-test-cases`。本机用小程序验证了两种框架的行为，以及 mcpp 会把第二个 `--` 原样转发。
其他程序（普通 `main`、带输出比对 harness 的 Boost.Test 文档示例）照常运行。

### 2.2 解析（`program_cases`）

- doctest：`[doctest] listing all test case names` 之后两条 `=` 分隔线之间的每一行是一个用例名。
  注释里、`#if 0` 里或被特性宏挡住的用例不会出现。
- Boost.Test：每行依次是缩进、名字、状态符（`*` 表示启用，空格表示禁用），有描述时再跟
  `: <描述>`。禁用的单元不运行，不计入。
- 模板用例两种框架都只列出实例（`name<int>`），所以另加去掉 `<…>` 后缀的名字，`coverage.toml`
  引用的是模板本身。
- 非 doctest 程序都算作整体程序 `program`。原先“源文件任意位置出现 `int main(`”的判断随之去掉
  （每个移植的 Boost.Test 文件的 harness 里都有 `main`）。

### 2.3 `check` 的两种模式

- 不带 `--listing`：只检查结构，以及 `case` 指向的测试文件存在，并打印“case names not verified”。
  原有的 coverage 任务（纯 Python，不构建）保持不变。
- `--listing FILE`（可重复）：测试在某个 listing 里出现，就要求它列出该用例；构建或运行失败
  （`compile_fail`、`not_run`）报错；不在任何 listing 中的条目汇总报告为“未核对”，不算错误。
  一个 listing 文件里没有任何测试记录时报错，防止 mcpp 提前失败后整步“通过”。
- 工作区成员的测试名按根清单 `[workspace] members` 映射回 `tests/` 下的路径
  （`boost-m-tests-dll` 的 `examples` 对应 `dll/examples`）。
- `link` 仍用正则按命名约定在源文件中找候选，它只是提议，最终由 `check --listing` 核对。

### 2.4 CI 上由两条腿分工

任何单独一条腿都无法核对全部用例：

- 根工程的测试在 **组 B** 列出：组 A 启用 `unit-test-framework`，`tests/test/included` 的程序在那里是
  空 stub（`int main() { return 0; }`），列不出用例。
- `tests/dll` 只在 **组 A** 运行（“Test sub-projects” 一步），在那里列出。

只放在 linux-gcc x86_64 上，因为各平台的用例集合相同，一条腿就够了。代价是组 B 把根工程的测试
再运行一遍：doctest 程序只列出用例，其余程序照常执行（首轮 CI 约 10 秒）。

## 3. CI 首轮失败与修复

首轮运行（run 44877283747）组 A 通过（dll 23 个用例全部核对），组 B 报一处：

```
tests/test/coverage.toml: ...testorg_reference.qbk#decorator_description: the program of
tests/test/included/doc_reference.cpp lists no test case 'decorator_description'
```

用例存在且会运行。原因在 Boost.Test 的输出格式（`boost/test/impl/unit_test_main.ipp`：先写名字，
再写 `*` 或空格，有描述时追加 `": " << p_description`）：

```
    decorator_description*: described
```

解析器原先要求整行以 `*` 结尾，带描述的单元全部漏掉。修正后先按第一个 `": "` 切掉描述，再看
最后一个字符是否为 `*`。顺序不能反：名字不含 `": "`，但模板实例的类型可能含 `*`
（`tmpl<int*>`），所以从右边找 `*` 是错的。

这次失败说明新检查确实在核对真实程序，而不是在源文件里匹配文本：正则版本恰好能找到
`decorator_description`，它的问题是 §2.2 列出的那些假阳性。

## 4. 未做的事

- `ex_free_port` 端口竞争与看门狗线程：见 §0，随 #7 处理。
- 其他平台不跑 listing 核对：用例集合与平台无关，平台特有的跳过通过 `coverage.toml` 的
  `excluded` 表达，不需要逐平台核对。

## 5. [重要] cobalt 不提供 `main`、geometry 去掉 Fusion 适配器

两处移除本身不变（理由见 `tools/export_policy.toml` 的 `[cobalt]`、`[geometry]` 段），补的是文档与测试：

- `docs/usage.md` §6（及 `usage.zh.md`）记录两处移除与替代写法；`usage.zh.md` §6 同时补上英文版已有的
  `boost::size` 一条。§2.2 增加“`#include` 写在 `import` 之前”的规则。
- 新增 `tests/cobalt/co_main.cpp`：先 `#include <boost/cobalt/main.hpp>`，再
  `import mcpplibs.mwrap.boost.cobalt;`，定义 `co_main`（task 与 channel）。它不是 doctest 程序，
  `main` 来自 `main.hpp`；CI 的每条腿都会构建并运行它。本机 gcc 16.1.0 与 llvm 22.1.8 均通过。
- 顺序的验证：把 include 移到 import 之后，两个编译器都编译失败——gcc 报
  `conflicting declaration of template 'std::reference_wrapper'` 等，clang 报
  `cannot add 'abi_tag' attribute in a redeclaration`。所以文档只给出 include 在前的写法，
  评审提到的“import 之后再 include”是不受支持的。
- geometry 的写法（include `adapted/boost_fusion.hpp` 在前）已由
  `tests/geometry/examples_core_geometries_6.cpp` 覆盖，没有新增测试。

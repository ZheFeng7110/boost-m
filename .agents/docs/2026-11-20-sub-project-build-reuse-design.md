---
subject: tests
status: active
---

# 子工程复用根工程的构建产物，mcpp 升级到 2026.11.20.1

> 日期：2026-11-20 · mcpp 2026.11.5.1 → 2026.11.20.1
>
> 来源：[模块测试加固的遗留问题](todos/module-tests-open-issues.md) 的最后一项；
> [模块封装层测试加固](2026-11-09-module-test-hardening-design.md) §11.5、§11.6 第 5 条。

## 0. 结论

`tests/dll` 与 `tests/test/utf` 成为根工作区的成员，CI 在根目录用
`mcpp test -p <成员> --features mcpplibs.mwrap.boost/<f>,...` 运行它们，构建目录就是根工程的构建目录，
成员转发根工程的 feature 集合，于是复用根工程 `mcpp build` 的全部目标文件与 BMI。本机 gcc 16.1.0、
mcpp 2026.11.20.1 从空 `target/` 按 CI 的顺序实测（§4）：`tests/dll` 10 秒、`tests/test/utf` 5 秒，
都不重编本包；此前各自重建本包时为 172 + 311 秒（CI 上 444 + 471 秒）。

复用依赖 mcpp 的两处行为，2026.11.5.1 都不满足（§2）：feature 宏按激活顺序排列，以及
dev-dependency 的 include 目录进入本包自己的单元。两者在上游已经修好，本轮把 mcpp 固定版本升到
2026.11.20.1，并处理升级带来的三处变化（§3）。

改动：

- 根 `mcpp.toml` 新增 `[workspace] members`：两个子工程与 dll 的七个插件包。`tests/dll` 原有的
  嵌套 `[workspace]` 删去，插件改为根工作区的成员（仍是 `tests/dll` 的 dev-dependency）。
- `tests.yml`：子工程一步改为在根目录 `mcpp test -p <成员> --features <转发组 A>`，位置不变
  （`Test libraries` 之后）；`MCPP_VERSION` 与 `.xlings.json` 升到 2026.11.20.1。
- 依赖键一律写成不加引号的 `mcpplibs.mwrap.boost = …`、`compat.doctest = …`（清单、模板、示例、
  README 与 usage 文档）。
- `tools/ci/test_template.py`：夹具的 `MCPP_HOME` 移出工作树，不再链接捐赠 home 的 ABI 表，描述符
  不再写 `mcpp = "mcpp.toml"`。

## 1. 工作区的作用与对发布的影响

[§5.1](2026-11-09-module-test-hardening-design.md) 当时不用根清单的 `[workspace]`，理由是对已发布
清单的影响未经验证。在 `.tmp/probe-ws` 的最小工程上（mcpp 2026.11.5.1）验证了：

- 有 `[package]` 的根工作区，裸 `mcpp build` / `mcpp test` 只作用于根包，根工程的命令不变。
- 工作区外的消费者以 `path`、`git`（`file://` 仓库加 `rev`）依赖根包都能构建运行；成员目录不存在时
  （例如裁剪过的副本）`path` 依赖照样可用。发布用的 tarball 是 `git archive` 的全树，成员目录都在。
- 成员的产物在 `bin/<包名>/` 下，与根包的 `bin/<lib>/` 测试程序不冲突。

升级后 `examples/basic`、`examples/profiles` 与两个模板（`basic`、`lib`）在本机照常构建运行。

工作区本身不带来复用：同一构建目录里，一个单元的命令与上次相同，ninja 才不重建。

## 2. 单元命令何时相同（2026.11.5.1 上的测量）

### 2.1 feature 宏的顺序

包的每个活动 feature 都以 `-DMCPP_FEATURE_<F>` 进入该包每个单元的命令。2026.11.5.1 按
`feature_closure` 的激活顺序排列这些宏：`[features].default` 与请求依次压栈、后进先出地展开
`implies`；根包作为成员的依赖时，成员清单自己声明的 feature 排在 `--features <dep>/<f>` 转发的前面。
集合相同而顺序不同，第一次实测时 `tests/dll` 的计划里 `dll` 从第 103 位移到了最后，362 个包目标文件
全部重编。按这条规则模拟出的顺序与实测逐项相同。

这一版上唯一的绕法是让每个成员声明的 feature 列表成为组 A 的前缀（组 A 以
`dll,unit-test-framework` 开头，utf 多声明一个用不到的 `dll`），本机实测可行。上游已在
**2026.11.17.2（mcpp#949，起因 mcpp#945）按名字排序这些宏**，所以本轮直接升级，不采用这一绕法，
组 A 保持原来的顺序，utf 只声明 `unit-test-framework`。

另外：`--features <dep>/default` 与成员 `[features] default = ["<dep>/<f>"]` 形式的转发在 2026.11.5.1
中都不生效（探针工程上没有激活任何 feature）。本仓库不受影响，因为 `default` 包含在组 A 的闭包内；
转发中仍写上 `default`，表达请求与根包相同。

### 2.2 dev-dependency 的 include 目录

2026.11.5.1 的根 `mcpp test` 把根包的 dev-dependency（doctest）的 include 目录加到根包**所有**单元上，
作为依赖的根包没有这一项，所以子工程不能与根 `mcpp test` 共用目标文件，而两者写同一路径；当时只能把
子工程排在 `Build libraries` 与 `Test libraries` 之间，复用前者。**2026.11.8.1（mcpp#906）起
dev-dependency 只到达测试单元**（docs/08），本包自己的单元在 `mcpp test` 下与 `mcpp build` 下命令相同。

### 2.3 为什么不放进同一计划

- `-p mcpplibs.mwrap.boost -p boost-m-tests-dll` 同一计划能复用，但计划中出现共享库后，根包测试程序
  的链接去掉了 `-static-libstdc++`，416 个根测试会以另一种方式重新链接；换到不含共享库的计划时又
  重新链接回来。
- `-p boost-m-tests-dll -p boost-m-tests-test-utf` 同一计划时 utf 随 dll 链接共享形态的本包；utf 显式写
  `linkage = "static"` 时两个成员对链接形态有分歧，mcpp 改为逐个规划，而 `--features` 在不声明这些
  feature 的成员上报错。

所以每个成员单独一个 `-p` 计划。

## 3. 升级到 2026.11.20.1 的其他变化

### 3.1 包目标文件按配置分目录

2026.11.19.1 起（mcpp#953 D5）一个包实例的目标文件在 `obj/<包>/<摘要>/`，摘要覆盖该包每个非测试单元
的目标路径与参数。根 `mcpp test` 保留未激活 feature 的基础源文件（这正是本仓库让每条腿跑全部测试
程序的机制），比 `mcpp build` 多 7 个组 B 库的模块接口单元（accumulators、beast、geometry、gil、
mqtt5、polygon、qvm 的 `.m.o`），摘要不同，所以根 `mcpp test` 在另一个目录里把本包完整编译一遍
（359 个目标文件，§4），与升级前的耗时相同；`mcpp build` 的目标文件不再被它覆盖，子工程因而可以放在
任何一步之后。各单元的命令除输出路径外完全相同，只因单元集合不同而不能共用。mcpp 的设计记录
（`2026-11-19-ecosystem-rule-convergence-design.md` §11.1）所说的摘要只覆盖决定命令的因素，不含单元
集合；已报告为 [mcpp#986](https://github.com/mcpp-community/mcpp/issues/986)（附最小复现：只在根为
工作区时出现，去掉 feature 门控后 build 与 test 共用一个实例）。

### 3.2 加引号的依赖键

`"mcpplibs.mwrap.boost" = …` 自 2026.11.8.1 起弃用（2027.2 移除），`--strict` 拒绝；模板渲染出的
就是这种写法，模板测试因此失败。依赖键改为不加引号的点分形式。TOML 把它读成逐段嵌套的表，模板测试
的检查随之按段查找。

### 3.3 模板测试的夹具

- 描述符中的字符串 `mcpp = "mcpp.toml"` 自 2026.11.19.1 起只在版本目录下精确匹配，不再查找下一层
  的 `*/mcpp.toml`。夹具把包放在 `1.92.0/boost-1.92.0/`，与真实 tarball 的形状相同；mcpp-index 中的
  mwrap 描述符不写这一字段，走默认查找，不受影响。夹具删去这一字段。
- 2026.11.19.1 起 `path` 包按整棵目录树的内容摘要进入全局缓存（mcpp#974）。夹具的 `MCPP_HOME` 原在
  工作树的 `.tmp/` 下，构建期间写入 home 改变了包的目录树，`--strict` 报"sources changed while it was
  built"。home 移到系统临时目录；生成的工程自带 `mcpp.toml`，不计入摘要，仍留在工作树中，让 xlings
  shim 找到固定版本的 mcpp。
- 夹具原来以符号链接共享捐赠 home 的 `abi`、`abi.gen`、`abi.json`。新版在夹具 home 中重新生成 ABI 表，
  经链接改写了捐赠 home 的 `abi.json`（写入夹具的 glibc 路径）并清掉了捐赠 home 的 `abi` 所指的那一代，
  捐赠 home 中 gcc 的解释器从此找不到（本机实际发生，删去三项后由 `mcpp toolchain install` 重建）。
  CI 的 `~/.mcpp` 是跨运行缓存的，所以这会污染后续运行。夹具不再链接这三项，由 mcpp 在夹具 home 中
  生成。

## 4. 实测

本机（x86_64 Linux，gcc 16.1.0，mcpp 2026.11.20.1，`MCPP_JOBS=4`），从空 `target/` 依次执行 CI
组 A 的步骤：

| 步骤 | 耗时 | 包目标文件编译数 |
|---|---|---|
| `mcpp build --features <A>` | 150 秒 | 352 |
| `mcpp test --features <A>` | 920 秒 | 359（另一配置目录，§3.1；另有 416 个测试 TU） |
| `mcpp test -p boost-m-tests-dll` | 10 秒 | 0 |
| `mcpp test -p boost-m-tests-test-utf` | 5 秒 | 0 |

全部通过（根 416、dll 4、utf 1 个测试）。组 B 的 `mcpp build`（64 秒）与 `mcpp test`（1109 秒，416 个测试）同样通过。

对照（mcpp 2026.11.5.1，改动前的方式，在根 `mcpp test` 之后运行）：`tests/test/utf` 311 秒、
`tests/dll` 172 秒，各重编 362 个包目标文件。

Windows（clang-msvc）、macOS 与 arm64 musl 腿没有在本机验证。

## 5. 否决的方案

- **在子工程目录里运行并依赖全局 BMI 缓存。** 这正是原来的做法：子工程有自己的 `target/`，CI 上
  两个子工程仍各自重建本包（§11.5 的 444 与 471 秒）。
- **停留在 2026.11.5.1，用 feature 前缀绕过 §2.1。** 可行，但要让 utf 声明用不到的 `dll`，并把子工程
  卡在两个根步骤之间；上游已修复，升级更干净。
- **成员不声明 feature，全部由 CLI 转发。** 成员单独运行时缺少所需的 feature，清单不再自洽。
- **把根测试与子工程放进一个 `--workspace` 计划。** 见 §2.3。

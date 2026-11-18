---
subject: tests
status: landed
---

# windows-llvm-msvc 上的进程崩溃：两个 clang 模块缺陷

> 日期：2026-11-18 · 分支 `fix/windows-msvc-crashes`（堆叠在 `test/module-test-hardening` 上）
>
> 来源：[模块测试加固的遗留问题](todos/module-tests-open-issues.md) 的第一项；
> [模块封装层测试加固](2026-11-09-module-test-hardening-design.md) §11.6 第 1 条。

## 0. 结论

三处崩溃都不是封装层的配置问题，而是 clang 在 MSVC ABI 下处理 C++20 模块时的两个缺陷，
llvm 22.1.8 与 23.1.3 上都会出现：

| 崩溃 | 原因 | 处理 |
|---|---|---|
| asio `composed_4`、`cobalt/doc`：堆损坏（0xC0000374） | 导入方展开模块中的 throw 时，生成的 CatchableType 缺拷贝构造函数（clice-io/xclang#27，修复为未合入的 llvm/llvm-project#114075），`exception_ptr` 按位复制异常对象，消息被释放两次 | 在该 TU 中让 Sema 检查一次同类型的 throw（不调用的函数） |
| math `daubechies_scaling`：SIGSEGV | std 模块与 math 模块 GMF 各有一份 MSVC 的 ppltasks，合并后 `_GetStaticAmbientSchedulerRef` 的 `call_once` lambda 写入的是该函数局部静态变量的另一份副本（与 llvm/llvm-project#221548 同形），`std::async` 读到空的调度器 | `tests/math/doc_sf.cpp` 先导入 math 再导入 std |

原先怀疑的方向（asio 的 `aligned_new` / `aligned_delete` 配对、`call_stack<>::top_` 在不同 GMF 中以
不同配置实例化）不成立。三项跳过都已删去。

## 1. 复现

Windows 主机上的实验通过 fork 上的堆叠 PR 进行：工作流 `windows-crash-repro.yml` 只构建相关测试程序，
先直接运行，再在 cdb 下运行（必要时开 full page heap）。三项都是**确定性**复现：composed_4 与
cobalt 文档程序每次以 0xC0000374 结束（cobalt 每次停在 `doc: race`），daubechies 每次 SIGSEGV。

Windows 腿的有效编译参数：`-fms-runtime-lib=dll -O0 -g`、MSVC 14.51.36231、
`-fms-compatibility-version=19.51.36260`，没有 `_DEBUG` / `_ITERATOR_DEBUG_LEVEL` 的差异。

## 2. 堆损坏：CatchableType 缺拷贝构造函数

cdb 在 0xC0000374 处的栈：`future<void>::get` → `rethrow_exception` → 展开时 `~future` →
`~exception_ptr` → `_ExceptionPtr_normal::_Destroy` → `~system_error` → `__std_exception_destroy` →
`free` 报堆损坏；page heap 下是 verifier stop 7（块已释放）。

同一个程序里，`boost::system::system_error` 的 CatchableType 有两份：

```
_CT??_R0?AVsystem_error@system@boost@@@848                                         # 无拷贝构造函数
_CT??_R0?AVsystem_error@system@boost@@@8??0system_error@system@boost@@QEAA@AEBV012@@Z48
```

cobalt 文档程序里另有 `std::bad_exception`、`with_throw_location<bad_exception>`、
`with_throw_location<system_error>` 的无拷贝构造函数版本。`_CT` / `_TI` 是 COMDAT-any，链接器可能
保留缺拷贝构造函数的一份；`make_exception_ptr` / `current_exception` 于是按位复制，两份对象共用
`what()` 的缓冲区。

原因（clice-io/xclang#27 的分析）：`Sema::CheckCXXThrowOperand` 把选中的拷贝构造函数记在
`ASTContext` 的表中，这张表不写入 BMI；导入方展开来自模块的 throw（非模板 inline 函数，或模块
已实例化过的特化）时查不到，CatchableType 就不带拷贝构造函数。

用最小程序验证（每个用例单独一个进程）：

| 程序 | 结果 |
|---|---|
| `import std` + asio 模块，只有 use_future 的错误路径 | 0xC0000374 |
| 同上，另有一个不调用的 `throw boost::system::system_error(...)` | 通过 |
| 只用文本包含，或只 `import std` 的 `make_exception_ptr` / `promise::set_exception` | 通过 |

TU 自己检查过该类型的 throw，表中就有条目，本 TU 生成的所有该类型（及其基类）的 CatchableType 都带
拷贝构造函数。因此：

- `tools/port_tests/helpers/asio.hpp`（内联进 `tests/asio/` 的 7 个移植文件）与 `tests/cobalt/doc.cpp`
  在 `_MSC_VER` 下各加一个不调用的函数，throw 受影响的类型；cobalt 用两个 `with_throw_location<>`，
  其基类覆盖其余类型。
- composed_4 去掉 `--only-if`；它打印 EINVAL 的系统文本（Windows 上是 WSA 的
  “An invalid argument was supplied”，10022），输出只在 Windows 上不比较（`--output-unless`）。

这是使用方同样会遇到的问题：任何复制 Boost 异常的 TU（`use_future`、cobalt 的 promise / race、
`std::current_exception`）都可能中招。[docs/usage.md](../../docs/usage.md) §5 写明了现象与上述绕过。

## 3. SIGSEGV：ppltasks 的局部静态变量被合并成两份

崩溃栈：`daubechies_scaling` → `std::async` → `Concurrency::task_options::task_options` → 复制
`get_ambient_scheduler()` 返回的 `shared_ptr` 时读 `[0+8]`，即返回的引用本身是空地址。MSVC 14.51
的 `pplwin.h`：

```cpp
inline ::std::shared_ptr<scheduler_interface> & _GetStaticAmbientSchedulerRef()
{
    static ::std::once_flag _Flag;
    static ::std::shared_ptr<scheduler_interface> * _S_scheduler_address;
    ::std::call_once(_Flag, [] {
        _S_scheduler_address = _GetStaticAmbientSchedulerStorage();
    });
    return *_S_scheduler_address;
}
```

出错的程序里这个静态变量有两份：

```
?_S_scheduler_address@?1??_GetStaticAmbientSchedulerRef@details@Concurrency@@YAAEAV?$shared_ptr@Uscheduler_interface@Concurrency@@@std@@XZ@4PEAV45@EA
?_S_scheduler_address@?1??_GetStaticAmbientSchedulerRef@details@Concurrency@@YAAEAV?$shared_ptr@Uscheduler_interface@Concurrency@@@std@@XZ@4PEAV45@EA.1
```

`.1` 是 LLVM 给同一目标文件中第二个同名全局变量加的后缀：clang 为 lambda 引用的变量另生成了一个
声明，函数体读一份、lambda 写另一份。MSVC STL 的 std 模块在 purview 中包含 `<future>`，math 模块的
GMF 经 `daubechies_scaling.hpp` 文本包含同一头文件，导入方合并两份定义时出错。这与
llvm/llvm-project#221548（lambda 的 `operator()` 与外层函数体分别取自两份模块副本）同形，但后者只
讨论了模板。

是否出错取决于 TU 先读到哪一份：

| 程序 | 结果 |
|---|---|
| 只 `import std`，或只用文本包含 | 通过 |
| `import std; import …asio;` + `std::async` | SIGSEGV |
| `import …asio; import std;` + `std::async` | 通过 |
| `import std; import …math;` + 只有 daubechies 一个用例 | 通过 |
| `tests/math/doc_sf.cpp`（std 在前） | SIGSEGV |
| `tests/math/doc_sf.cpp`（math 在前） | 通过 |

`doc_sf.cpp` 改为先导入 math。这是经验性的绕过，在其他测试程序中不能保证；修复要靠 clang。

## 4. 否决的方向

- **升级工具链**：llvm 23.1.3（mcpp 可装的最新版）上三项全部照旧。带 #114075 补丁的 xclang 23.1.5.2
  不在 xlings 中。
- **让封装层统一处理**：缺陷出在导入方 TU 的代码生成，模块无法替导入方补上 Sema 的表项；
  留下哪一份 `_CT` 由链接器决定，封装层无法让完整的一份胜出。
- **page heap 下由 cdb 截停**：verifier stop 7 直接结束进程，不进调试器；改为不开 page heap，
  在 0xC0000374 处截停后取得释放栈。

## 5. 遗留

- clang 合入 #114075 后，删去 `helpers/asio.hpp` 与 `cobalt/doc.cpp` 中的 throw；#221548 一类的合并
  缺陷修好后，`doc_sf.cpp` 的导入顺序可以恢复。
- 实验工作流 `windows-crash-repro.yml` 与驱动脚本 `.github/repro/windows_crash.py` 在合入前删去，见分支历史。

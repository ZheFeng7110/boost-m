# 模块测试加固的遗留问题

来源：[模块封装层测试加固](../2026-11-09-module-test-hardening-design.md) §11.6。各项在源码旁有注释，
处理完后删去对应的跳过或守卫。

- [ ] **windows-llvm-msvc 上两个 clang 模块缺陷的绕过**（原因见
  [记录](../2026-11-18-windows-msvc-module-crashes-analysis.md)）。llvm/llvm-project#114075 合入后，删去
  `tools/port_tests/helpers/asio.hpp`（及重新生成的 `tests/asio/*.cpp`）与 `tests/cobalt/doc.cpp` 中补全
  CatchableType 的 throw；ppltasks 局部静态变量合并的缺陷修好后，`tests/math/doc_sf.cpp` 可恢复
  `import std` 在前。两处都要在 Windows 上确认。
- [x] **aarch64 Linux 上 `ooura_fourier_cos<double>` 返回 NaN**：上游问题，与封装层无关。该腿以
  musl 为 C 库，musl 的 113 位 `expl`/`expm1l`/`log1pl` 直接调用 `double` 版本，最后一个小节点上
  `exp(-eta)` 溢出成 `inf`，权重成为 NaN 而节点为 0，被收入求和；sine 版因 `node <= 0` 即停而不受影响。
  `patches/math.patch` 让 cosine 的小节点循环在权重非有限时停止，`doc: fourier_integrals` 的跳过已删去
  （分析见[发现记录](../2026-11-12-module-tests-findings-and-upstream-bugs-analysis.md) §4 #6）。
- [x] **Windows 上的 `tests/dll`**：mcpp 的 `windows_auto_export` 对以共享形态构建的依赖包同样生效，
  会生成 `.def`。但只要 DLL 的某个输入目标文件自己声明了导出（`.drectve` 中的 `-export:`），mcpp 就把
  导出面交给这些声明，不再发现其余符号；GNU ld 的 auto-export 规则相同。本包的 352 个目标文件中有 35 个
  带这种声明，都与 Boost 的 DLL 构建无关：thread `win32/tss_dll.cpp` 里消除 LNK4221 的 `foo`、
  `codecvt_null<wchar_t>` 无条件的 `BOOST_SYMBOL_EXPORT`、serialization 的 `BOOST_DLLEXPORT`。结果
  `.def` 为空。`patches/thread.patch` 与 `patches/serialization.patch` 去掉了这三处（说明见
  `patches/README.md`），Windows 上 `.def` 有 50689 项（PE 上限 65535），`tests/dll` 四个程序全部通过。
  `tests.yml` 的 Windows 跳过已删去（实验见 fork PR #3）。
- [ ] 子工程（`tests/dll`、`tests/test/utf`）各自重新构建本包，A 组因此多约 15 分钟；研究复用根工程的
  构建产物。

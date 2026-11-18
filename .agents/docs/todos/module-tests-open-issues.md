# 模块测试加固的遗留问题

来源：[模块封装层测试加固](../2026-11-09-module-test-hardening-design.md) §11.6。各项在源码旁有注释，
处理完后删去对应的跳过或守卫。

- [ ] **windows-llvm-msvc 上两个 clang 模块缺陷的绕过**（原因见
  [记录](../2026-11-18-windows-msvc-module-crashes-analysis.md)）。llvm/llvm-project#114075 合入后，删去
  `tools/port_tests/helpers/asio.hpp`（及重新生成的 `tests/asio/*.cpp`）与 `tests/cobalt/doc.cpp` 中补全
  CatchableType 的 throw；ppltasks 局部静态变量合并的缺陷修好后，`tests/math/doc_sf.cpp` 可恢复
  `import std` 在前。两处都要在 Windows 上确认。
- [ ] **aarch64 Linux 上 `ooura_fourier_cos<double>` 返回 NaN**（`tests/math/doc_numerics.cpp` 的
  `doc: fourier_integrals`，在 113 位 `long double` 上跳过）。先在该平台上用纯 include 的程序确认是
  上游问题还是封装层问题。
- [ ] **Windows 上的 `tests/dll`。** 本包以 `linkage = "shared"` 构建的 DLL 不导出包内编译源码的符号
  （`boost::filesystem::detail::path_algorithms::*`、`boost::system::error_category` 的转换运算符），
  测试程序链接失败；`.github/workflows/tests.yml` 在 Windows 上跳过 `tests/dll`。确认 mcpp 的
  `windows_auto_export` 对以共享形态构建的依赖包是否生效。
- [ ] 子工程（`tests/dll`、`tests/test/utf`）各自重新构建本包，A 组因此多约 15 分钟；研究复用根工程的
  构建产物。

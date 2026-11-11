# 模块测试加固的遗留问题

来源：[模块封装层测试加固](../2026-11-09-module-test-hardening-design.md) §11.6。各项在源码旁有注释，
处理完后删去对应的跳过或守卫。

- [ ] **windows-llvm-msvc 上的进程崩溃。** asio 的 `composed_4`（`tests/asio/examples_cpp*.cpp`，生成脚本的
  `--only-if`）与整个 `tests/cobalt/doc.cpp` 以堆损坏（0xC0000374）结束进程，
  cobalt 文档程序每次崩在不同用例；`tests/math/doc_sf.cpp` 的 `daubechies_scaling` 求值以 SIGSEGV
  结束。只在这一条腿上出现。需要在 Windows 主机上复现；怀疑方向是同一段 asio 内联代码在不同模块的
  GMF 中以不同配置实例化（如 `aligned_new` / `aligned_delete` 的配对、`call_stack<>::top_`）。
- [ ] **aarch64 Linux 上 `ooura_fourier_cos<double>` 返回 NaN**（`tests/math/doc_numerics.cpp` 的
  `doc: fourier_integrals`，在 113 位 `long double` 上跳过）。先在该平台上用纯 include 的程序确认是
  上游问题还是封装层问题。
- [ ] **Windows 上的 `tests/dll`。** 本包以 `linkage = "shared"` 构建的 DLL 不导出包内编译源码的符号
  （`boost::filesystem::detail::path_algorithms::*`、`boost::system::error_category` 的转换运算符），
  测试程序链接失败；`.github/workflows/tests.yml` 在 Windows 上跳过 `tests/dll`。确认 mcpp 的
  `windows_auto_export` 对以共享形态构建的依赖包是否生效。
- [ ] 子工程（`tests/dll`、`tests/test/utf`）各自重新构建本包，A 组因此多约 15 分钟；研究复用根工程的
  构建产物。
- [ ] 决定是否把移植脚本（`port_examples.py`、各库的 `regen_*.py`、harness 头文件，目前在本地
  `.tmp/scripts/`）整理进 `tools/`，以便升级 Boost 时重跑。

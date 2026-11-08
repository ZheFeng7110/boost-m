# 设计记录

<!-- 本文件由 tools/doc-tools/gen_index.py 生成，不要手工编辑；CI 检查它与记录一致。 -->

本目录保存每次改动的理由：调研了什么、测到了什么、决定了什么、否决了什么。
一篇记录描述的是它写成的那一刻，**落地后不再修改**，因此这里的内容都不是对
现状的陈述；现状见 [docs/](../../docs/)。

## 规则

- 文件名为 `YYYY-MM-DD-<slug>.md`，slug 只用小写字母、数字、`.` 与 `-`；
  建议按性质以 `-design`、`-plan` 或 `-analysis` 结尾。
- 2026-11-08 及以后的记录必须以 front matter 开头（更早的记录不补）：

  ```yaml
  ---
  subject: generator                  # 简短、可复用的主题词
  status: active                      # active | landed | superseded | abandoned
  superseded_by: 2026-12-01-....md    # 仅 status 为 superseded 时，写文件名
  ---
  ```

- 落地后只允许修改 front matter 的 `status` / `superseded_by`，以及由脚本完成的
  路径修正。结论变了就写一篇新记录，并把旧记录标为 `superseded`。
- **不删除记录。** 往年且不处于 `active` 的记录由
  `tools/doc-tools/archive_records.py` 移入 `archive/<YYYY>/`，引用随之改写。
- `todos/` 放尚未完成的工作项，它们不是决定的记录。
- 移动或改名用 `tools/doc-tools/move_record.py`，它同时改写仓库中对该文件的引用。

共 21 篇记录（其中 0 篇已归档），0 项待办。

## 按主题

只列出声明了 subject 的记录。

### docs

- [设计记录的管理：只增不删、按年归档、生成的索引与 `tools/doc-tools/`](2026-11-08-design-records-management-design.md) — landed

## 按日期

### 2026-11

- [设计记录的管理：只增不删、按年归档、生成的索引与 `tools/doc-tools/`](2026-11-08-design-records-management-design.md) — landed
- [封装层生成原理改造：clang JSON AST + 声明式策略 + `--check`](2026-11-05-json-ast-generation.md)

### 2026-10

- [与 Boost 上游兼容](2026-10-28-upstream-compat.md)
- [mcpplibs `mwrap` 模块封装层生成原理技术报告](2026-10-28-mwrap-module-wrapper-generation.md)
- [Boost 1.92.0 迁移到 mwrap 规范](2026-10-26-boost-mwrap-migration.md)
- [生成脚本幂等性 CI](2026-10-18-generated-scripts-ci.md)
- [macOS LLVM exception unwinding workaround](2026-10-10-macos-lld-unwind-workaround.md)

### 2026-09

- [macOS-llvm 腿 program_options 测试 SIGABRT 修复 (Mach-O typeinfo 兜底回归)](2026-09-25-macos-typeinfo-catch-regression.md)
- [发布后自动登记 mcpp-index 并开 PR (2026-09-23)](2026-09-23-mcpp-index-auto-register.md)
- [GitCode 镜像同步与 Release 发布 (2026-09-23)](2026-09-23-gitcode-mirror-release.md)
- [Boost 1.92.0 上游升级 (vendored 重导 + 产物重生成)](2026-09-23-boost-1.92-upgrade.md)
- [特性宏 profile (`backend-*`) 设计 (2026-09-21)](2026-09-21-feature-macro-profiles.md)
- [内部链接对象导出补全 — range / multi_array / accumulators / mqtt5](2026-09-20-internal-linkage-export-completion.md)
- [变量模板初始化器跨模块 `.deps` 边补全 (A1+A2) + 消费者 smoke (D)](2026-09-19-var-template-initializer-deps-plan.md)
- [MinGW sysroot 一键引导 (scripts/fetch_mingw_sysroot.py)](2026-09-19-mingw-sysroot-bootstrap.md)
- [变量模板导出 (libclang UNEXPOSED_DECL 分类)](2026-09-17-variable-template-export.md)
- [gen_exports.py 增量缓存设计（方案 A + C）](2026-09-09-gen-exports-caching.md)
- [`boost.version` 模块设计 (C5, 2026-09-09)](2026-09-09-boost-version-module.md)
- [计划 — 发布预览版 (Preview Release)](2026-09-08-release-preview-plan.md)
- [dep_graph 结构性缺陷修复 — 两遍扫描体引用边补全 (pass 2)](2026-09-08-dep-graph-two-pass-body-edge-completion.md)
- [boost-module 总体设计汇总 (2026-09-08 重写版)](2026-09-08-consolidated-design.md)

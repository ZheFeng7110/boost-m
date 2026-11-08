---
subject: docs
status: landed
---

# 设计记录的管理：只增不删、按年归档、生成的索引与 `tools/doc-tools/`

> 日期：2026-11-08 · 分支 `docs/agents-records`
>
> 参考：[mcpp-community/mcpp](https://github.com/mcpp-community/mcpp) 的
> `.agents/docs/`（约 350 篇记录、生成的索引）与 `docs/specs/`

## 0. 结论

1. `.agents/docs/` 下的记录**只增不删**。一篇记录描述它写成的那一刻，落地后只允许改
   front matter 的 `status` / `superseded_by`，以及由脚本完成的路径修正。结论变了就写
   新记录，并把旧记录标为 `superseded`。
2. 目录变大的问题由**归档与索引**解决，不靠删除：往年且不处于 `active` 的记录移入
   `.agents/docs/archive/<YYYY>/`，`.agents/docs/README.md` 由脚本从记录生成。
3. 2026-11-08 及以后的记录必须声明 front matter（`subject`、`status`，必要时
   `superseded_by`）。更早的 20 篇不补写，按日期和标题进入索引，检查也不覆盖它们。
4. `.agents/` 下只有 `docs/` 与 `skills/`。原 `.agents/plan/` 的两篇计划并入
   `.agents/docs/`，未完成的工作项以后放在 `.agents/docs/todos/`。
5. 管理、检查、移动文档的脚本统一放在 `tools/doc-tools/`。建议只用 Python 标准库，
   但不强制；需要第三方库时写进 `tools/pyproject.toml` 的 `dependencies`。
   CI 是一个独立的工作流 `.github/workflows/check-docs.yml`。

## 1. 背景：删旧文档换一篇汇总的代价

[2026-09-08 汇总](2026-09-08-consolidated-design.md)替代并删除了此前全部 19 篇
计划与设计文档，只留下一个提交哈希 `03f6161` 作为出处。实测发现：

- 本仓库的历史从 2026-10-26 的 `Initial commit` 开始，共 29 个提交，
  `git cat-file -t 03f6161…` 报错，**该提交不在本仓库中**。它在旧仓库
  `ZheFeng7110/boost-module` 的 `b1.91.0wdev` 分支上。本次给汇总文档加上了旧仓库与
  该提交 `.agents/` 的链接，这是删除式汇总唯一还能补救的部分。
- 汇总只保留结论。例如
  [两遍扫描](2026-09-08-dep-graph-two-pass-body-edge-completion.md)
  要求「不得复现 C4 §4.2 两案否决记录中的副作用」，C4 原文已不在本仓库。
- 汇总文档此后又被修订（C5），§10 列着「待后续对话修正」的问题。它同时承担历史、现状和
  待办三种角色，越写越长，并与 `docs/architecture.md` 产生漂移。

这些代价都来自**删除**和**把现状写进记录**，与记录的数量无关。mcpp 有约 350 篇记录
仍然可用，原因有两点：读者不需要通读记录，现状另有一处回答；每篇记录有状态，索引可以
按状态和主题过滤。

## 2. 约束

- 旧记录不改写。为补 front matter 而修改 20 篇旧记录，本身就违反第 1 条。
- 脚本必须跨平台：CI 只在 Linux 上运行，但维护者也在 Windows（MinGW）上工作。
- 不扩大 `verify-scripts.yml`。它的触发路径和 240 分钟的生成校验与文档无关。

## 3. 方案

### 3.1 布局

| 位置 | 内容 |
|---|---|
| `.agents/docs/YYYY-MM-DD-<slug>.md` | 当年的记录，以及往年仍处于 `active` 的记录 |
| `.agents/docs/archive/<YYYY>/` | 往年且已落地、被取代或放弃的记录 |
| `.agents/docs/todos/` | 尚未完成的工作项，不是决定的记录 |
| `.agents/docs/README.md` | 生成的索引：进行中、按主题、按月、归档按年、待办 |

slug 只用 `a-z 0-9 . -`。`-design` / `-plan` / `-analysis` 后缀是建议，不做检查。
因此 `2026-09-19-var-template-initializer-deps.md` 移入时改名为 `…-plan.md`，
另一篇原本就以 `-plan` 结尾，未改名。

**归档而不是按年建目录。** 另一个可选方案是所有记录从一开始就写进 `<YYYY>/`，
这样文件永不移动，引用永不改写。没有采用它，原因有二：其一，当年的记录直接放在
`.agents/docs/` 下，写记录的人和 agent 不需要知道目录规则；其二，移动发生在每年一次、
由脚本执行，引用改写也由同一个脚本完成（§3.3）。付出的代价是：仓库外部指向
`blob/main/.agents/docs/<name>` 的链接在归档后失效。

**`superseded_by` 写文件名而不是路径。** 这样后继记录被归档时，这个字段不需要改写。
因此检查要求所有记录的文件名唯一（含归档）。

### 3.2 front matter

```yaml
---
subject: generator      # 简短、可复用的主题词：a-z 0-9 -
status: landed          # active | landed | superseded | abandoned
superseded_by: 2026-12-01-....md   # 仅 superseded
---
```

是否必须声明 front matter 由文件名中的日期决定：不早于 `FRONT_MATTER_SINCE`
（2026-11-08）就必须声明。考虑过列出旧记录的豁免名单，但日期判据不需要维护名单，
而且 agent 总是按当天日期命名。旧记录若事后加了 front matter（例如标为 `superseded`），
同样按上述格式校验。

不从文件名推断 subject：猜错的分组比不分组更误导读者。

### 3.3 `tools/doc-tools/`

| 脚本 | 作用 |
|---|---|
| `records.py` | 共用库：记录的发现与分类、front matter、标题、链接解析、移动与引用改写 |
| `gen_index.py` | 生成索引；`--check` 在过期时以 1 退出 |
| `check_docs.py` | 八项检查，见文件头 |
| `move_record.py` | 移动或改名任意文档，改写引用，必要时重建索引 |
| `archive_records.py` | 把往年非 `active` 的记录移入 `archive/<YYYY>/`；拒绝归档当年 |

引用改写分两类：

- Markdown 内联链接按**两端的新位置**重新计算相对路径。被移动的文件自己的链接也会重算，
  例如 `../../docs/x.md` 在归档后变为 `../../../../docs/x.md`。fenced code 中的链接不改。
- 任意被跟踪文本文件中出现的仓库相对路径（如 `.agents/docs/<name>`，常见于注释和
  code span）整体替换。

这两类之外的写法不会被识别。例如汇总文档按 `.agents/` 相对写的 `plan/x.md`，本次是
手工修正的；移动后应再按文件名搜索一次。

跨平台的做法：

- 路径一律用 `pathlib`，并以 `/` 输出；
- 读写使用 UTF-8 和 `newline=""`，改写时保留文件原有的 CRLF；
- 索引写出时固定为 LF，比较前把 CRLF 归一化为 LF，因此 `core.autocrlf=true`
  的 Windows 检出不会被误判为过期；
- 移动优先用 `git mv`，不可用时退回 `Path.rename`。

### 3.4 检查的边界

`check_docs.py` 检查：

- 文件放在哪里，以及文件名是否合规；
- front matter 的格式；
- 索引是否最新；
- 新记录与用户文档（`docs/**`、顶层 `README*.md`）里的相对链接是否能打开；
- 用户文档不引用 `.agents/`。用户文档不应把读者送去一份不保证稳定的记录。

旧记录不检查链接：它们链接的 `docs/zh/architecture.md`、
`docs/github-release-process.md` 等文件已经不存在，而这些记录本身不改写。

以下几项**不检查**：

- 已落地的记录事后是否被改过。区分「允许的改动」和「改写结论」需要读者，
  留给 review；
- 文档是否与实现一致。

### 3.5 CI

`.github/workflows/check-docs.yml`：只在 `.agents/**`、`docs/**`、`README*.md`、
`tools/doc-tools/**` 或该工作流自身变化时触发，在 `ubuntu-latest` 上使用 Python 3.11
（与 `tools/pyproject.toml` 的下限一致），先跑 `tools/doc-tools/tests`，再跑
`check_docs.py`。

## 4. 本次的改动

- `.agents/plan/` 的两篇计划由 `move_record.py` 移入 `.agents/docs/`，
  `2026-09-19-mingw-sysroot-bootstrap.md` 中的路径被自动改写。这次移动也是脚本在真实
  仓库上的第一次使用。
- 汇总文档：在提交哈希后和 §11 中加了旧仓库链接；把 `plan/2026-09-08-release-preview-plan.md`
  改为 `docs/…`。
- 未改动：`tools/boost_common.py`、`tools/gen_features.py` 和
  `2026-09-09-boost-version-module.md` 引用的
  `.agents/plan/2026-09-09-boost-version-module-promotion.md`。查旧仓库全部历史，
  这份文件从未被提交过，这些引用在迁移前就已失效。

## 5. 验证

- `python3 -m unittest discover -s tools/doc-tools/tests`：13 项全部通过。覆盖索引生成、
  CRLF 归一化、新旧记录的 front matter 判据、`superseded_by` 解析、放置规则、
  用户文档的引用方向、双向链接改写、保留 CRLF、跳过 fenced code，以及归档时保留
  `active` 记录并拒绝当年。
- `python3 tools/doc-tools/check_docs.py` 在本分支上通过。
- `archive_records.py --dry-run --today 2027-01-02` 列出了 2026 年的全部记录。

## 6. 后续

- 把汇总文档中仍然有效的约束迁入 `docs/architecture.md` 或新建的 `docs/specs/`，
  然后把汇总文档标为 `superseded`。
- 把反复出现的流程写成 `.agents/skills/`（上游升级、发布、重新生成导出），并加入口文件
  `AGENTS.md`。

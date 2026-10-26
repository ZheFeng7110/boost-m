# Retained pre-mwrap wrapper

This directory holds the wrapper contribution this repository succeeds, kept
because its licence and attribution continue to apply to the parts that were
migrated. It is **not** vendored upstream Boost — that tree is
`upstream/boost/`, under its own BSL-1.0 licence.

| Field | Value |
| --- | --- |
| Repository | `https://github.com/ZheFeng7110/boost-module` |
| Branch | `b1.92.0wdev` |
| Migrated commit | `6393bd80` (`release: v1.92.0.0.0.1`) |
| Design baseline named by the migration doc | `77c6b48f4f6a0f0564f8e4a17cbc029468299067` |
| Old package identity | `ZheFeng7110.boost`, version `1.92.0.0.0.1`, modules `boost.*` |
| Licence | BSL-1.0 — see `LICENSE` in this directory |
| Migrated by | `mcpplibs/boost-m`, package `mcpplibs.mwrap.boost@1.92.0` |

## What was migrated

The module interfaces, generated export lists, vendored Boost tree, patches,
tests, generators and design history moved into this repository and were
re-identified as documented in `.agents/docs/2026-10-26-boost-mwrap-migration.md`.
The migration is a new identity, not an in-place rename: the old package
version, tag and source archives stay immutable and are not republished here.

## What stays here only

- `LICENSE` — the BSL-1.0 licence of the old wrapper contribution.
- `README.md`, `README_zh.md` — the old package READMEs (English, 简体中文).
- `CHANGELOG.md`, `CHANGELOG_zh.md` — the old changelog.
- `release_notes/` — the old release notes, including the `1.91.x` and
  `v1.92.0.0.0.x` series; `release_notes/zh/` keeps their Chinese versions.

These files keep their original text and links. They are history, not current
documentation: current consumers read `/README.md`, `/docs/usage.md` and
`/docs/architecture.md`.

## Licence boundary

The Apache-2.0 licence and `mcpp-community` notices at the repository root apply
to the new wrapper contribution. They do not relicense this directory, nor
`upstream/boost/`. `tools/package-source.sh` deploys all three under distinct
paths so a consumer bundle keeps each notice with its own bytes.

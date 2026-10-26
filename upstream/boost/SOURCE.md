# Vendored Boost source

This directory is the pinned official Boost release, pruned for a wrapper
repository. It contains upstream code only: wrapper interfaces live in `src/`,
wrapper TUs and headers in `port/`, adaptations in `patches/`.

| Field | Value |
| --- | --- |
| Upstream project | Boost |
| Upstream version | 1.92.0 (BOOST_VERSION 109200) |
| Source URL | https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.gz |
| Archive | `boost_1_92_0.tar.gz` |
| Archive SHA-256 | `c4a3b310ddd2472416e091067166b0713be97c63f38c212c484ada022fd296ce` |
| Imported by | `tools/import_boost.py` |

The upstream `README.md` and `LICENSE_1_0.txt` are kept byte-for-byte from the
release archive; the Boost licence (BSL-1.0) applies to this tree. The wrapper
contribution is Apache-2.0 — see `/LICENSE`, `/NOTICE` and
`docs/architecture.md` §Licensing.

## Kept top-level entries

- `CMakeLists.txt`
- `LICENSE_1_0.txt`
- `README.md`
- `boost`
- `libs`
- `tools`


## Pruned directories (any depth)

- `.appveyor`
- `.azure-pipelines`
- `.ci`
- `.circleci`
- `.github`
- `.travis`
- `doc`
- `docs`
- `example`
- `examples`
- `more`
- `status`


## Pruned files (any depth)

- `.appveyor.yml`
- `.cirrus.yml`
- `.travis.yml`
- `INSTALL`
- `Jamfile.v2`
- `Jamroot`
- `appveyor.yml`
- `azure-pipelines.yml`
- `boost-build.jam`
- `boostcpp.jam`
- `bootstrap.bat`
- `bootstrap.jam`
- `bootstrap.sh`
- `build.jam`
- `index.htm`
- `index.html`


## CRLF -> LF normalized files

- `libs/fiber/README.md`
- `libs/mysql/tools/seed_corpus/field_table.csv`
- `libs/mysql/tools/seed_corpus/protocol_messages.csv`


Reported layout: every member below the archive's wrapping directory that is not
listed as pruned is copied byte-for-byte, except the normalized files above.

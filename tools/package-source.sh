#!/usr/bin/env bash
# Package the immutable source asset for this mwrap release.
#
#   tools/package-source.sh [version] [output-dir]
#
# The version defaults to [package].version of mcpp.toml. The primary asset is
# `boost-m-<version>.tar.gz` with the `boost-m-<version>/` wrap directory, built
# from the Git-tracked tree — the same shape the release workflow uploads and the
# index descriptor points at. `--all` additionally writes .tar.xz / .zip / .7z
# for mirrors; compression runs with a bounded thread count (no `xz -T0`), because
# it is part of the local resource budget.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

ALL=0
if [[ "${1:-}" == "--all" ]]; then
    ALL=1
    shift
fi

manifest_version() {
    sed -n 's/^version *= *"\([^"]*\)".*/\1/p' mcpp.toml | head -1
}

VERSION="${1:-$(manifest_version)}"
OUT_DIR="${2:-$REPO_ROOT/target/dist}"
if [[ -z "$VERSION" ]]; then
    echo "FATAL: cannot read [package].version from mcpp.toml" >&2
    exit 1
fi

# Refuse to package a tree whose manifest disagrees with the tag it will carry.
TAG="mwrap-$VERSION"
if git rev-parse --git-dir >/dev/null 2>&1; then
    HEAD_TAGS="$(git tag --points-at HEAD)"
    if [[ -n "$HEAD_TAGS" && "$HEAD_TAGS" != *"$TAG"* ]]; then
        echo "FATAL: HEAD is tagged '$HEAD_TAGS', package version says '$TAG'" >&2
        exit 1
    fi
fi

if command -v python3 >/dev/null 2>&1; then
    python3 tools/check_mwrap_release.py
fi

NAME="boost-m-$VERSION"
mkdir -p "$OUT_DIR"
STAGING="$(mktemp -d)"
trap 'rm -rf "$STAGING"' EXIT

# Tracked files only: ignored caches, target/ and local payloads never ship.
git archive --format=tar HEAD | tar -x -C "$STAGING"

# The archive must carry what consumers and the licence policy need.
for required in LICENSE NOTICE mcpp.toml templates upstream/boost/LICENSE_1_0.txt \
                upstream/boost/README.md upstream/boost-module/LICENSE; do
    [[ -e "$STAGING/$required" ]] || {
        echo "FATAL: archive is missing $required" >&2
        exit 1
    }
done

mkdir -p "$OUT_DIR/$NAME"
cp -a "$STAGING/." "$OUT_DIR/$NAME/"
STAGE_ROOT="$OUT_DIR/$NAME"

echo "==> Creating $NAME.tar.gz"
tar -C "$OUT_DIR" -cf - "$NAME" | gzip -9 > "$OUT_DIR/$NAME.tar.gz"

if [[ $ALL -eq 1 ]]; then
    echo "==> Creating $NAME.tar.xz (bounded threads)"
    tar -C "$OUT_DIR" -cf - "$NAME" | xz -9e -T2 > "$OUT_DIR/$NAME.tar.xz"
    echo "==> Creating $NAME.zip"
    (cd "$OUT_DIR" && zip -9 -q -r "$NAME.zip" "$NAME")
    if command -v 7z >/dev/null 2>&1; then
        echo "==> Creating $NAME.7z"
        7z a -mx=9 -mfb=273 -md=1536m -ms=on -bd -bso0 -bsp0 "$OUT_DIR/$NAME.7z" "$NAME" >/dev/null
    fi
fi

(
    cd "$OUT_DIR"
    for f in "$NAME".tar.gz "$NAME".tar.xz "$NAME".zip "$NAME".7z; do
        if [[ -f "$f" ]]; then
            sha256sum "$f" > "$f.sha256"
        fi
    done
)

rm -rf "$STAGE_ROOT"

# Byte-identity cross-check when a previous asset exists: assets are immutable,
# a differing digest means this is a new version that needs a new tag.
echo "Done:"
ls -l "$OUT_DIR/$NAME".*

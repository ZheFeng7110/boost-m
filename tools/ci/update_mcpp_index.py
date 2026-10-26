#!/usr/bin/env python3
"""Prepend a new version entry to the mcpplibs.mwrap.boost index descriptor.

The descriptor (`pkgs/mwrap/boost.lua` in mcpp-index) keeps one
`["<version>"] = { ... }` block per platform under `xpm.<platform>`. This
script inserts the new version first (newest first) in linux/macosx/windows,
leaving every existing version and all surrounding text untouched.

It is idempotent: running it twice with the same `--key` replaces the block
instead of duplicating it. The key is the package version (`1.92.0`), not the
repository tag; the tag series is `mwrap-<version>` and the archive is
`boost-m-<version>.tar.gz`, so both URLs are checked for that shape before the
descriptor is written — a mirror asset with the wrong name is a broken release,
not a typo to keep.

Usage:
    python3 tools/ci/update_mcpp_index.py \
        --file ../mcpp-index/pkgs/mwrap/boost.lua \
        --key 1.92.0 \
        --global-url https://github.com/mcpplibs/boost-m/releases/download/mwrap-1.92.0/boost-m-1.92.0.tar.gz \
        --cn-url https://gitcode.com/mcpp-res/mwrap-boost/releases/download/1.92.0/boost-m-1.92.0.tar.gz \
        --sha256 <digest>
"""

import argparse
import re
import sys

PLATFORMS = ("linux", "macosx", "windows")


def version_block(key, global_url, cn_url, sha256, indent):
    pad = " " * indent
    inner = " " * (indent + 4)
    url_inner = " " * (indent + 8)
    return (
        f'{pad}["{key}"] = {{\n'
        f'{inner}url = {{\n'
        f'{url_inner}GLOBAL = "{global_url}",\n'
        f'{url_inner}CN     = "{cn_url}",\n'
        f'{inner}}},\n'
        f'{inner}sha256 = "{sha256}",\n'
        f'{pad}}},\n'
    )


def remove_entry(lines, key):
    """Drop every `["key"] = { ... }` block, matching the closing brace by indent.

    A version block's closing `},` sits at the same indentation as its opening
    line; the nested `url = {` closes deeper, so an indent-aware scan never
    mistakes it for the block end.
    """
    header = re.compile(r'^(\s*)\["%s"\] = \{\s*$' % re.escape(key))
    out = []
    i = 0
    while i < len(lines):
        match = header.match(lines[i])
        if not match:
            out.append(lines[i])
            i += 1
            continue
        close = match.group(1) + "},"
        i += 1
        while i < len(lines) and lines[i].rstrip("\n") != close:
            i += 1
        i += 1  # skip the closing `},` line itself
    return out


def insert_after_platform(lines, platform, block):
    pattern = re.compile(r'^\s*%s\s*=\s*\{\s*$' % re.escape(platform))
    for idx, line in enumerate(lines):
        if pattern.match(line):
            lines.insert(idx + 1, block)
            return True
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--file", required=True, help="descriptor to patch in place")
    parser.add_argument("--key", required=True, help="bare xpm version key, no leading v")
    parser.add_argument("--global-url", required=True)
    parser.add_argument("--cn-url", required=True)
    parser.add_argument("--sha256", required=True)
    parser.add_argument("--indent", type=int, default=12,
                        help="indent of the version key lines (default: 12)")
    args = parser.parse_args()

    with open(args.file, "r", encoding="utf-8") as handle:
        lines = handle.readlines()

    tag = "mwrap-" + args.key
    asset = "boost-m-" + args.key + ".tar.gz"
    for label, url in (("GLOBAL", args.global_url), ("CN", args.cn_url)):
        if asset not in url:
            sys.exit(f"error: {label} URL does not name the asset '{asset}': {url}")
    if tag not in args.global_url:
        sys.exit(f"error: GLOBAL URL does not name the immutable tag '{tag}': {args.global_url}")
    if len(args.sha256) != 64 or any(c not in "0123456789abcdef" for c in args.sha256):
        sys.exit(f"error: --sha256 must be a lowercase hex digest: {args.sha256}")

    lines = remove_entry(lines, args.key)
    block = version_block(args.key, args.global_url, args.cn_url,
                          args.sha256, args.indent)
    for platform in PLATFORMS:
        if not insert_after_platform(lines, platform, block):
            sys.exit(f"error: platform '{platform}' not found in {args.file}")

    with open(args.file, "w", encoding="utf-8") as handle:
        handle.writelines(lines)


if __name__ == "__main__":
    main()

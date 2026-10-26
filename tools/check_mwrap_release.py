#!/usr/bin/env python3
"""Check the immutable mwrap release coordinate and the retained documents.

Run in CI before a tag is created (`--tag` omitted, a PR check) and again on the
tag itself (`--tag mwrap-1.92.0`), where a mismatch refuses the release. It
verifies only facts a machine can read: identity, version scheme, licence split,
deployed notices and the bytes of every retained upstream/legacy document.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tomllib


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", help="release tag; omitted during PR checks")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]

    manifest = tomllib.loads((root / "mcpp.toml").read_text(encoding="utf-8"))
    package = manifest["package"]
    assert package["namespace"] == "mcpplibs.mwrap", package
    assert re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", package["name"]), package["name"]
    assert package["name"] == "boost", package["name"]

    upstream = package["metadata"]["mwrap"]["upstream-version"]
    version = package["version"]
    # The first mwrap release is the upstream version itself; a wrapper fix
    # appends .1, .2, ... and never publishes 1.92.0.0 (which sorts equal).
    assert version == upstream or re.fullmatch(
        re.escape(upstream) + r"\.[1-9][0-9]*", version), (version, upstream)
    assert package["metadata"]["mwrap"].get("wrapper-revision", 0) == 0 or \
        version != upstream, "wrapper-revision > 0 requires a wrapper version suffix"
    if args.tag:
        assert args.tag == "mwrap-" + version, (args.tag, version)

    assert package["license"] == "Apache-2.0", package["license"]
    assert package["authors"] == ["mcpp-community"], package.get("authors")
    assert package["repo"] == "https://github.com/mcpplibs/boost-m", package["repo"]

    license_text = (root / "LICENSE").read_text(encoding="utf-8")
    assert "Apache License" in license_text and "Version 2.0" in license_text
    notice_text = (root / "NOTICE").read_text(encoding="utf-8")
    assert "mcpp-community" in notice_text
    assert "Boost Software License" in notice_text

    # Retained documents: upstream Boost's README/licence and the pre-mwrap
    # wrapper's licence/README. Byte-identical to their sources, hashed here so
    # a rewrite is caught before publication.
    documents = json.loads(
        (root / "upstream/documents.json").read_text(encoding="utf-8"))["documents"]
    assert any(Path(d["path"]).name.startswith("README") for d in documents)
    assert any(Path(d["path"]).name.startswith(("LICENSE", "COPYING")) for d in documents)
    assert any(d["path"].startswith("upstream/boost-module/") for d in documents)

    deployed = {entry["from"] for entry in manifest.get("runtime", {}).get("deploy", [])}
    assert {"LICENSE", "NOTICE"} <= deployed, deployed
    for document in documents:
        path = Path(document["path"])
        assert not path.is_absolute() and ".." not in path.parts
        assert path.parts[0] == "upstream", path
        digest = hashlib.sha256((root / path).read_bytes()).hexdigest()
        assert digest == document["sha256"], path
        assert document["source"], path
        assert path.as_posix() in deployed, path

    # The wrapper's own licence and NOTICE land beside the upstream ones but in
    # distinct directories, so equal basenames can never overwrite each other.
    destinations = {entry["to"] for entry in manifest["runtime"]["deploy"]}
    assert "licenses/boost/wrapper" in destinations, destinations
    assert "licenses/boost/upstream/boost" in destinations, destinations
    assert "licenses/boost/upstream/boost-module" in destinations, destinations

    identity = package["namespace"] + "." + package["name"]
    for name, other in (("README.md", "README.zh.md"),
                        ("README.zh.md", "README.md")):
        text = (root / name).read_text(encoding="utf-8")
        assert len(text.splitlines()) <= 150, name
        assert "](" + other + ")" in "\n".join(text.splitlines()[:12]), name
        assert identity in text and version in text, name
        assert text.count("```") % 2 == 0, name

    assert (root / "templates/basic/template.toml").is_file()
    print(identity + "@" + version)


if __name__ == "__main__":
    main()

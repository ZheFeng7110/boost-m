#!/usr/bin/env python3
"""Render this package's templates from a real `mcpp new`, then build and run them.

The candidate version is not in mcpp-index yet, so the test installs an isolated
`MCPP_HOME` fixture: the working tree is placed at the path mcpp keeps an
installed package in, and a descriptor naming the same identity is written into
that home's built-in index. `mcpp new` then runs its real rendering path — no
sed, no hand-substituted placeholders.

After rendering, the generated manifest is parsed with tomllib and checked:
one dependency key that is the complete selector `mcpplibs.mwrap.boost`, the
package's version, and the template's features. The check happens before the
dependency is redirected to the fixture with the same parser, features kept.

Usage:
    python3 tools/test_template.py                # default template
    python3 tools/test_template.py --template lib # a named template
    python3 tools/test_template.py --keep         # leave the fixture on disk
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import tomllib

ROOT = Path(__file__).resolve().parents[1]


def package_identity():
    manifest = tomllib.loads((ROOT / "mcpp.toml").read_text(encoding="utf-8"))
    package = manifest["package"]
    return package["namespace"], package["name"], package["version"]


def donor_home():
    """The MCPP_HOME that already holds the toolchains this test reuses."""
    configured = os.environ.get("MCPP_HOME")
    return Path(configured) if configured else Path.home() / ".mcpp"


def link_or_copy(source, target):
    """Share a donor directory or file; fall back to a copy across filesystems."""
    try:
        target.symlink_to(source, target_is_directory=source.is_dir())
    except OSError:
        if source.is_dir():
            shutil.copytree(source, target)
        else:
            shutil.copy2(source, target)


def install_fixture(home, namespace, name, version):
    """Describe the working tree as an installed package in an isolated home.

    The sandbox pieces a resolved build needs — engine, registry binary, subos
    toolchains, ABI tables, package cache and the index — are linked from the
    donor home, so this test performs no download and no sandbox bootstrap. Only
    the package entry and its descriptor are new; the package directory is a
    symlink to the working tree, so the 400+ MB vendored tree is not copied.
    """
    donor = donor_home()
    home.mkdir(parents=True, exist_ok=True)
    for entry in ("engine", "build-cache", "config.toml"):
        source = donor / entry
        if source.exists() and not (home / entry).exists():
            link_or_copy(source, home / entry)

    registry = home / "registry"
    (registry / "data/xpkgs").mkdir(parents=True, exist_ok=True)
    for entry in ("bin", "subos", "config", "abi", "abi.gen", "abi.json"):
        source = donor / "registry" / entry
        if source.exists() and not (registry / entry).exists():
            link_or_copy(source, registry / entry)
    donor_xpkgs = donor / "registry/data/xpkgs"
    if donor_xpkgs.is_dir():
        for entry in donor_xpkgs.iterdir():
            target = registry / "data/xpkgs" / entry.name
            if not target.exists():
                link_or_copy(entry, target)

    donor_indices = donor / "indices"
    if donor_indices.is_dir():
        shutil.copytree(donor_indices, home / "indices", dirs_exist_ok=True)

    package_dir = (registry / "data/xpkgs" / (namespace + "-x-" + name)
                   / version / (name + "-" + version))
    package_dir.parent.mkdir(parents=True, exist_ok=True)
    package_dir.symlink_to(ROOT, target_is_directory=True)

    index = home / "indices/mcpplibs/pkgs/mwrap"
    index.mkdir(parents=True, exist_ok=True)
    entry = '{url="https://example.invalid/candidate.tar.gz",sha256="fixture"}'
    descriptor = (
        'package={spec="1",namespace=' + json.dumps(namespace) +
        ',name=' + json.dumps(name) +
        ',description="Installed template fixture",type="package",'
        'mcpp="mcpp.toml",xpm={')
    descriptor += ",".join(
        platform + "={[" + json.dumps(version) + "]=" + entry + "}"
        for platform in ("linux", "macosx", "windows")) + "}}\n"
    (index / (namespace + "." + name + ".lua")).write_text(descriptor)
    return package_dir


def check_rendered(manifest_path, identity, version, template_features):
    """Assert the rendered dependency is the complete selector at this version."""
    data = tomllib.loads(manifest_path.read_text(encoding="utf-8"))
    dependencies = data.get("dependencies", {})
    assert list(dependencies) == [identity], (
        "expected exactly one dependency keyed by the complete selector, got {}"
        .format(list(dependencies)))
    options = dependencies[identity]
    assert isinstance(options, dict), (
        "the rendered dependency must be the table form so features survive: {}"
        .format(options))
    assert options.get("version") == version, options
    assert "default-features" in options or "features" in options, options
    if template_features:
        assert options.get("features") == template_features, options
    return options


def redirect_to_fixture(manifest_path, identity, version, options, package_dir):
    """Point the checked dependency at the fixture, preserving explicit features.

    Rewritten through tomllib's parsed values and one precise line replacement:
    the dependency line is the only one that starts with the quoted identity."""
    replacement = "{ path = " + json.dumps(package_dir.as_posix())
    if options.get("features"):
        replacement += ", features = " + json.dumps(options["features"])
    if options.get("default-features") is False:
        replacement += ", default-features = false"
    replacement += " }"
    lines = manifest_path.read_text(encoding="utf-8").splitlines()
    prefix = json.dumps(identity) + " ="
    matches = [i for i, line in enumerate(lines) if line.startswith(prefix)]
    assert len(matches) == 1, matches
    lines[matches[0]] = prefix + " " + replacement
    manifest_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def run(command, cwd, env):
    print("+", " ".join(command), flush=True)
    subprocess.run(command, cwd=cwd, env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mcpp", default="mcpp")
    parser.add_argument("--template", help="named template; omit for the default")
    parser.add_argument("--toolchain", help="pin the toolchain for this test")
    parser.add_argument("--keep", action="store_true",
                        help="keep the fixture and generated project")
    args = parser.parse_args()

    namespace, name, version = package_identity()
    identity = namespace + "." + name

    temporary = Path(tempfile.mkdtemp(prefix=name + "-template-"))
    home = temporary / "home"
    package_dir = install_fixture(home, namespace, name, version)
    env = dict(os.environ, MCPP_HOME=str(home))

    selector = identity + "@" + version
    try:
        run([args.mcpp, "new", "--list-templates", selector],
            temporary, env)
        run([args.mcpp, "new", "quickstart", "--template",
             selector + (":" + args.template if args.template else "")],
            temporary, env)

        project = temporary / "quickstart"
        manifest = project / "mcpp.toml"
        options = check_rendered(manifest, identity, version,
                                 template_features(args.template))
        redirect_to_fixture(manifest, identity, version, options, package_dir)

        build = [args.mcpp, "build", "--strict"]
        if args.toolchain:
            build += ["--toolchain", args.toolchain]
        run(build, project, env)

        if (project / "src/main.cpp").is_file():
            command = [args.mcpp, "run"]
            if args.toolchain:
                command += ["--toolchain", args.toolchain]
            run(command, project, env)
        print("template {}: ok".format(args.template or "default"))
    finally:
        if args.keep:
            print("kept", temporary)
        else:
            shutil.rmtree(temporary, ignore_errors=True)
    return 0


def template_features(template):
    """Features the named template is required to declare in the dependency."""
    features = {
        "basic": ["optional"],
        "lib": ["optional"],
    }
    return features.get(template or "basic", [])


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Set the status or the case of coverage.toml entries in bulk.

After a Boost upgrade, `tools/ci/test_coverage.py scan` adds the new examples
and documentation sections as `todo` and `link` maps those that follow the
naming convention to their cases. What is left is marked here, by substrings
of the entry's path (examples) or doc reference (snippets):

    python3 tools/port_tests/mark_coverage.py asio excluded "needs OpenSSL" /ssl/
    python3 tools/port_tests/mark_coverage.py beast "case=beast/doc:doc: websocket" - websocket.qbk#
    python3 tools/port_tests/mark_coverage.py json todo - parse_into

STATUS is excluded, gap, unsupported or todo (test_coverage.py describes
them); `case=<test>:<case>` maps the entries to a case instead, with REASON
kept as a note unless it is `-`.
"""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "ci"))
import test_coverage as tc  # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n", 1)[0])
    ap.add_argument("lib")
    ap.add_argument("what", help="a status, or case=<test>:<test case>")
    ap.add_argument("reason", help="why (required for a status other than todo); - for none")
    ap.add_argument("keys", nargs="+", help="substrings of the entries' path / doc")
    args = ap.parse_args()
    data = tc.load(args.lib)
    if data is None:
        raise SystemExit("no tests/%s/coverage.toml" % args.lib)
    n = 0
    for table, key in (("example", "path"), ("snippet", "doc")):
        for e in data.get(table, []):
            if not any(k in e[key] for k in args.keys):
                continue
            for k in ("case", "status", "reason"):
                e.pop(k, None)
            if args.what.startswith("case="):
                e["case"] = args.what[len("case="):]
            else:
                e["status"] = args.what
            if args.reason != "-":
                e["reason"] = args.reason
            n += 1
    tc.dump(args.lib, data)
    print(args.lib, "marked", n)


if __name__ == "__main__":
    main()

"""Every using-directive the generator does not follow must be declared in the
policy: `skip-directive` with a reason, or `republish`
(boost::numeric::ublas::detail -> boost::numeric::ublas)."""
import io
import json
import unittest
from unittest import mock

import ast_json
import gen_exports

HDR = "/b/boost/numeric/ublas/matrix.hpp"
NS = "boost::numeric::ublas::detail"


def loc(line=1):
    return {"offset": 1, "line": line, "col": 1, "file": HDR}


def ns(nid, name, inner):
    return {"id": nid, "kind": "NamespaceDecl", "loc": loc(),
            "range": {"begin": loc(), "end": loc()}, "name": name,
            "inner": inner}


def struct(nid, name, line):
    return {"id": nid, "kind": "CXXRecordDecl", "loc": loc(line),
            "name": name, "tagUsed": "struct", "completeDefinition": True}


def directive(nid, target_id, target_name, line):
    return {"id": nid, "kind": "UsingDirectiveDecl", "loc": loc(line),
            "nominatedNamespace": {"id": target_id, "kind": "NamespaceDecl",
                                   "name": target_name}}


def model():
    """namespace boost { namespace numeric { namespace ublas {
         struct matrix;
         namespace detail { using namespace boost::numeric::ublas;
                            struct own; }
         namespace adl_barrier { struct found_by_adl; }
         using namespace adl_barrier;
       } } }"""
    text = json.dumps(ns("0x1", "boost", [
        ns("0x2", "numeric", [
            ns("0x3", "ublas", [
                struct("0x10", "matrix", 2),
                ns("0x4", "detail", [
                    directive("0x20", "0x3", "ublas", 4),
                    struct("0x11", "own", 5),
                ]),
                ns("0x5", "adl_barrier", [
                    struct("0x12", "found_by_adl", 7),
                ]),
                directive("0x21", "0x5", "adl_barrier", 8),
            ]),
        ]),
    ]), indent=2) + "\n"
    return gen_exports.Model(ast_json.TU.from_stream(io.BytesIO(text.encode())))


def policy(**sections):
    return gen_exports.Policy({"numeric": sections})


SKIP = {"namespace": NS, "reason": "resize helpers"}


class SkipDirectiveTests(unittest.TestCase):
    def setUp(self):
        self.home = "numeric"
        patcher = mock.patch.object(gen_exports, "home_of_file",
                                    side_effect=lambda f: self.home)
        patcher.start()
        self.addCleanup(patcher.stop)
        self.m = model()

    def check(self, pol):
        return gen_exports.check_skipped_directives("numeric", self.m, pol)

    def test_undeclared_directive_is_an_error(self):
        entries, errors = self.check(policy())
        self.assertEqual(entries, [])
        # The ADL barrier is followed, so only the detail directive is named.
        self.assertEqual(len(errors), 1)
        for part in ("using namespace boost::numeric::ublas;", NS,
                     "[[numeric.skip-directive]]", "[[numeric.republish]]"):
            self.assertIn(part, errors[0])

    def test_declared_directive_is_listed_with_its_reason(self):
        entries, errors = self.check(policy(**{"skip-directive": [SKIP]}))
        self.assertEqual(errors, [])
        self.assertEqual(entries, [("numeric", "skip-directive", NS, "",
                                    "resize helpers")])

    def test_republish_declares_the_directive(self):
        pol = policy(republish=[{"namespace": NS, "reason": "public"}])
        self.assertEqual(self.check(pol), ([], []))

    def test_stale_entry_is_an_error(self):
        gone = dict(SKIP, namespace="boost::numeric::ublas::gone")
        errors = self.check(policy(**{"skip-directive": [SKIP, gone]}))[1]
        self.assertEqual(len(errors), 1)
        self.assertIn("boost::numeric::ublas::gone", errors[0])

    def test_adl_barrier_cannot_be_declared(self):
        barrier = dict(SKIP, namespace="boost::numeric::ublas")
        errors = self.check(policy(**{"skip-directive": [SKIP, barrier]}))[1]
        self.assertEqual(len(errors), 1)
        self.assertIn("policy skip-directive boost::numeric::ublas:",
                      errors[0])

    def test_entry_that_is_also_a_republish_is_an_error(self):
        pol = policy(**{"skip-directive": [SKIP],
                        "republish": [{"namespace": NS, "reason": "public"}]})
        errors = self.check(pol)[1]
        self.assertEqual(len(errors), 1)
        self.assertIn("also a republish", errors[0])

    def test_directive_in_another_library_header_is_its_owner_business(self):
        self.home = "other"
        self.assertEqual(self.check(policy()), ([], []))
        errors = self.check(policy(**{"skip-directive": [SKIP]}))[1]
        self.assertEqual(len(errors), 1)
        self.assertIn("library that owns its header", errors[0])

    def test_skipped_directive_republishes_nothing(self):
        out = gen_exports.collect_injections("numeric", self.m, {})
        self.assertIn("boost::numeric::ublas::found_by_adl", out)
        self.assertFalse([q for q in out if q.startswith(NS + "::")])


if __name__ == "__main__":
    unittest.main()

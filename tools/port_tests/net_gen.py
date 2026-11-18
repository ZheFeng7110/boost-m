"""Shared driver of the loopback example ports (asio, beast, cobalt, mqtt5).

A Gen collects examples of one output file with their per-example options and
runs port_examples.py once for all of them. Servers run in a background thread
on a free port (`serve`), clients are ported together with the server they
belong with (`with_`); the harness of helpers/asio.hpp provides the ports, the
probes and the harness clients.
"""

import common as c


def serve_main(ns, args):
    """Setup code starting the ported server `ns` in the background."""
    return ("ex_serve_async([p = ex_port] { (void)ex_call_main(&%s::example_main, "
            "std::vector<std::string>{%s}); }, ex_port);" % (ns, ", ".join(['"server"'] + args)))


def ns(rel):
    """The case namespace of an example (port_examples.py --case-from-rel)."""
    return "ex_" + rel.replace("/", "_").replace("-", "_")


class Gen:
    def __init__(self, out, header, lib="asio", modules="asio", pre="", includes=(), helpers=("asio.hpp",),
                 oracle_includes=("boost/asio.hpp",), oracle_args=()):
        self.out, self.header, self.lib, self.modules, self.pre, self.includes = out, header, lib, modules, pre, includes
        self.helpers, self.oracle_includes, self.oracle_args = helpers, oracle_includes, oracle_args
        self.files, self.opts = [], []

    def add(self, rel, **kw):
        """An example, by its path under libs/<lib>/example/ without .cpp."""
        self.files.append(rel)
        k = rel
        if "setup" in kw:
            self.opts += ["--setup", "%s=%s" % (k, kw["setup"])]
        if "argv" in kw:
            self.opts += ["--argv", "%s=%s" % (k, ",,".join(kw["argv"]))]
        if "serve" in kw:
            self.opts += ["--serve", "%s=%s" % (k, kw["serve"])]
        if "stdin" in kw:
            self.opts += ["--stdin", "%s=%s" % (k, kw["stdin"])]
        for old, new in kw.get("re", []):
            self.opts += ["--post-re", "%s:%s=>%s" % (k, old, new)]
        for old, new in kw.get("sub", []):
            self.opts += ["--post-sub", "%s:%s=>%s" % (k, old, new)]
        if "with_" in kw:
            self.opts += ["--with", "%s=%s" % (k, ",".join(kw["with_"]))]
        if "merge" in kw:
            self.opts += ["--merge", "%s=%s" % (k, ",".join(kw["merge"]))]
        if kw.get("nocheck"):
            self.opts += ["--output-unless", "%s=%s" % (k, kw["nocheck"])]
        if "only_if" in kw:
            self.opts += ["--only-if", "%s=%s" % (k, kw["only_if"])]
        if "oracle" in kw:
            self.opts += ["--oracle-for", "%s=%s" % (k, kw["oracle"])]

    def run(self):
        stem = self.out.rsplit("/", 1)[-1].rsplit(".", 1)[0]
        helpers = c.write_helpers(stem + "_helpers.hpp", [c.helper(h) for h in self.helpers])
        cmd = ["--lib", self.lib, "--modules", self.modules,
               "--case-from-rel", "--inline-local", "--extra-helpers", helpers,
               "--add-include", "boost/asio/detail/config.hpp", "--add-include", "boost/asio/coroutine.hpp",
               "--oracle-arg=-lpthread"] + ["--oracle-arg=" + a for a in self.oracle_args]
        for h in ("thread", "functional", "random") + tuple(self.oracle_includes):
            cmd += ["--oracle-arg=-include", "--oracle-arg=" + h]
        cmd += ["--out", self.out, "--header", self.header + self.pre] + self.opts
        for h in self.includes:
            cmd += ["--add-include", h]
        cmd += [str(c.lib_dir(self.lib) / "example" / (f + ".cpp")) for f in self.files]
        print("==", self.out, len(self.files))
        return c.port(cmd)

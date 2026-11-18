"""Boost.Beast: the HTTP and WebSocket examples of libs/beast/example/, over
loopback in one process (examples_http.cpp, examples_websocket.cpp)."""
import common as c
from net_gen import Gen, serve_main, ns

GROUPS = ["http", "websocket"]

HEADER = """// mcpplibs.mwrap.boost.beast — the official {what} examples
// (libs/beast/example/), run over the loopback interface in one process. A
// server example runs in a background thread on a free port (ex_serve_async)
// and a harness client below — itself written with Beast — sends one request
// (HTTP GET of a document root's index.html, or one WebSocket message) and the
// case output is what it received. A client example runs against the ported
// server example it belongs with, started the same way. The expected output
// is what the same code printed when built from upstream/boost headers alone.
// Response headers that carry the build (Server: Boost.Beast/<version>) or the
// time are left out by the harness.
"""
HELPERS = ("asio.hpp", "beast.hpp")
ORACLE = ("boost/asio.hpp", "boost/beast.hpp")
SYS = (c.oracle_lib("system_extras"),)  # error_category -> std::error_category

# BOOST_BEAST_VERSION_STRING; BOOST_ASIO_HANDLER_TYPE (echo-op); BOOST_THROW_EXCEPTION (fast)
MACROS = ("boost/beast/version.hpp", "boost/asio/async_result.hpp", "boost/throw_exception.hpp")
FREE = "ex_port = ex_free_port();"
DOC = "static const std::string ex_root = ex_doc_root();"
LOCAL = 'ex_port = ex_local_path("beast.sock");'
GET = 'ex_http_get(ex_port, "/index.html")'
WS = 'ex_ws_exchange(ex_port, "hello beast")'
ADDR = ['"127.0.0.1"', "ex_port"]
NOLOCAL = "!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)"
POSIX = "!defined(BOOST_ASIO_WINDOWS)"  # Windows' AF_UNIX: bind fails on the CI hosts


def http():
    g = Gen("tests/beast/examples_http.cpp", HEADER.format(what="HTTP"), lib="beast", modules="asio,beast",
            helpers=HELPERS, oracle_includes=ORACLE, oracle_args=SYS, includes=MACROS)
    for s, extra in [("sync/http_server_sync", []), ("async/http_server_async", ['"1"']),
                     ("stackless/http_server_stackless", ['"1"']), ("awaitable/http_server_awaitable", ['"1"'])]:
        g.add("http/server/" + s, setup=FREE + " " + DOC, argv=ADDR + ["ex_root"] + extra, serve=GET)
    # fast: a worker serves one connection at a time and the listening probe stays
    # open, so two workers
    g.add("http/server/fast/http_server_fast", setup=FREE + " " + DOC, argv=ADDR + ["ex_root", '"2"', '"block"'], serve=GET)
    # small: a fixed page per request, no document root
    g.add("http/server/small/http_server_small", setup=FREE, argv=ADDR, serve='ex_http_get(ex_port, "/count")')
    g.add("http/server/async-local/http_server_async_local", setup=LOCAL + " " + DOC, argv=["ex_port", "ex_root", '"1"'],
          serve='ex_http_get_local(ex_port, "/index.html")', nocheck=NOLOCAL, only_if=POSIX)
    srv = "http/server/sync/http_server_sync"
    for client in ["sync/http_client_sync", "async/http_client_async", "awaitable/http_client_awaitable"]:
        g.add("http/client/" + client, with_=[srv],
              setup=FREE + " " + DOC + " " + serve_main(ns(srv), ['"127.0.0.1"', "p", "ex_root"]),
              argv=ADDR + ['"/index.html"', '"1.1"'], nocheck="1")  # prints the Date and Server headers
    srv = "http/server/async-local/http_server_async_local"
    g.add("http/client/async-local/http_client_async_local", with_=[srv],
          setup=LOCAL + " " + DOC + " " + serve_main(ns(srv), ["p", "ex_root", '"1"']),
          argv=["ex_port", '"localhost"', '"80"', '"/index.html"', '"1.1"'], nocheck="1", only_if=POSIX)
    return g.run()


def websocket():
    g = Gen("tests/beast/examples_websocket.cpp", HEADER.format(what="WebSocket"), lib="beast", modules="asio,beast",
            helpers=HELPERS, oracle_includes=ORACLE, oracle_args=SYS, includes=MACROS)
    g.add("websocket/server/sync/websocket_server_sync", setup=FREE, argv=ADDR, serve=WS)
    for s in ["async/websocket_server_async", "stackless/websocket_server_stackless",
              "awaitable/websocket_server_awaitable"]:
        g.add("websocket/server/" + s, setup=FREE, argv=ADDR + ['"1"'], serve=WS)
    g.add("websocket/server/async-local/websocket_server_async_local", setup=LOCAL, argv=["ex_port", '"1"'],
          serve='ex_ws_exchange_local(ex_port, "hello beast")', nocheck=NOLOCAL, only_if=POSIX)
    srv = "websocket/server/sync/websocket_server_sync"
    for client in ["sync/websocket_client_sync", "async/websocket_client_async", "awaitable/websocket_client_awaitable"]:
        g.add("websocket/client/" + client, with_=[srv], setup=FREE + " " + serve_main(ns(srv), ['"127.0.0.1"', "p"]),
              argv=ADDR + ['"hello beast"'])
    srv = "websocket/server/async-local/websocket_server_async_local"
    g.add("websocket/client/async-local/websocket_client_async_local", with_=[srv],
          setup=LOCAL + " " + serve_main(ns(srv), ["p", '"1"']), argv=["ex_port", '"localhost"', '"80"', '"hello beast"'],
          nocheck=NOLOCAL, only_if=POSIX)
    g.add("websocket/server/chat-multi/main", setup=FREE + " " + DOC, argv=ADDR + ["ex_root", '"1"'], serve=WS,
          merge=["http_session.cpp", "listener.cpp", "shared_state.cpp", "websocket_session.cpp"])
    g.add("advanced/server/advanced_server", setup=FREE + " " + DOC, argv=ADDR + ["ex_root", '"1"'],
          serve=GET + " + " + WS)
    # echo-op accepts one connection, echoes one line and exits: not probed
    g.add("echo-op/echo_op", setup=FREE + " ex_no_probe = true;", argv=ADDR,
          serve='ex_tcp_exchange(ex_port, "hello op\\n", 0)')
    return g.run()


def run(group):
    return {"http": http, "websocket": websocket}[group]()

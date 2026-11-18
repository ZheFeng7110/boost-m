"""Boost.MQTT5: libs/mqtt5/example/ against an in-process broker
(examples.cpp; also writes tests/mqtt5/broker.hpp)."""
import common as c
from net_gen import Gen

HEADER = """// mcpplibs.mwrap.boost.mqtt5 — the official examples (libs/mqtt5/example/),
// ported mechanically against an in-process MQTT 5 broker (the harness below)
// instead of broker.hivemq.com: each case starts the broker on a free port and
// passes it as argv (host, port, client id). The output must equal what the
// same code printed when built from upstream/boost headers alone. Changes:
// ec.message() prints no error as "Success" (ex_message; the C library's text
// differs between platforms); the programs that run until SIGINT stop as on
// SIGINT (their signal_set is cancelled) after their first message
// (receiver, publisher) or their fifth completion (multiflight_client). The TLS examples need OpenSSL and are excluded.
"""
ARGV = ['"127.0.0.1"', "ex_port", '"boost_mqtt5_tester"']
BROKER = "ex_port = ex_free_port(); ex_mqtt_broker(ex_port);"
MSG = [(r"\bec\.message\(\)", "ex_message(ec)")]


def run(group):
    g = Gen("tests/mqtt5/examples.cpp", HEADER, lib="mqtt5", modules="asio,beast,mqtt5",
            helpers=("asio.hpp", "mqtt5.hpp"),
            oracle_includes=("boost/asio.hpp", "boost/beast.hpp", "map", "mutex"),
            oracle_args=(c.oracle_lib("system_extras"),))  # error_category -> std::error_category
    for s in ["hello_world_over_tcp", "hello_world_in_multithreaded_env", "hello_world_in_coro_multithreaded_env",
              "timeout_with_awaitable_operators", "timeout_with_parallel_group"]:
        g.add(s, setup=BROKER, argv=ARGV, re=MSG)
    g.add("hello_world_over_websocket_tcp", setup="ex_port = ex_free_port(); ex_mqtt_broker(ex_port, true);",
          argv=['"127.0.0.1/mqtt"', "ex_port", '"boost_mqtt5_tester"'], re=MSG)
    SIG = [(r"(boost::asio::signal_set signals\(ioc, SIGINT, SIGTERM\);)", "\\1 ex_signals = &signals;")]
    g.add("multiflight_client", setup=BROKER, argv=ARGV, re=MSG + SIG,
          sub=[('ex_out << "\\t rc: " << rc.message() << std::endl;',
                'ex_out << "\\t rc: " << rc.message() << std::endl;\n                if (i == 5) ex_signals->cancel();  // boost-m: as on SIGINT')])
    g.add("receiver", setup='ex_port = ex_free_port(); ex_mqtt_broker(ex_port, false, {{"test", "a retained message"}});',
          argv=ARGV, re=MSG + SIG, sub=[(
        'ex_out << "\\t payload: " << payload << std::endl;',
        'ex_out << "\\t payload: " << payload << std::endl;\n        ex_signals->cancel();  // boost-m: as on SIGINT')])
    g.add("publisher", setup=BROKER, argv=ARGV, re=MSG + SIG, nocheck="1",  # the reading is random
          sub=[("timer.expires_after(std::chrono::seconds(5));",
                "ex_signals->cancel();  // boost-m: as on SIGINT\n        timer.expires_after(std::chrono::seconds(5));")])
    rc = g.run()

    # The broker for the hand-written files (doc.cpp, custom_types.cpp).
    (c.TESTS / "mqtt5/broker.hpp").write_text(
        "// boost-m tests: the in-process MQTT 5 broker of tests/mqtt5/examples.cpp\n"
        "// (written by the generator from the same helper code), for doc.cpp and\n"
        "// custom_types.cpp. Include after the imports.\n"
        "#pragma once\n"
        "inline std::string ex_free_port() {\n"
        "    boost::asio::io_context c;\n"
        "    boost::asio::ip::tcp::acceptor a(c, {boost::asio::ip::make_address(\"127.0.0.1\"), 0});\n"
        "    return std::to_string(a.local_endpoint().port());\n"
        "}\n" + (c.HELPERS / "mqtt5.hpp").read_text())
    return rc

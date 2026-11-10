// mcpplibs.mwrap.boost.mqtt5 — the documentation's code (libs/mqtt5/doc/qbk/):
// the introduction's first program, choosing the transport, the QoS 1
// publish of the auto-reconnect page, disconnecting and restarting the
// client, per-operation cancellation and executors. "<your-mqtt-broker>" is
// the in-process broker of broker.hpp on a free port; the pages' std::cout
// output goes to a string the cases compare.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.asio;
import mcpplibs.mwrap.boost.beast;
import mcpplibs.mwrap.boost.mqtt5;
#include "broker.hpp"

namespace {
std::string broker() {
    auto port = ex_free_port();
    ex_mqtt_broker(port);
    return port;
}
std::uint16_t port_of(const std::string& p) { return static_cast<std::uint16_t>(std::stoi(p)); }
// The text of no error is the C library's; the pages print ec.message().
std::string msg(boost::mqtt5::error_code ec) { return ec ? ec.message() : "Success"; }
}  // namespace

TEST_CASE("doc: intro") {
    auto port = broker();
    std::string out;
    boost::asio::io_context ioc;
    boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> c(ioc);
    c.brokers("127.0.0.1", port_of(port))
        .credentials("<your-client-id>", "<client-username>", "<client-pwd>")
        .async_run(boost::asio::detached);
    c.async_publish<boost::mqtt5::qos_e::at_most_once>(
        "<topic>", "Hello world!", boost::mqtt5::retain_e::no, boost::mqtt5::publish_props{},
        [&c, &out](boost::mqtt5::error_code ec) {
            out += msg(ec) + "\n";
            c.async_disconnect(boost::asio::detached);
        });
    ioc.run();
    CHECK(out == "Success\n");
}

TEST_CASE("doc: transport_protocol") {
    boost::asio::io_context ioc;
    boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(ioc);
    static_assert(std::is_same_v<decltype(client)::executor_type, boost::asio::any_io_executor>);
    // WebSocket over TCP is the other stream the package can build (TLS needs OpenSSL)
    using ws_client = boost::mqtt5::mqtt_client<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>;
    ws_client wsc(ioc);
    CHECK(client.get_executor() == wsc.get_executor());
}

TEST_CASE("doc: auto_reconnect") {
    // The client is started before the broker listens: the publish is buffered
    // and sent after the reconnect, and its handler runs once, with the PUBACK.
    auto port = ex_free_port();
    boost::asio::io_context ioc;
    boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(ioc);
    client.brokers("127.0.0.1", port_of(port)).async_run(boost::asio::detached);
    int calls = 0;
    boost::mqtt5::reason_code result;
    client.async_publish<boost::mqtt5::qos_e::at_least_once>(
        "my-topic", "Hello world!", boost::mqtt5::retain_e::no, boost::mqtt5::publish_props{},
        [&](boost::mqtt5::error_code ec, boost::mqtt5::reason_code rc, boost::mqtt5::puback_props) {
            ++calls;
            CHECK_FALSE(ec);
            result = rc;
            client.async_disconnect(boost::asio::detached);
        });
    boost::asio::steady_timer later(ioc, std::chrono::milliseconds(300));
    later.async_wait([&](boost::mqtt5::error_code) { ex_mqtt_broker(port); });
    ioc.run();
    CHECK(calls == 1);
    CHECK_FALSE(result);  // success
}

TEST_CASE("doc: disconnecting_the_client") {
    auto port = broker();
    std::string out;
    // async_disconnect right after the publish: the publish may not be sent
    {
        boost::asio::io_context ioc;
        boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(ioc);
        client.brokers("127.0.0.1", port_of(port)).async_run(boost::asio::detached);
        client.async_publish<boost::mqtt5::qos_e::at_most_once>(
            "<topic>", "Hello world!", boost::mqtt5::retain_e::no, boost::mqtt5::publish_props{},
            [&out](boost::mqtt5::error_code ec) { out += (ec ? "aborted" : "sent") + std::string("\n"); });
        client.async_disconnect(boost::asio::detached);
        ioc.run();
    }
    CHECK((out == "aborted\n" || out == "sent\n"));
    out.clear();
    // in the handler: the publish completes first
    {
        boost::asio::io_context ioc;
        boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(ioc);
        client.brokers("127.0.0.1", port_of(port)).async_run(boost::asio::detached);
        client.async_publish<boost::mqtt5::qos_e::at_most_once>(
            "<topic>", "Hello world!", boost::mqtt5::retain_e::no, boost::mqtt5::publish_props{},
            [&client, &out](boost::mqtt5::error_code ec) {
                out += msg(ec) + "\n";
                client.async_disconnect(boost::asio::detached);
            });
        ioc.run();
    }
    CHECK(out == "Success\n");
}

TEST_CASE("doc: reusing_the_client") {
    auto port = broker();
    boost::asio::io_context ioc;
    boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(ioc);
    client.brokers("127.0.0.1", port_of(port)).async_run(boost::asio::detached);
    client.async_disconnect(boost::asio::detached);
    // reconfigured and restarted
    client.connect_property(boost::mqtt5::prop::session_expiry_interval, 120)
        .keep_alive(30)
        .async_run(boost::asio::detached);
    boost::mqtt5::error_code result = boost::asio::error::would_block;
    client.async_publish<boost::mqtt5::qos_e::at_least_once>(
        "<topic>", "again", boost::mqtt5::retain_e::no, boost::mqtt5::publish_props{},
        [&](boost::mqtt5::error_code ec, boost::mqtt5::reason_code, boost::mqtt5::puback_props) {
            result = ec;
            client.async_disconnect(boost::asio::detached);
        });
    ioc.run();
    CHECK_FALSE(result);
}

TEST_CASE("doc: per_op_cancellation") {
    // No broker: the publish waits, and the terminal cancellation completes it.
    auto port = ex_free_port();
    boost::asio::io_context ioc;
    boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(ioc);
    client.brokers("127.0.0.1", port_of(port)).async_run(boost::asio::detached);
    boost::asio::cancellation_signal signal;
    std::string out;
    client.async_publish<boost::mqtt5::qos_e::at_least_once>(
        "<topic>", "Hello world!", boost::mqtt5::retain_e::no, boost::mqtt5::publish_props{},
        boost::asio::bind_cancellation_slot(
            signal.slot(),
            [&client, &out](boost::mqtt5::error_code ec, boost::mqtt5::reason_code, boost::mqtt5::puback_props) {
                out = ec == boost::asio::error::operation_aborted ? "aborted" : msg(ec);
                client.cancel();
            }));
    signal.emit(boost::asio::cancellation_type_t::terminal);
    ioc.run();
    CHECK(out == "aborted");
}

TEST_CASE("doc: executors") {
    auto port = broker();
    boost::asio::io_context ioc;
    auto strand = boost::asio::make_strand(ioc.get_executor());
    boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(strand);
    client.brokers("127.0.0.1", port_of(port)).async_run(boost::asio::detached);
    bool on_strand = false;
    client.async_publish<boost::mqtt5::qos_e::at_most_once>(
        "<topic>", "Hello world!", boost::mqtt5::retain_e::no, boost::mqtt5::publish_props{},
        [&client, &strand, &on_strand](boost::mqtt5::error_code) {
            on_strand = strand.running_in_this_thread();
            client.cancel();
        });
    ioc.run();
    CHECK(on_strand);
}

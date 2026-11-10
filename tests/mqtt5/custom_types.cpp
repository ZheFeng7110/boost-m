// mcpplibs.mwrap.boost.mqtt5 — user types at the client's customization
// points: a user stream layer over a TCP socket (next_layer, the read/write
// operations, and the async_shutdown overload the client finds by ADL), a
// user logger (the at_* hooks logger_traits detects), and a user
// authenticator for enhanced authentication (async_auth and method), all
// against the in-process broker of broker.hpp; the traits rejecting types
// without the hooks.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.asio;
import mcpplibs.mwrap.boost.beast;
import mcpplibs.mwrap.boost.mqtt5;
#include "broker.hpp"

namespace asio = boost::asio;
namespace mqtt5 = boost::mqtt5;
using error_code = boost::system::error_code;

namespace user {
inline std::atomic<std::size_t> bytes_read{0}, bytes_written{0}, shutdowns{0};

// A stream layer counting the octets that cross it; the client connects its
// lowest layer (the socket) and reads and writes through it.
class counting_stream {
    asio::ip::tcp::socket next_;

public:
    using next_layer_type = asio::ip::tcp::socket;
    using executor_type = next_layer_type::executor_type;
    explicit counting_stream(const executor_type& ex) : next_(ex) {}
    next_layer_type& next_layer() { return next_; }
    executor_type get_executor() { return next_.get_executor(); }
    template <class MutableBuffers, class Token>
    auto async_read_some(const MutableBuffers& b, Token&& token) {
        return next_.async_read_some(b, asio::deferred([](error_code ec, std::size_t n) {
            bytes_read += n;
            return asio::deferred.values(ec, n);
        }))(std::forward<Token>(token));
    }
    template <class ConstBuffers, class Token>
    auto async_write_some(const ConstBuffers& b, Token&& token) {
        return next_.async_write_some(b, asio::deferred([](error_code ec, std::size_t n) {
            bytes_written += n;
            return asio::deferred.values(ec, n);
        }))(std::forward<Token>(token));
    }
};
// The client shuts a stream down through async_shutdown, found by ADL.
template <class Handler>
void async_shutdown(counting_stream& s, Handler&& handler) {
    ++shutdowns;
    error_code ec;
    s.next_layer().shutdown(asio::socket_base::shutdown_both, ec);
    std::move(handler)(ec);
}

// A logger recording the events it is told about.
struct recording_logger {
    std::shared_ptr<std::vector<std::string>> events = std::make_shared<std::vector<std::string>>();
    void at_resolve(error_code ec, std::string_view host, std::string_view, const asio::ip::tcp::resolver::results_type&) {
        events->push_back("resolve " + std::string(host) + (ec ? " failed" : ""));
    }
    void at_tcp_connect(error_code ec, asio::ip::tcp::endpoint) {
        events->push_back(ec ? "connect failed" : "connect");
    }
    void at_connack(mqtt5::reason_code rc, bool session_present, const mqtt5::connack_props&) {
        events->push_back(std::string("connack ") + (rc ? "error" : "ok") + (session_present ? " session" : ""));
    }
    void at_disconnect(mqtt5::reason_code, const mqtt5::disconnect_props&) { events->push_back("disconnect"); }
};
struct silent_logger {};

// Enhanced authentication: the steps the client asks for.
struct recording_authenticator {
    std::shared_ptr<std::vector<std::string>> steps = std::make_shared<std::vector<std::string>>();
    void async_auth(mqtt5::auth_step_e step, std::string data, mqtt5::detail::auth_handler_type handler) {
        steps->push_back(std::to_string(static_cast<int>(step)) + ":" + data);
        std::move(handler)(error_code{}, step == mqtt5::auth_step_e::client_initial ? "client-hello" : "");
    }
    std::string_view method() const { return "user-method"; }
};
struct not_an_authenticator {
    std::string_view method() const { return "m"; }
};
}  // namespace user

namespace {
std::uint16_t port_of(const std::string& p) { return static_cast<std::uint16_t>(std::stoi(p)); }
}

TEST_CASE("custom: stream layer") {
    static_assert(mqtt5::detail::has_next_layer<user::counting_stream>);
    static_assert(std::is_same_v<mqtt5::detail::lowest_layer_type<user::counting_stream>, asio::ip::tcp::socket>);
    static_assert(!mqtt5::detail::has_tls_layer<user::counting_stream>);
    auto port = ex_free_port();
    ex_mqtt_broker(port);
    user::bytes_read = user::bytes_written = user::shutdowns = 0;
    asio::io_context ioc;
    mqtt5::mqtt_client<user::counting_stream> client(ioc);
    client.brokers("127.0.0.1", port_of(port)).async_run(asio::detached);
    error_code result = asio::error::would_block;
    client.async_publish<mqtt5::qos_e::at_least_once>(
        "user/stream", "through the user layer", mqtt5::retain_e::no, mqtt5::publish_props{},
        [&](error_code ec, mqtt5::reason_code, mqtt5::puback_props) {
            result = ec;
            client.async_disconnect(asio::detached);
        });
    ioc.run();
    CHECK_FALSE(result);
    CHECK(user::bytes_written > std::size_t(22));  // CONNECT, PUBLISH (its payload), DISCONNECT
    CHECK(user::bytes_read >= std::size_t(8 + 4));  // CONNACK, PUBACK
    CHECK(user::shutdowns >= 1);
}

TEST_CASE("custom: logger") {
    static_assert(mqtt5::has_at_connack<user::recording_logger>);
    static_assert(mqtt5::has_at_resolve<user::recording_logger>);
    static_assert(!mqtt5::has_at_connack<user::silent_logger>);
    static_assert(!mqtt5::has_at_transport_error<user::recording_logger>);
    auto port = ex_free_port();
    ex_mqtt_broker(port);
    asio::io_context ioc;
    user::recording_logger logger;
    mqtt5::mqtt_client<asio::ip::tcp::socket, std::monostate, user::recording_logger> client(ioc, {}, logger);
    client.brokers("127.0.0.1", port_of(port)).async_run(asio::detached);
    client.async_publish<mqtt5::qos_e::at_most_once>("user/log", "x", mqtt5::retain_e::no, mqtt5::publish_props{},
                                                    [&](error_code) { client.async_disconnect(asio::detached); });
    ioc.run();
    REQUIRE(logger.events->size() >= 3);
    CHECK((*logger.events)[0] == "resolve 127.0.0.1");
    CHECK((*logger.events)[1] == "connect");
    CHECK((*logger.events)[2] == "connack ok");
}

TEST_CASE("custom: authenticator") {
    static_assert(mqtt5::detail::is_authenticator<user::recording_authenticator>);
    static_assert(!mqtt5::detail::is_authenticator<user::not_an_authenticator>);
    auto port = ex_free_port();
    ex_mqtt_broker(port);
    asio::io_context ioc;
    user::recording_authenticator auth;
    mqtt5::mqtt_client<asio::ip::tcp::socket> client(ioc);
    client.brokers("127.0.0.1", port_of(port)).authenticator(auth).async_run(asio::detached);
    error_code result = asio::error::would_block;
    client.async_publish<mqtt5::qos_e::at_least_once>(
        "user/auth", "authenticated", mqtt5::retain_e::no, mqtt5::publish_props{},
        [&](error_code ec, mqtt5::reason_code, mqtt5::puback_props) {
            result = ec;
            client.async_disconnect(asio::detached);
        });
    ioc.run();
    CHECK_FALSE(result);
    // the initial data for the CONNECT, then the server's final step (the
    // broker accepted the method in its CONNACK)
    REQUIRE(auth.steps->size() == 2);
    CHECK((*auth.steps)[0] == std::to_string(static_cast<int>(mqtt5::auth_step_e::client_initial)) + ":");
    CHECK((*auth.steps)[1] == std::to_string(static_cast<int>(mqtt5::auth_step_e::server_final)) + ":");
}

// mcpplibs.mwrap.boost.asio — the official cpp11 examples (libs/asio/example/cpp11/)
// that run without a network peer, ported mechanically: each program runs as a
// case and its output must equal what the same code printed when built from
// upstream/boost headers alone. The operations/ examples accept three
// connections on a fixed port: here the port is a free one and a background
// peer (the loopback harness below) connects and drains, so they run in one
// process; their 1 s delay between repeated messages is 10 ms here (the
// output does not depend on it). Outputs not compared: parallel_sort (completion order), the
// parallel_group examples that write to the process's stdout/stderr
// descriptors (bypassing the captured stream, completion order varies) and
// priority_scheduler (std::priority_queue does not order equal priorities).
// boost/asio/yield.hpp / unyield.hpp (the stackless-coroutine macros) stay
// where the examples include them; BOOST_ASIO_COMPLETION_TOKEN_FOR(sig) is
// written as the completion_token_for<sig> concept it expands to; boost/asio/coroutine.hpp is included first
// so those in-place includes add only the macros. The deferred examples print
// ec.message() through ex_message(), which spells no error "Success" (the
// C library's own text for it differs between platforms); programs that need
// a POSIX stream_descriptor or socketpair are compiled only where asio has them.
// The actor benchmark passes its token 500 thousand times, not 50 million.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/coroutine.hpp>
#include <cassert>
#include <boost/config.hpp>
import std;
import mcpplibs.mwrap.boost.asio;
import mcpplibs.mwrap.boost.thread;

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\n';
    return 0;
}

// ---- loopback harness (boost-m tests): free ports, waiting for a listener,
// and a background peer that drains or echoes, so server-side examples run in
// one process. The fixed ports of the examples (55555, ...) are replaced by
// ex_port_number(). ----
// UDP servers cannot be probed: ex_free_udp_port() marks the case so the
// harness waits briefly instead (and the UDP clients below retry).
inline bool ex_udp = false;
inline bool ex_local = false;  // ex_port is a local (UNIX domain) socket path
inline bool ex_no_probe = false;  // the server accepts one connection only: do not probe it
inline std::string ex_port;
inline unsigned short ex_port_number() { return static_cast<unsigned short>(std::stoi(ex_port)); }
inline std::string ex_free_port() {
    ex_udp = ex_local = ex_no_probe = false;
    boost::asio::io_context c;
    boost::asio::ip::tcp::acceptor a(c, {boost::asio::ip::make_address("127.0.0.1"), 0});
    return std::to_string(a.local_endpoint().port());
}
inline bool ex_try_connect(boost::asio::ip::tcp::socket& s, const std::string& port) {
    boost::system::error_code ec;
    s.connect({boost::asio::ip::make_address("127.0.0.1"), static_cast<unsigned short>(std::stoi(port))}, ec);
    return !ec;
}
// The probe connection stays open (a server would report it closing).
inline void ex_wait_listening(const std::string& port) {
    auto& c = *new boost::asio::io_context;
    for (int i = 0; i < 500; ++i) {
        auto* s = new boost::asio::ip::tcp::socket(c);
        if (ex_try_connect(*s, port)) return;
        delete s;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
// Connects `n` times (retrying while the example is not listening yet),
// optionally sends `send`, and reads until the server closes.
inline void ex_peer_async(std::string port, int n, std::string send = {}) {
    std::thread([port, n, send] {
        for (int k = 0; k < n; ++k) {
            boost::asio::io_context c;
            boost::asio::ip::tcp::socket s(c);
            int tries = 0;
            while (!ex_try_connect(s, port)) {
                if (++tries > 300) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                s = boost::asio::ip::tcp::socket(c);
            }
            boost::system::error_code ec;
            if (!send.empty()) boost::asio::write(s, boost::asio::buffer(send), ec);
            char buf[4096];
            while (!ec) s.read_some(boost::asio::buffer(buf), ec);
        }
    }).detach();
}
inline std::string ex_free_udp_port() {
    boost::asio::io_context c;
    boost::asio::ip::udp::socket u(c, {boost::asio::ip::make_address("127.0.0.1"), 0});
    ex_udp = true;
    ex_local = false;
    return std::to_string(u.local_endpoint().port());
}
inline bool ex_try_connect_local(const std::string& path) {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
    boost::asio::io_context c;
    boost::asio::local::stream_protocol::socket s(c);
    boost::system::error_code ec;
    s.connect(boost::asio::local::stream_protocol::endpoint(path), ec);
    return !ec;
#else
    (void)path;
    return false;
#endif
}
// Runs a server example in a detached thread (it serves until the process
// exits) and waits until it accepts connections.
inline void ex_serve_async(std::function<void()> server, std::string port) {
    std::thread(std::move(server)).detach();
    if (ex_udp || ex_no_probe) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } else if (ex_local) {
        for (int i = 0; i < 500 && !ex_try_connect_local(port); ++i)  // (a probe that closes: the local servers do not report it)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } else {
        ex_wait_listening(port);
    }
}
// Calls a ported main with its own argv (kept alive: a server runs on).
template <class F>
int ex_call_main(F f, std::vector<std::string> a) {
    if constexpr (std::is_invocable_v<F>) {
        return f();
    } else {
        auto* store = new std::vector<std::string>(std::move(a));
        auto* ptrs = new std::vector<char*>;
        for (auto& x : *store) ptrs->push_back(x.data());
        ptrs->push_back(nullptr);
        return f(int(store->size()), ex_argv_t{ptrs->data()});
    }
}
// ---- harness clients: each prints what it received and returns 0 ----
inline std::string ex_shape(const std::string& got, bool show) {
    if (show) return got;
    return got.empty() ? "(nothing)\n" : (got.back() == '\n' ? "a line\n" : "text\n");
}
// TCP: sends `send`, then reads `n` bytes (n == 0: until the server closes).
// The connection stays open: a server seeing the client leave would report
// it (from its own thread, racing with this output), and some end on it.
inline int ex_tcp_exchange(const std::string& port, const std::string& send, std::size_t n, bool show = true) {
    auto& c = *new boost::asio::io_context;  // kept: see above
    auto& s = *new boost::asio::ip::tcp::socket(c);
    for (int i = 0; !ex_try_connect(s, port); ++i) {  // (an unprobed server may not listen yet)
        if (i == 300) { ex_out << "connect failed\n"; return 1; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        s = boost::asio::ip::tcp::socket(c);
    }
    boost::system::error_code ec;
    if (!send.empty()) boost::asio::write(s, boost::asio::buffer(send), ec);
    std::string got;
    if (n) {
        got.resize(n);
        got.resize(boost::asio::read(s, boost::asio::buffer(got), ec));
    } else {
        char buf[1024];
        while (!ec) got.append(buf, s.read_some(boost::asio::buffer(buf), ec));
    }
    ex_out << "received: " << ex_shape(got, show);
    return 0;
}
// UDP: sends `send` (retrying while there is no reply) and prints the reply.
inline int ex_udp_exchange(const std::string& port, const std::string& send, bool show = true) {
    boost::asio::io_context c;
    boost::asio::ip::udp::socket u(c, boost::asio::ip::udp::v4());
    boost::asio::ip::udp::endpoint to(boost::asio::ip::make_address("127.0.0.1"),
                                      static_cast<unsigned short>(std::stoi(port)));
    u.non_blocking(true);
    char buf[1024];
    for (int i = 0; i < 300; ++i) {
        boost::system::error_code ec;
        if (i % 20 == 0) u.send_to(boost::asio::buffer(send), to, 0, ec);
        boost::asio::ip::udp::endpoint from;
        std::size_t k = u.receive_from(boost::asio::buffer(buf), from, 0, ec);
        if (!ec) {
            ex_out << "received: " << ex_shape(std::string(buf, k), show);
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ex_out << "no reply\n";
    return 1;
}
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
inline int ex_local_exchange(const std::string& path, const std::string& send, std::size_t n, bool show = true) {
    auto& c = *new boost::asio::io_context;  // kept open, as in ex_tcp_exchange
    auto& s = *new boost::asio::local::stream_protocol::socket(c);
    boost::system::error_code ec;
    s.connect(boost::asio::local::stream_protocol::endpoint(path), ec);
    if (ec) { ex_out << "connect failed\n"; return 1; }
    boost::asio::write(s, boost::asio::buffer(send), ec);
    std::string got(n, '\0');
    got.resize(boost::asio::read(s, boost::asio::buffer(got), ec));
    ex_out << "received: " << ex_shape(got, show);
    return 0;
}
#else
// No local sockets: the examples print that and return; nothing to talk to
// (their output is not compared there).
inline int ex_local_exchange(const std::string&, const std::string&, std::size_t, bool = true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
#endif
// A fresh path in the temporary directory (removed first).
inline std::string ex_temp_path(const std::string& name) {
    auto p = std::filesystem::temp_directory_path() / ("boost-m-asio-" + std::to_string(std::random_device{}()) + "-" + name);
    std::filesystem::remove_all(p);
    return p.string();
}
inline std::string ex_local_path(const std::string& name) {
    ex_udp = false;
    ex_local = true;
    return ex_temp_path(name);
}
// An HTTP document root holding index.html.
inline std::string ex_doc_root() {
    auto d = ex_temp_path("www");
    std::filesystem::create_directories(d);
    std::ofstream(std::filesystem::path(d) / "index.html") << "<html><body>boost-m</body></html>\n";
    return d;
}
// A background TCP echo server of the harness (the upstream of a proxy).
inline std::string ex_harness_echo() {
    auto* c = new boost::asio::io_context;
    auto* a = new boost::asio::ip::tcp::acceptor(*c, {boost::asio::ip::make_address("127.0.0.1"), 0});
    std::string port = std::to_string(a->local_endpoint().port());
    std::thread([a] {
        for (;;) {
            boost::asio::ip::tcp::socket s(a->get_executor());
            a->accept(s);
            std::thread([s = std::move(s)]() mutable {
                char buf[1024];
                boost::system::error_code ec;
                for (;;) {
                    std::size_t k = s.read_some(boost::asio::buffer(buf), ec);
                    if (ec) return;
                    boost::asio::write(s, boost::asio::buffer(buf, k), ec);
                }
            }).detach();
        }
    }).detach();
    return port;
}
// An error_code's message: "Success" for no error. The text of no error is
// the C library's (glibc "Success", musl "No error information", macOS
// "Undefined error: 0", Windows "The operation completed successfully").
inline std::string ex_message(const boost::system::error_code& ec) {
    return ec ? ec.message() : std::string("Success");
}
#if defined(BOOST_ASIO_HAS_PIPE) && !defined(BOOST_ASIO_WINDOWS)
// A descriptor reading `text` then EOF: stands in for the process's stdin
// (STDIN_FILENO) where an example reads it through a stream_descriptor.
inline int ex_stdin_fd(const std::string& text) {
    boost::asio::io_context c;
    boost::asio::readable_pipe r(c);
    boost::asio::writable_pipe w(c);
    boost::asio::connect_pipe(r, w);
    boost::asio::write(w, boost::asio::buffer(text));
    w.close();
    return r.release();
}
#endif

// libs/asio/example/cpp11/deferred/deferred_1.cpp
namespace ex_cpp11_deferred_deferred_1 {
using boost::asio::deferred;

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  auto deferred_op = timer.async_wait(deferred);

  std::move(deferred_op)(
      [](boost::system::error_code ec)
      {
        ex_out << "timer wait finished: " << ex_message(ec) << "\n";
      }
    );

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/deferred/deferred_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(timer wait finished: Success
)ex"));
}
}  // namespace ex_cpp11_deferred_deferred_1

// libs/asio/example/cpp11/deferred/deferred_2.cpp
namespace ex_cpp11_deferred_deferred_2 {
using boost::asio::deferred;

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  auto deferred_op = timer.async_wait(
      deferred(
        [&](boost::system::error_code ec)
        {
          ex_out << "first timer wait finished: " << ex_message(ec) << "\n";
          timer.expires_after(std::chrono::seconds(1));
          return timer.async_wait(deferred);
        }
      )
    );

  std::move(deferred_op)(
      [](boost::system::error_code ec)
      {
        ex_out << "second timer wait finished: " << ex_message(ec) << "\n";
      }
    );

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/deferred/deferred_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(first timer wait finished: Success
second timer wait finished: Success
)ex"));
}
}  // namespace ex_cpp11_deferred_deferred_2

// libs/asio/example/cpp11/executors/actor.cpp
namespace ex_cpp11_executors_actor {
using boost::asio::any_io_executor;
using boost::asio::defer;
using boost::asio::post;
using boost::asio::strand;
using boost::asio::system_executor;

//------------------------------------------------------------------------------
// A tiny actor framework
// ~~~~~~~~~~~~~~~~~~~~~~

class actor;

// Used to identify the sender and recipient of messages.
typedef actor* actor_address;

// Base class for all registered message handlers.
class message_handler_base
{
public:
  virtual ~message_handler_base() {}

  // Used to determine which message handlers receive an incoming message.
  virtual const std::type_info& message_id() const = 0;
};

// Base class for a handler for a specific message type.
template <class Message>
class message_handler : public message_handler_base
{
public:
  // Handle an incoming message.
  virtual void handle_message(Message msg, actor_address from) = 0;
};

// Concrete message handler for a specific message type.
template <class Actor, class Message>
class mf_message_handler : public message_handler<Message>
{
public:
  // Construct a message handler to invoke the specified member function.
  mf_message_handler(void (Actor::* mf)(Message, actor_address), Actor* a)
    : function_(mf), actor_(a)
  {
  }

  // Used to determine which message handlers receive an incoming message.
  virtual const std::type_info& message_id() const
  {
    return typeid(Message);
  }

  // Handle an incoming message.
  virtual void handle_message(Message msg, actor_address from)
  {
    (actor_->*function_)(std::move(msg), from);
  }

  // Determine whether the message handler represents the specified function.
  bool is_function(void (Actor::* mf)(Message, actor_address)) const
  {
    return mf == function_;
  }

private:
  void (Actor::* function_)(Message, actor_address);
  Actor* actor_;
};

// Base class for all actors.
class actor
{
public:
  virtual ~actor()
  {
  }

  // Obtain the actor's address for use as a message sender or recipient.
  actor_address address()
  {
    return this;
  }

  // Send a message from one actor to another.
  template <class Message>
  friend void send(Message msg, actor_address from, actor_address to)
  {
    // Execute the message handler in the context of the target's executor.
    post(to->executor_,
      [=]
      {
        to->call_handler(std::move(msg), from);
      });
  }

protected:
  // Construct the actor to use the specified executor for all message handlers.
  actor(any_io_executor e)
    : executor_(std::move(e))
  {
  }

  // Register a handler for a specific message type. Duplicates are permitted.
  template <class Actor, class Message>
  void register_handler(void (Actor::* mf)(Message, actor_address))
  {
    handlers_.push_back(
      std::make_shared<mf_message_handler<Actor, Message>>(
        mf, static_cast<Actor*>(this)));
  }

  // Deregister a handler. Removes only the first matching handler.
  template <class Actor, class Message>
  void deregister_handler(void (Actor::* mf)(Message, actor_address))
  {
    const std::type_info& id = typeid(Message);
    for (auto iter = handlers_.begin(); iter != handlers_.end(); ++iter)
    {
      if ((*iter)->message_id() == id)
      {
        auto mh = static_cast<mf_message_handler<Actor, Message>*>(iter->get());
        if (mh->is_function(mf))
        {
          handlers_.erase(iter);
          return;
        }
      }
    }
  }

  // Send a message from within a message handler.
  template <class Message>
  void tail_send(Message msg, actor_address to)
  {
    // Execute the message handler in the context of the target's executor.
    actor* from = this;
    defer(to->executor_,
      [=]
      {
        to->call_handler(std::move(msg), from);
      });
  }

private:
  // Find the matching message handlers, if any, and call them.
  template <class Message>
  void call_handler(Message msg, actor_address from)
  {
    const std::type_info& message_id = typeid(Message);
    for (auto& h: handlers_)
    {
      if (h->message_id() == message_id)
      {
        auto mh = static_cast<message_handler<Message>*>(h.get());
        mh->handle_message(msg, from);
      }
    }
  }

  // All messages associated with a single actor object should be processed
  // non-concurrently. We use a strand to ensure non-concurrent execution even
  // if the underlying executor may use multiple threads.
  strand<any_io_executor> executor_;

  std::vector<std::shared_ptr<message_handler_base>> handlers_;
};

// A concrete actor that allows synchronous message retrieval.
template <class Message>
class receiver : public actor
{
public:
  receiver()
    : actor(system_executor())
  {
    register_handler(&receiver::message_handler);
  }

  // Block until a message has been received.
  Message wait()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this]{ return !message_queue_.empty(); });
    Message msg(std::move(message_queue_.front()));
    message_queue_.pop_front();
    return msg;
  }

private:
  // Handle a new message by adding it to the queue and waking a waiter.
  void message_handler(Message msg, actor_address /* from */)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    message_queue_.push_back(std::move(msg));
    condition_.notify_one();
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<Message> message_queue_;
};

//------------------------------------------------------------------------------


using boost::asio::thread_pool;

class member : public actor
{
public:
  explicit member(any_io_executor e)
    : actor(std::move(e))
  {
    register_handler(&member::init_handler);
  }

private:
  void init_handler(actor_address next, actor_address from)
  {
    next_ = next;
    caller_ = from;

    register_handler(&member::token_handler);
    deregister_handler(&member::init_handler);
  }

  void token_handler(int token, actor_address /*from*/)
  {
    int msg(token);
    actor_address to(caller_);

    if (token > 0)
    {
      msg = token - 1;
      to = next_;
    }

    tail_send(msg, to);
  }

  actor_address next_;
  actor_address caller_;
};

int example_main()
{
  const std::size_t num_threads = 16;
  const int num_hops = 500000;
  const std::size_t num_actors = 503;
  const int token_value = (num_hops + num_actors - 1) / num_actors;
  const std::size_t actors_per_thread = num_actors / num_threads;

  struct single_thread_pool : thread_pool { single_thread_pool() : thread_pool(1) {} };
  single_thread_pool pools[num_threads];
  std::vector<std::shared_ptr<member>> members(num_actors);
  receiver<int> rcvr;

  // Create the member actors.
  for (std::size_t i = 0; i < num_actors; ++i)
    members[i] = std::make_shared<member>(pools[(i / actors_per_thread) % num_threads].get_executor());

  // Initialise the actors by passing each one the address of the next actor in the ring.
  for (std::size_t i = num_actors, next_i = 0; i > 0; next_i = --i)
    send(members[next_i]->address(), rcvr.address(), members[i - 1]->address());

  // Send exactly one token to each actor, all with the same initial value, rounding up if required.
  for (std::size_t i = 0; i < num_actors; ++i)
    send(token_value, rcvr.address(), members[i]->address());

  // Wait for all signal messages, indicating the tokens have all reached zero.
  for (std::size_t i = 0; i < num_actors; ++i)
    rcvr.wait();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/executors/actor") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_cpp11_executors_actor

// libs/asio/example/cpp11/executors/bank_account_1.cpp
namespace ex_cpp11_executors_bank_account_1 {
using boost::asio::static_thread_pool;
namespace execution = boost::asio::execution;

// Traditional active object pattern.
// Member functions do not block.

class bank_account
{
  int balance_ = 0;
  mutable static_thread_pool pool_{1};

public:
  void deposit(int amount)
  {
    pool_.executor().execute(
        [this, amount]
        {
          balance_ += amount;
        });
  }

  void withdraw(int amount)
  {
    pool_.executor().execute(
        [this, amount]
        {
          if (balance_ >= amount)
            balance_ -= amount;
        });
  }

  void print_balance() const
  {
    pool_.executor().execute(
        [this]
        {
          ex_out << "balance = " << balance_ << "\n";
        });
  }

  ~bank_account()
  {
    pool_.wait();
  }
};

int example_main()
{
  bank_account acct;
  acct.deposit(20);
  acct.withdraw(10);
  acct.print_balance();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/executors/bank_account_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(balance = 10
)ex"));
}
}  // namespace ex_cpp11_executors_bank_account_1

// libs/asio/example/cpp11/executors/bank_account_2.cpp
namespace ex_cpp11_executors_bank_account_2 {
using boost::asio::static_thread_pool;
namespace execution = boost::asio::execution;

// Traditional active object pattern.
// Member functions block until operation is finished.

class bank_account
{
  int balance_ = 0;
  mutable static_thread_pool pool_{1};

public:
  void deposit(int amount)
  {
    boost::asio::require(pool_.executor(), execution::blocking.always).execute(
        [this, amount]
        {
          balance_ += amount;
        });
  }

  void withdraw(int amount)
  {
    boost::asio::require(pool_.executor(), execution::blocking.always).execute(
        [this, amount]
        {
          if (balance_ >= amount)
            balance_ -= amount;
        });
  }

  int balance() const
  {
    int result = 0;
    boost::asio::require(pool_.executor(), execution::blocking.always).execute(
        [this, &result]
        {
          result = balance_;
        });
    return result;
  }
};

int example_main()
{
  bank_account acct;
  acct.deposit(20);
  acct.withdraw(10);
  ex_out << "balance = " << acct.balance() << "\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/executors/bank_account_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(balance = 10
)ex"));
}
}  // namespace ex_cpp11_executors_bank_account_2

// libs/asio/example/cpp11/executors/fork_join.cpp
namespace ex_cpp11_executors_fork_join {
using boost::asio::static_thread_pool;
namespace execution = boost::asio::execution;

// A fixed-size thread pool used to implement fork/join semantics. Functions
// are scheduled using a simple FIFO queue. Implementing work stealing, or
// using a queue based on atomic operations, are left as tasks for the reader.
class fork_join_pool
{
public:
  // The constructor starts a thread pool with the specified number of threads.
  // Note that the thread_count is not a fixed limit on the pool's concurrency.
  // Additional threads may temporarily be added to the pool if they join a
  // fork_executor.
  explicit fork_join_pool(
      std::size_t thread_count = std::max(std::thread::hardware_concurrency(), 1u) * 2)
    : use_count_(1),
      threads_(thread_count)
  {
    try
    {
      // Ask each thread in the pool to dequeue and execute functions until
      // it is time to shut down, i.e. the use count is zero.
      for (thread_count_ = 0; thread_count_ < thread_count; ++thread_count_)
      {
        threads_.executor().execute(
            [this]
            {
              std::unique_lock<std::mutex> lock(mutex_);
              while (use_count_ > 0)
                if (!execute_next(lock))
                  condition_.wait(lock);
            });
      }
    }
    catch (...)
    {
      stop_threads();
      threads_.wait();
      throw;
    }
  }

  // The destructor waits for the pool to finish executing functions.
  ~fork_join_pool()
  {
    stop_threads();
    threads_.wait();
  }

private:
  friend class fork_executor;

  // The base for all functions that are queued in the pool.
  struct function_base
  {
    std::shared_ptr<std::size_t> work_count_;
    void (*execute_)(std::shared_ptr<function_base>& p);
  };

  // Execute the next function from the queue, if any. Returns true if a
  // function was executed, and false if the queue was empty.
  bool execute_next(std::unique_lock<std::mutex>& lock)
  {
    if (queue_.empty())
      return false;
    auto p(queue_.front());
    queue_.pop();
    lock.unlock();
    execute(lock, p);
    return true;
  }

  // Execute a function and decrement the outstanding work.
  void execute(std::unique_lock<std::mutex>& lock,
      std::shared_ptr<function_base>& p)
  {
    std::shared_ptr<std::size_t> work_count(std::move(p->work_count_));
    try
    {
      p->execute_(p);
      lock.lock();
      do_work_finished(work_count);
    }
    catch (...)
    {
      lock.lock();
      do_work_finished(work_count);
      throw;
    }
  }

  // Increment outstanding work.
  void do_work_started(const std::shared_ptr<std::size_t>& work_count) noexcept
  {
    if (++(*work_count) == 1)
      ++use_count_;
  }

  // Decrement outstanding work. Notify waiting threads if we run out.
  void do_work_finished(const std::shared_ptr<std::size_t>& work_count) noexcept
  {
    if (--(*work_count) == 0)
    {
      --use_count_;
      condition_.notify_all();
    }
  }

  // Dispatch a function, executing it immediately if the queue is already
  // loaded. Otherwise adds the function to the queue and wakes a thread.
  void do_execute(std::shared_ptr<function_base> p,
      const std::shared_ptr<std::size_t>& work_count)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() > thread_count_ * 16)
    {
      do_work_started(work_count);
      lock.unlock();
      execute(lock, p);
    }
    else
    {
      queue_.push(p);
      do_work_started(work_count);
      condition_.notify_one();
    }
  }

  // Ask all threads to shut down.
  void stop_threads()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    --use_count_;
    condition_.notify_all();
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::queue<std::shared_ptr<function_base>> queue_;
  std::size_t use_count_;
  std::size_t thread_count_;
  static_thread_pool threads_;
};

// A class that satisfies the Executor requirements. Every function or piece of
// work associated with a fork_executor is part of a single, joinable group.
class fork_executor
{
public:
  fork_executor(fork_join_pool& ctx)
    : context_(ctx),
      work_count_(std::make_shared<std::size_t>(0))
  {
  }

  fork_join_pool& query(execution::context_t) const noexcept
  {
    return context_;
  }

  template <class Func>
  void execute(Func f) const
  {
    auto p(std::make_shared<function<Func>>(std::move(f), work_count_));
    context_.do_execute(p, work_count_);
  }

  friend bool operator==(const fork_executor& a,
      const fork_executor& b) noexcept
  {
    return a.work_count_ == b.work_count_;
  }

  friend bool operator!=(const fork_executor& a,
      const fork_executor& b) noexcept
  {
    return a.work_count_ != b.work_count_;
  }

  // Block until all work associated with the executor is complete. While it is
  // waiting, the thread may be borrowed to execute functions from the queue.
  void join() const
  {
    std::unique_lock<std::mutex> lock(context_.mutex_);
    while (*work_count_ > 0)
      if (!context_.execute_next(lock))
        context_.condition_.wait(lock);
  }

private:
  template <class Func>
  struct function : fork_join_pool::function_base
  {
    explicit function(Func f, const std::shared_ptr<std::size_t>& w)
      : function_(std::move(f))
    {
      work_count_ = w;
      execute_ = [](std::shared_ptr<fork_join_pool::function_base>& p)
      {
        Func tmp(std::move(static_cast<function*>(p.get())->function_));
        p.reset();
        tmp();
      };
    }

    Func function_;
  };

  fork_join_pool& context_;
  std::shared_ptr<std::size_t> work_count_;
};

// Helper class to automatically join a fork_executor when exiting a scope.
class join_guard
{
public:
  explicit join_guard(const fork_executor& ex) : ex_(ex) {}
  join_guard(const join_guard&) = delete;
  join_guard(join_guard&&) = delete;
  ~join_guard() { ex_.join(); }

private:
  fork_executor ex_;
};

//------------------------------------------------------------------------------


fork_join_pool pool;

template <class Iterator>
void fork_join_sort(Iterator begin, Iterator end)
{
  std::size_t n = end - begin;
  if (n > 32768)
  {
    {
      fork_executor fork(pool);
      join_guard join(fork);
      fork.execute([=]{ fork_join_sort(begin, begin + n / 2); });
      fork.execute([=]{ fork_join_sort(begin + n / 2, end); });
    }
    std::inplace_merge(begin, begin + n / 2, end);
  }
  else
  {
    std::sort(begin, end);
  }
}

int example_main(int argc, char* argv[])
{
  if (argc != 2)
  {
    ex_out << "Usage: fork_join <size>\n";
    return 1;
  }

  std::vector<double> vec(std::atoll(argv[1]));
  std::iota(vec.begin(), vec.end(), 0);

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(vec.begin(), vec.end(), g);

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

  fork_join_sort(vec.begin(), vec.end());

  std::chrono::steady_clock::duration elapsed = std::chrono::steady_clock::now() - start;

  ex_out << "sort took ";
  ex_out << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
  ex_out << " microseconds" << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/executors/fork_join") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_args.push_back("1000");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(sort took 30 microseconds
)ex"));
#endif
}
}  // namespace ex_cpp11_executors_fork_join

// libs/asio/example/cpp11/executors/pipeline.cpp
namespace ex_cpp11_executors_pipeline {
using boost::asio::execution_context;
using boost::asio::executor_binder;
using boost::asio::get_associated_executor;
using boost::asio::post;
using boost::asio::system_executor;
using boost::asio::use_future;
using boost::asio::use_service;
namespace execution = boost::asio::execution;

// An executor that launches a new thread for each function submitted to it.
// This class satisfies the executor requirements.
class thread_executor
{
private:
  // Service to track all threads started through a thread_executor.
  class thread_bag : public execution_context::service
  {
  public:
    typedef thread_bag key_type;

    explicit thread_bag(execution_context& ctx)
      : execution_context::service(ctx)
    {
    }

    void add_thread(std::thread&& t)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      threads_.push_back(std::move(t));
    }

  private:
    virtual void shutdown()
    {
      for (auto& t : threads_)
        t.join();
    }

    std::mutex mutex_;
    std::vector<std::thread> threads_;
  };

public:
  execution_context& query(execution::context_t) const
  {
    return boost::asio::query(system_executor(), execution::context);
  }

  execution::blocking_t query(execution::blocking_t) const
  {
    return execution::blocking.never;
  }

  thread_executor require(execution::blocking_t::never_t) const
  {
    return *this;
  }

  template <class Func>
  void execute(Func f) const
  {
    thread_bag& bag = use_service<thread_bag>(query(execution::context));
    bag.add_thread(std::thread(std::move(f)));
  }

  friend bool operator==(const thread_executor&,
      const thread_executor&) noexcept
  {
    return true;
  }

  friend bool operator!=(const thread_executor&,
      const thread_executor&) noexcept
  {
    return false;
  }
};

// Base class for all thread-safe queue implementations.
class queue_impl_base
{
  template <class> friend class queue_front;
  template <class> friend class queue_back;
  std::mutex mutex_;
  std::condition_variable condition_;
  bool stop_ = false;
};

// Underlying implementation of a thread-safe queue, shared between the
// queue_front and queue_back classes.
template <class T>
class queue_impl : public queue_impl_base
{
  template <class> friend class queue_front;
  template <class> friend class queue_back;
  std::queue<T> queue_;
};

// The front end of a queue between consecutive pipeline stages.
template <class T>
class queue_front
{
public:
  typedef T value_type;

  explicit queue_front(std::shared_ptr<queue_impl<T>> impl)
    : impl_(impl)
  {
  }

  void push(T t)
  {
    std::unique_lock<std::mutex> lock(impl_->mutex_);
    impl_->queue_.push(std::move(t));
    impl_->condition_.notify_one();
  }

  void stop()
  {
    std::unique_lock<std::mutex> lock(impl_->mutex_);
    impl_->stop_ = true;
    impl_->condition_.notify_one();
  }

private:
  std::shared_ptr<queue_impl<T>> impl_;
};

// The back end of a queue between consecutive pipeline stages.
template <class T>
class queue_back
{
public:
  typedef T value_type;

  explicit queue_back(std::shared_ptr<queue_impl<T>> impl)
    : impl_(impl)
  {
  }

  bool pop(T& t)
  {
    std::unique_lock<std::mutex> lock(impl_->mutex_);
    while (impl_->queue_.empty() && !impl_->stop_)
      impl_->condition_.wait(lock);
    if (!impl_->queue_.empty())
    {
      t = impl_->queue_.front();
      impl_->queue_.pop();
      return true;
    }
    return false;
  }

private:
  std::shared_ptr<queue_impl<T>> impl_;
};

// Launch the last stage in a pipeline.
template <class T, class F>
std::future<void> pipeline(queue_back<T> in, F f)
{
  // Get the function's associated executor, defaulting to thread_executor.
  auto ex = get_associated_executor(f, thread_executor());

  // Run the function, and as we're the last stage return a future so that the
  // caller can wait for the pipeline to finish.
  return post(ex, use_future([in, f]() mutable { f(in); }));
}

// Launch an intermediate stage in a pipeline.
template <class T, class F, class... Tail>
std::future<void> pipeline(queue_back<T> in, F f, Tail... t)
{
  // Determine the output queue type.
  typedef typename executor_binder<F, thread_executor>::second_argument_type::value_type output_value_type;

  // Create the output queue and its implementation.
  auto out_impl = std::make_shared<queue_impl<output_value_type>>();
  queue_front<output_value_type> out(out_impl);
  queue_back<output_value_type> next_in(out_impl);

  // Get the function's associated executor, defaulting to thread_executor.
  auto ex = get_associated_executor(f, thread_executor());

  // Run the function.
  post(ex, [in, out, f]() mutable
      {
        f(in, out);
        out.stop();
      });

  // Launch the rest of the pipeline.
  return pipeline(next_in, std::move(t)...);
}

// Launch the first stage in a pipeline.
template <class F, class... Tail>
std::future<void> pipeline(F f, Tail... t)
{
  // Determine the output queue type.
  typedef typename executor_binder<F, thread_executor>::argument_type::value_type output_value_type;

  // Create the output queue and its implementation.
  auto out_impl = std::make_shared<queue_impl<output_value_type>>();
  queue_front<output_value_type> out(out_impl);
  queue_back<output_value_type> next_in(out_impl);

  // Get the function's associated executor, defaulting to thread_executor.
  auto ex = get_associated_executor(f, thread_executor());

  // Run the function.
  post(ex, [out, f]() mutable
      {
        f(out);
        out.stop();
      });

  // Launch the rest of the pipeline.
  return pipeline(next_in, std::move(t)...);
}

//------------------------------------------------------------------------------


using boost::asio::bind_executor;
using boost::asio::thread_pool;

void reader(queue_front<std::string> out)
{
  std::string line;
  while (std::getline(ex_in, line))
    out.push(line);
}

void filter(queue_back<std::string> in, queue_front<std::string> out)
{
  std::string line;
  while (in.pop(line))
    if (line.length() > 5)
      out.push(line);
}

void upper(queue_back<std::string> in, queue_front<std::string> out)
{
  std::string line;
  while (in.pop(line))
  {
    std::string new_line;
    for (char c : line)
      new_line.push_back(std::toupper(c));
    out.push(new_line);
  }
}

void writer(queue_back<std::string> in)
{
  std::size_t count = 0;
  std::string line;
  while (in.pop(line))
    ex_out << count++ << ": " << line << std::endl;
}

int example_main()
{
  thread_pool pool(1);

  auto f = pipeline(reader, filter, bind_executor(pool, upper), writer);
  f.wait();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/executors/pipeline") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(the quick brown fox
jumps over the lazy dog
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(0: THE QUICK BROWN FOX
1: JUMPS OVER THE LAZY DOG
)ex"));
}
}  // namespace ex_cpp11_executors_pipeline

// libs/asio/example/cpp11/executors/priority_scheduler.cpp
namespace ex_cpp11_executors_priority_scheduler {
using boost::asio::dispatch;
using boost::asio::execution_context;
namespace execution = boost::asio::execution;

class priority_scheduler : public execution_context
{
public:
  // A class that satisfies the Executor requirements.
  class executor_type
  {
  public:
    executor_type(priority_scheduler& ctx, int pri) noexcept
      : context_(ctx), priority_(pri)
    {
    }

    priority_scheduler& query(execution::context_t) const noexcept
    {
      return context_;
    }

    template <class Func>
    void execute(Func f) const
    {
      auto p(std::make_shared<item<Func>>(priority_, std::move(f)));
      std::lock_guard<std::mutex> lock(context_.mutex_);
      context_.queue_.push(p);
      context_.condition_.notify_one();
    }

    friend bool operator==(const executor_type& a,
        const executor_type& b) noexcept
    {
      return &a.context_ == &b.context_;
    }

    friend bool operator!=(const executor_type& a,
        const executor_type& b) noexcept
    {
      return &a.context_ != &b.context_;
    }

  private:
    priority_scheduler& context_;
    int priority_;
  };

  ~priority_scheduler() noexcept
  {
    shutdown();
    destroy();
  }

  executor_type get_executor(int pri = 0) noexcept
  {
    return executor_type(*const_cast<priority_scheduler*>(this), pri);
  }

  void run()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;)
    {
      condition_.wait(lock, [&]{ return stopped_ || !queue_.empty(); });
      if (stopped_)
        return;
      auto p(queue_.top());
      queue_.pop();
      lock.unlock();
      p->execute_(p);
      lock.lock();
    }
  }

  void stop()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    condition_.notify_all();
  }

private:
  struct item_base
  {
    int priority_;
    void (*execute_)(std::shared_ptr<item_base>&);
  };

  template <class Func>
  struct item : item_base
  {
    item(int pri, Func f) : function_(std::move(f))
    {
      priority_ = pri;
      execute_ = [](std::shared_ptr<item_base>& p)
      {
        Func tmp(std::move(static_cast<item*>(p.get())->function_));
        p.reset();
        tmp();
      };
    }

    Func function_;
  };

  struct item_comp
  {
    bool operator()(
        const std::shared_ptr<item_base>& a,
        const std::shared_ptr<item_base>& b)
    {
      return a->priority_ < b->priority_;
    }
  };

  std::mutex mutex_;
  std::condition_variable condition_;
  std::priority_queue<
    std::shared_ptr<item_base>,
    std::vector<std::shared_ptr<item_base>>,
    item_comp> queue_;
  bool stopped_ = false;
};

int example_main()
{
  priority_scheduler sched;
  auto low = sched.get_executor(0);
  auto med = sched.get_executor(1);
  auto high = sched.get_executor(2);
  dispatch(low, []{ ex_out << "1\n"; });
  dispatch(low, []{ ex_out << "11\n"; });
  dispatch(med, []{ ex_out << "2\n"; });
  dispatch(med, []{ ex_out << "22\n"; });
  dispatch(high, []{ ex_out << "3\n"; });
  dispatch(high, []{ ex_out << "33\n"; });
  dispatch(high, []{ ex_out << "333\n"; });
  dispatch(sched.get_executor(-1), [&]{ sched.stop(); });
  sched.run();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/executors/priority_scheduler") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(3
33
333
2
22
11
1
)ex"));
#endif
}
}  // namespace ex_cpp11_executors_priority_scheduler

// libs/asio/example/cpp11/files/async_file_copy.cpp
namespace ex_cpp11_files_async_file_copy {
#if defined(BOOST_ASIO_HAS_FILE)

class file_copier
{
public:
  file_copier(boost::asio::io_context& io_context,
      const char* from, const char* to)
    : from_file_(io_context, from,
        boost::asio::stream_file::read_only),
      to_file_(io_context, to,
        boost::asio::stream_file::write_only
          | boost::asio::stream_file::create
          | boost::asio::stream_file::truncate)
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    from_file_.async_read_some(boost::asio::buffer(data_),
        [this](boost::system::error_code error, std::size_t n)
        {
          if (!error)
          {
            do_write(n);
          }
          else if (error != boost::asio::error::eof)
          {
            ex_out << "Error copying file: " << error.message() << "\n";
          }
        });
  }

  void do_write(std::size_t n)
  {
    boost::asio::async_write(to_file_, boost::asio::buffer(data_, n),
        [this](boost::system::error_code error, std::size_t /*n*/)
        {
          if (!error)
          {
            do_read();
          }
          else
          {
            ex_out << "Error copying file: " << error.message() << "\n";
          }
        });
  }

  boost::asio::stream_file from_file_;
  boost::asio::stream_file to_file_;
  char data_[4096];
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: async_file_copy <from> <to>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    file_copier copier(io_context, argv[1], argv[2]);
    copier.start();

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_FILE)
int example_main(int, char*[]) {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_FILE)

TEST_CASE("example: cpp11/files/async_file_copy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    static const std::string ex_src = ex_temp_path("copy-src"); std::ofstream(ex_src) << "file contents\n";
    ex_args.push_back(ex_src);
    ex_args.push_back(ex_temp_path("copy-dst"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_cpp11_files_async_file_copy

// libs/asio/example/cpp11/files/blocking_file_copy.cpp
namespace ex_cpp11_files_blocking_file_copy {
#if defined(BOOST_ASIO_HAS_FILE)

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: blocking_file_copy <from> <to>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    boost::asio::stream_file from_file(io_context, argv[1],
        boost::asio::stream_file::read_only);

    boost::asio::stream_file to_file(io_context, argv[2],
        boost::asio::stream_file::write_only
          | boost::asio::stream_file::create
          | boost::asio::stream_file::truncate);

    char data[4096];
    boost::system::error_code error;
    for (;;)
    {
      std::size_t n = from_file.read_some(boost::asio::buffer(data), error);
      if (error)
        break;
      boost::asio::write(to_file, boost::asio::buffer(data, n), error);
      if (error)
        break;
    }

    if (error && error != boost::asio::error::eof)
    {
      ex_out << "Error copying file: " << error.message() << "\n";
      return 1;
    }
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_FILE)
int example_main(int, char*[]) {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_FILE)

TEST_CASE("example: cpp11/files/blocking_file_copy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    static const std::string ex_src = ex_temp_path("copy-src"); std::ofstream(ex_src) << "file contents\n";
    ex_args.push_back(ex_src);
    ex_args.push_back(ex_temp_path("copy-dst"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_cpp11_files_blocking_file_copy

// libs/asio/example/cpp11/invocation/prioritised_handlers.cpp
namespace ex_cpp11_invocation_prioritised_handlers {
using boost::asio::ip::tcp;

class handler_priority_queue : public boost::asio::execution_context
{
public:
  template <typename Function>
  void add(int priority, Function function)
  {
    std::unique_ptr<queued_handler_base> handler(
        new queued_handler<Function>(
          priority, std::move(function)));

    handlers_.push(std::move(handler));
  }

  void execute_all()
  {
    while (!handlers_.empty())
    {
      handlers_.top()->execute();
      handlers_.pop();
    }
  }

  class executor
  {
  public:
    executor(handler_priority_queue& q, int p)
      : context_(q), priority_(p)
    {
    }

    handler_priority_queue& context() const noexcept
    {
      return context_;
    }

    template <typename Function, typename Allocator>
    void dispatch(Function f, const Allocator&) const
    {
      context_.add(priority_, std::move(f));
    }

    template <typename Function, typename Allocator>
    void post(Function f, const Allocator&) const
    {
      context_.add(priority_, std::move(f));
    }

    template <typename Function, typename Allocator>
    void defer(Function f, const Allocator&) const
    {
      context_.add(priority_, std::move(f));
    }

    void on_work_started() const noexcept {}
    void on_work_finished() const noexcept {}

    bool operator==(const executor& other) const noexcept
    {
      return &context_ == &other.context_ && priority_ == other.priority_;
    }

    bool operator!=(const executor& other) const noexcept
    {
      return !operator==(other);
    }

  private:
    handler_priority_queue& context_;
    int priority_;
  };

  template <typename Handler>
  boost::asio::executor_binder<Handler, executor>
  wrap(int priority, Handler handler)
  {
    return boost::asio::bind_executor(
        executor(*this, priority), std::move(handler));
  }

private:
  class queued_handler_base
  {
  public:
    queued_handler_base(int p)
      : priority_(p)
    {
    }

    virtual ~queued_handler_base()
    {
    }

    virtual void execute() = 0;

    friend bool operator<(const std::unique_ptr<queued_handler_base>& a,
        const std::unique_ptr<queued_handler_base>& b) noexcept
    {
      return a->priority_ < b->priority_;
    }

  private:
    int priority_;
  };

  template <typename Function>
  class queued_handler : public queued_handler_base
  {
  public:
    queued_handler(int p, Function f)
      : queued_handler_base(p), function_(std::move(f))
    {
    }

    void execute() override
    {
      function_();
    }

  private:
    Function function_;
  };

  std::priority_queue<std::unique_ptr<queued_handler_base>> handlers_;
};

//----------------------------------------------------------------------

void high_priority_handler(const boost::system::error_code& /*ec*/,
    tcp::socket /*socket*/)
{
  ex_out << "High priority handler\n";
}

void middle_priority_handler(const boost::system::error_code& /*ec*/)
{
  ex_out << "Middle priority handler\n";
}

struct low_priority_handler
{
  // Make the handler a move-only type.
  low_priority_handler() = default;
  low_priority_handler(const low_priority_handler&) = delete;
  low_priority_handler(low_priority_handler&&) = default;

  void operator()()
  {
    ex_out << "Low priority handler\n";
  }
};

int example_main()
{
  boost::asio::io_context io_context;

  handler_priority_queue pri_queue;

  // Post a completion handler to be run immediately.
  boost::asio::post(io_context, pri_queue.wrap(0, low_priority_handler()));

  // Start an asynchronous accept that will complete immediately.
  tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 0);
  tcp::acceptor acceptor(io_context, endpoint);
  tcp::socket server_socket(io_context);
  acceptor.async_accept(pri_queue.wrap(100, high_priority_handler));
  tcp::socket client_socket(io_context);
  client_socket.connect(acceptor.local_endpoint());

  // Set a deadline timer to expire immediately.
  boost::asio::steady_timer timer(io_context);
  timer.expires_at(boost::asio::steady_timer::clock_type::time_point::min());
  timer.async_wait(pri_queue.wrap(42, middle_priority_handler));

  while (io_context.run_one())
  {
    // The custom invocation hook adds the handlers to the priority queue
    // rather than executing them from within the poll_one() call.
    while (io_context.poll_one())
      ;

    pri_queue.execute_all();
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/invocation/prioritised_handlers") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(High priority handler
Middle priority handler
Low priority handler
)ex"));
}
}  // namespace ex_cpp11_invocation_prioritised_handlers

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/asio/example/cpp11/local/connect_pair.cpp
namespace ex_cpp11_local_connect_pair {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

using boost::asio::local::stream_protocol;

class uppercase_filter
{
public:
  uppercase_filter(stream_protocol::socket sock)
    : socket_(std::move(sock))
  {
    read();
  }

private:
  void read()
  {
    socket_.async_read_some(boost::asio::buffer(data_),
        [this](boost::system::error_code ec, std::size_t size)
        {
          if (!ec)
          {
            // Compute result.
            for (std::size_t i = 0; i < size; ++i)
              data_[i] = std::toupper(data_[i]);

            // Send result.
            write(size);
          }
          else
          {
            throw boost::system::system_error(ec);
          }
        });
  }

  void write(std::size_t size)
  {
    boost::asio::async_write(socket_, boost::asio::buffer(data_, size),
        [this](boost::system::error_code ec, std::size_t /*size*/)
        {
          if (!ec)
          {
            // Wait for request.
            read();
          }
          else
          {
            throw boost::system::system_error(ec);
          }
        });
  }

  stream_protocol::socket socket_;
  std::array<char, 512> data_;
};

int example_main()
{
  try
  {
    boost::asio::io_context io_context;

    // Create a connected pair and pass one end to a filter.
    stream_protocol::socket socket(io_context);
    stream_protocol::socket filter_socket(io_context);
    boost::asio::local::connect_pair(socket, filter_socket);
    uppercase_filter filter(std::move(filter_socket));

    // The io_context runs in a background thread to perform filtering.
    boost::thread thread(
        [&io_context]()
        {
          try
          {
            io_context.run();
          }
          catch (std::exception& e)
          {
            ex_out << "Exception in thread: " << e.what() << "\n";
            std::exit(1);
          }
        });

    for (;;)
    {
      // Collect request from user.
      ex_out << "Enter a string: ";
      std::string request;
      if (!std::getline(ex_in, request)) { io_context.stop(); thread.join(); break; }

      // Send request to filter.
      boost::asio::write(socket, boost::asio::buffer(request));

      // Wait for reply from filter.
      std::vector<char> reply(request.size());
      boost::asio::read(socket, boost::asio::buffer(reply));

      // Show reply to user.
      ex_out << "Result: ";
      ex_out.write(&reply[0], request.size());
      ex_out << std::endl;
    }
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
    std::exit(1);
  }
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

TEST_CASE("example: cpp11/local/connect_pair") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hello
world
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter a string: Result: HELLO
Enter a string: Result: WORLD
Enter a string: )ex"));
}
}  // namespace ex_cpp11_local_connect_pair
#endif

// libs/asio/example/cpp11/operations/composed_1.cpp
namespace ex_cpp11_operations_composed_1 {
using boost::asio::ip::tcp;

//------------------------------------------------------------------------------

// This is the simplest example of a composed asynchronous operation, where we
// simply repackage an existing operation. The asynchronous operation
// requirements are met by delegating responsibility to the underlying
// operation.

template <typename CompletionToken>
auto async_write_message(tcp::socket& socket,
    const char* message, CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is void.
  // However, when the completion token is boost::asio::yield_context (used for
  // stackful coroutines) the return type would be std::size_t, and when the
  // completion token is boost::asio::use_future it would be std::future<std::size_t>.
  // When the completion token is boost::asio::deferred, the return type differs for
  // each asynchronous operation.
  //
  // In this example we are trivially delegating to an underlying asynchronous
  // operation, so we can deduce the return type from that.
  -> decltype(
      boost::asio::async_write(socket,
        boost::asio::buffer(message, std::strlen(message)),
        std::forward<CompletionToken>(token)))
{
  // When delegating to the underlying operation we must take care to perfectly
  // forward the completion token. This ensures that our operation works
  // correctly with move-only function objects as callbacks, as well as other
  // completion token types.
  return boost::asio::async_write(socket,
      boost::asio::buffer(message, std::strlen(message)),
      std::forward<CompletionToken>(token));
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, "Testing callback\r\n",
      [](const boost::system::error_code& error, std::size_t n)
      {
        if (!error)
        {
          ex_out << n << " bytes transferred\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation and its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_write_message(socket,
      "Testing deferred\r\n", boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error, std::size_t n)
      {
        if (!error)
        {
          ex_out << n << " bytes transferred\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<std::size_t> f = async_write_message(
      socket, "Testing future\r\n", boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    std::size_t n = f.get();
    ex_out << n << " bytes transferred\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/operations/composed_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(18 bytes transferred
18 bytes transferred
16 bytes transferred
)ex"));
}
}  // namespace ex_cpp11_operations_composed_1

// libs/asio/example/cpp11/operations/composed_2.cpp
namespace ex_cpp11_operations_composed_2 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This next simplest example of a composed asynchronous operation involves
// repackaging multiple operations but choosing to invoke just one of them. All
// of these underlying operations have the same completion signature. The
// asynchronous operation requirements are met by delegating responsibility to
// the underlying operations.

// In addition to determining the mechanism by which an asynchronous operation
// delivers its result, a completion token also determines the time when the
// operation commences. For example, when the completion token is a simple
// callback the operation commences before the initiating function returns.
// However, if the completion token's delivery mechanism uses a future, we
// might instead want to defer initiation of the operation until the returned
// future object is waited upon.
//
// To enable this, when implementing an asynchronous operation we must package
// the initiation step as a function object.
struct async_write_message_initiation
{
  // The initiation function object's call operator is passed the concrete
  // completion handler produced by the completion token. This completion
  // handler matches the asynchronous operation's completion handler signature,
  // which in this example is:
  //
  //   void(boost::system::error_code error, std::size_t)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments as members in the initiaton function object. However, we should
  // prefer to propagate them as function call arguments as this allows the
  // completion token to optimise how they are passed. For example, a lazy
  // future which defers initiation would need to make a decay-copy of the
  // arguments, but when using a simple callback the arguments can be trivially
  // forwarded straight through.)
  template <typename CompletionHandler>
  void operator()(CompletionHandler&& completion_handler, tcp::socket& socket,
      const char* message, bool allow_partial_write) const
  {
    if (allow_partial_write)
    {
      // When delegating to an underlying operation we must take care to
      // perfectly forward the completion handler. This ensures that our
      // operation works correctly with move-only function objects as
      // callbacks.
      return socket.async_write_some(
          boost::asio::buffer(message, std::strlen(message)),
          std::forward<CompletionHandler>(completion_handler));
    }
    else
    {
      // As above, we must perfectly forward the completion handler when calling
      // the alternate underlying operation.
      return boost::asio::async_write(socket,
          boost::asio::buffer(message, std::strlen(message)),
          std::forward<CompletionHandler>(completion_handler));
    }
  }
};

template <typename CompletionToken>
auto async_write_message(tcp::socket& socket,
    const char* message, bool allow_partial_write,
    CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is void.
  // However, when the completion token is boost::asio::yield_context (used for
  // stackful coroutines) the return type would be std::size_t, and when the
  // completion token is boost::asio::use_future it would be std::future<std::size_t>.
  // When the completion token is boost::asio::deferred, the return type differs for
  // each asynchronous operation.
  //
  // In C++11 we deduce the type from the call to boost::asio::async_initiate.
  -> decltype(
      boost::asio::async_initiate<
        CompletionToken, void(boost::system::error_code, std::size_t)>(
          async_write_message_initiation(),
          token, std::ref(socket), message, allow_partial_write))
{
  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code, std::size_t)>(
      async_write_message_initiation(),
      token, std::ref(socket), message, allow_partial_write);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, "Testing callback\r\n", false,
      [](const boost::system::error_code& error, std::size_t n)
      {
        if (!error)
        {
          ex_out << n << " bytes transferred\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation and its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_write_message(socket,
      "Testing deferred\r\n", false, boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error, std::size_t n)
      {
        if (!error)
        {
          ex_out << n << " bytes transferred\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<std::size_t> f = async_write_message(
      socket, "Testing future\r\n", false, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    std::size_t n = f.get();
    ex_out << n << " bytes transferred\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/operations/composed_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(18 bytes transferred
18 bytes transferred
16 bytes transferred
)ex"));
}
}  // namespace ex_cpp11_operations_composed_2

// libs/asio/example/cpp11/operations/composed_3.cpp
namespace ex_cpp11_operations_composed_3 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// In this composed operation we repackage an existing operation, but with a
// different completion handler signature. The asynchronous operation
// requirements are met by delegating responsibility to the underlying
// operation.

// In addition to determining the mechanism by which an asynchronous operation
// delivers its result, a completion token also determines the time when the
// operation commences. For example, when the completion token is a simple
// callback the operation commences before the initiating function returns.
// However, if the completion token's delivery mechanism uses a future, we
// might instead want to defer initiation of the operation until the returned
// future object is waited upon.
//
// To enable this, when implementing an asynchronous operation we must package
// the initiation step as a function object.
struct async_write_message_initiation
{
  // The initiation function object's call operator is passed the concrete
  // completion handler produced by the completion token. This completion
  // handler matches the asynchronous operation's completion handler signature,
  // which in this example is:
  //
  //   void(boost::system::error_code error)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments as members in the initiaton function object. However, we should
  // prefer to propagate them as function call arguments as this allows the
  // completion token to optimise how they are passed. For example, a lazy
  // future which defers initiation would need to make a decay-copy of the
  // arguments, but when using a simple callback the arguments can be trivially
  // forwarded straight through.)
  template <typename CompletionHandler>
  void operator()(CompletionHandler&& completion_handler,
      tcp::socket& socket, const char* message) const
  {
    // The async_write operation has a completion handler signature of:
    //
    //   void(boost::system::error_code error, std::size n)
    //
    // This differs from our operation's signature in that it is also passed
    // the number of bytes transferred as an argument of type std::size_t. We
    // will adapt our completion handler to async_write's completion handler
    // signature by using std::bind, which drops the additional argument.
    //
    // However, it is essential to the correctness of our composed operation
    // that we preserve the executor of the user-supplied completion handler.
    // The std::bind function will not do this for us, so we must do this by
    // first obtaining the completion handler's associated executor (defaulting
    // to the I/O executor - in this case the executor of the socket - if the
    // completion handler does not have its own) ...
    auto executor = boost::asio::get_associated_executor(
        completion_handler, socket.get_executor());

    // ... and then binding this executor to our adapted completion handler
    // using the boost::asio::bind_executor function.
    boost::asio::async_write(socket,
        boost::asio::buffer(message, std::strlen(message)),
        boost::asio::bind_executor(executor,
          std::bind(std::forward<CompletionHandler>(
            completion_handler), std::placeholders::_1)));
  }
};

template <typename CompletionToken>
auto async_write_message(tcp::socket& socket,
    const char* message, CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++11 we deduce the type from the call to boost::asio::async_initiate.
  -> decltype(
      boost::asio::async_initiate<
        CompletionToken, void(boost::system::error_code)>(
          async_write_message_initiation(),
          token, std::ref(socket), message))
{
  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code)>(
      async_write_message_initiation(),
      token, std::ref(socket), message);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, "Testing callback\r\n",
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation and its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_write_message(socket,
      "Testing deferred\r\n", boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_message(
      socket, "Testing future\r\n", boost::asio::use_future);

  io_context.run();

  // Get the result of the operation.
  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Message sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/operations/composed_3") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Message sent
Message sent
Message sent
)ex"));
}
}  // namespace ex_cpp11_operations_composed_3

// libs/asio/example/cpp11/operations/composed_4.cpp
namespace ex_cpp11_operations_composed_4 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// In this composed operation we repackage an existing operation, but with a
// different completion handler signature. We will also intercept an empty
// message as an invalid argument, and propagate the corresponding error to the
// user. The asynchronous operation requirements are met by delegating
// responsibility to the underlying operation.

// In addition to determining the mechanism by which an asynchronous operation
// delivers its result, a completion token also determines the time when the
// operation commences. For example, when the completion token is a simple
// callback the operation commences before the initiating function returns.
// However, if the completion token's delivery mechanism uses a future, we
// might instead want to defer initiation of the operation until the returned
// future object is waited upon.
//
// To enable this, when implementing an asynchronous operation we must package
// the initiation step as a function object.
struct async_write_message_initiation
{
  // The initiation function object's call operator is passed the concrete
  // completion handler produced by the completion token. This completion
  // handler matches the asynchronous operation's completion handler signature,
  // which in this example is:
  //
  //   void(boost::system::error_code error)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments as members in the initiaton function object. However, we should
  // prefer to propagate them as function call arguments as this allows the
  // completion token to optimise how they are passed. For example, a lazy
  // future which defers initiation would need to make a decay-copy of the
  // arguments, but when using a simple callback the arguments can be trivially
  // forwarded straight through.)
  template <typename CompletionHandler>
  void operator()(CompletionHandler&& completion_handler,
      tcp::socket& socket, const char* message) const
  {
    // The post operation has a completion handler signature of:
    //
    //   void()
    //
    // and the async_write operation has a completion handler signature of:
    //
    //   void(boost::system::error_code error, std::size n)
    //
    // Both of these operations' completion handler signatures differ from our
    // operation's completion handler signature. We will adapt our completion
    // handler to these signatures by using std::bind, which drops the
    // additional arguments.
    //
    // However, it is essential to the correctness of our composed operation
    // that we preserve the executor of the user-supplied completion handler.
    // The std::bind function will not do this for us, so we must do this by
    // first obtaining the completion handler's associated executor (defaulting
    // to the I/O executor - in this case the executor of the socket - if the
    // completion handler does not have its own) ...
    auto executor = boost::asio::get_associated_executor(
        completion_handler, socket.get_executor());

    // ... and then binding this executor to our adapted completion handler
    // using the boost::asio::bind_executor function.
    std::size_t length = std::strlen(message);
    if (length == 0)
    {
      boost::asio::post(
          boost::asio::bind_executor(executor,
            std::bind(std::forward<CompletionHandler>(completion_handler),
              boost::asio::error::invalid_argument)));
    }
    else
    {
      boost::asio::async_write(socket,
          boost::asio::buffer(message, length),
          boost::asio::bind_executor(executor,
            std::bind(std::forward<CompletionHandler>(completion_handler),
              std::placeholders::_1)));
    }
  }
};

template <typename CompletionToken>
auto async_write_message(tcp::socket& socket,
    const char* message, CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++11 we deduce the type from the call to boost::asio::async_initiate.
  -> decltype(
      boost::asio::async_initiate<
        CompletionToken, void(boost::system::error_code)>(
          async_write_message_initiation(),
          token, std::ref(socket), message))
{
  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code)>(
      async_write_message_initiation(),
      token, std::ref(socket), message);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, "",
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation and its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_write_message(socket, "", boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_message(
      socket, "", boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Message sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/operations/composed_4") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Error: Invalid argument
Error: Invalid argument
Exception: Invalid argument [system:22]
)ex"));
}
}  // namespace ex_cpp11_operations_composed_4

// libs/asio/example/cpp11/operations/composed_5.cpp
namespace ex_cpp11_operations_composed_5 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation automatically serialises a message, using its I/O
// streams insertion operator, before sending it on the socket. To do this, it
// must allocate a buffer for the encoded message and ensure this buffer's
// validity until the underlying async_write operation completes.

// In addition to determining the mechanism by which an asynchronous operation
// delivers its result, a completion token also determines the time when the
// operation commences. For example, when the completion token is a simple
// callback the operation commences before the initiating function returns.
// However, if the completion token's delivery mechanism uses a future, we
// might instead want to defer initiation of the operation until the returned
// future object is waited upon.
//
// To enable this, when implementing an asynchronous operation we must package
// the initiation step as a function object.
struct async_write_message_initiation
{
  // The initiation function object's call operator is passed the concrete
  // completion handler produced by the completion token. This completion
  // handler matches the asynchronous operation's completion handler signature,
  // which in this example is:
  //
  //   void(boost::system::error_code error)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments as members in the initiaton function object. However, we should
  // prefer to propagate them as function call arguments as this allows the
  // completion token to optimise how they are passed. For example, a lazy
  // future which defers initiation would need to make a decay-copy of the
  // arguments, but when using a simple callback the arguments can be trivially
  // forwarded straight through.)
  template <typename CompletionHandler>
  void operator()(CompletionHandler&& completion_handler,
      tcp::socket& socket, std::unique_ptr<std::string> encoded_message) const
  {
    // In this example, the composed operation's intermediate completion
    // handler is implemented as a hand-crafted function object, rather than
    // using a lambda or std::bind.
    struct intermediate_completion_handler
    {
      // The intermediate completion handler holds a reference to the socket so
      // that it can obtain the I/O executor (see get_executor below).
      tcp::socket& socket_;

      // The allocated buffer for the encoded message. The std::unique_ptr
      // smart pointer is move-only, and as a consequence our intermediate
      // completion handler is also move-only.
      std::unique_ptr<std::string> encoded_message_;

      // The user-supplied completion handler.
      typename std::decay<CompletionHandler>::type handler_;

      // The function call operator matches the completion signature of the
      // async_write operation.
      void operator()(const boost::system::error_code& error, std::size_t /*n*/)
      {
        // Deallocate the encoded message before calling the user-supplied
        // completion handler.
        encoded_message_.reset();

        // Call the user-supplied handler with the result of the operation.
        // The arguments must match the completion signature of our composed
        // operation.
        handler_(error);
      }

      // It is essential to the correctness of our composed operation that we
      // preserve the executor of the user-supplied completion handler. With a
      // hand-crafted function object we can do this by defining a nested type
      // executor_type and member function get_executor. These obtain the
      // completion handler's associated executor, and default to the I/O
      // executor - in this case the executor of the socket - if the completion
      // handler does not have its own.
      using executor_type = boost::asio::associated_executor_t<
          typename std::decay<CompletionHandler>::type,
          tcp::socket::executor_type>;

      executor_type get_executor() const noexcept
      {
        return boost::asio::get_associated_executor(
            handler_, socket_.get_executor());
      }

      // Although not necessary for correctness, we may also preserve the
      // allocator of the user-supplied completion handler. This is achieved by
      // defining a nested type allocator_type and member function
      // get_allocator. These obtain the completion handler's associated
      // allocator, and default to std::allocator<void> if the completion
      // handler does not have its own.
      using allocator_type = boost::asio::associated_allocator_t<
          typename std::decay<CompletionHandler>::type,
          std::allocator<void>>;

      allocator_type get_allocator() const noexcept
      {
        return boost::asio::get_associated_allocator(
            handler_, std::allocator<void>{});
      }
    };

    // Initiate the underlying async_write operation using our intermediate
    // completion handler.
    auto encoded_message_buffer = boost::asio::buffer(*encoded_message);
    boost::asio::async_write(socket, encoded_message_buffer,
        intermediate_completion_handler{socket, std::move(encoded_message),
          std::forward<CompletionHandler>(completion_handler)});
  }
};

template <typename T, typename CompletionToken>
auto async_write_message(tcp::socket& socket,
    const T& message, CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++11 we deduce the type from the call to boost::asio::async_initiate.
  -> decltype(
      boost::asio::async_initiate<
        CompletionToken, void(boost::system::error_code)>(
          async_write_message_initiation(), token,
          std::ref(socket), std::declval<std::unique_ptr<std::string>>()))
{
  // Encode the message and copy it into an allocated buffer. The buffer will
  // be maintained for the lifetime of the asynchronous operation.
  std::ostringstream os;
  os << message;
  std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code)>(
      async_write_message_initiation(), token,
      std::ref(socket), std::move(encoded_message));
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, 123456,
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation and its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_write_message(socket,
      std::string("abcdef"), boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_message(
      socket, 654.321, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Message sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/operations/composed_5") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Message sent
Message sent
Message sent
)ex"));
}
}  // namespace ex_cpp11_operations_composed_5

// libs/asio/example/cpp11/operations/composed_6.cpp
namespace ex_cpp11_operations_composed_6 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation shows composition of multiple underlying operations.
// It automatically serialises a message, using its I/O streams insertion
// operator, before sending it N times on the socket. To do this, it must
// allocate a buffer for the encoded message and ensure this buffer's validity
// until all underlying async_write operation complete. A one second delay is
// inserted prior to each write operation, using a steady_timer.

// In addition to determining the mechanism by which an asynchronous operation
// delivers its result, a completion token also determines the time when the
// operation commences. For example, when the completion token is a simple
// callback the operation commences before the initiating function returns.
// However, if the completion token's delivery mechanism uses a future, we
// might instead want to defer initiation of the operation until the returned
// future object is waited upon.
//
// To enable this, when implementing an asynchronous operation we must package
// the initiation step as a function object.
struct async_write_message_initiation
{
  // The initiation function object's call operator is passed the concrete
  // completion handler produced by the completion token. This completion
  // handler matches the asynchronous operation's completion handler signature,
  // which in this example is:
  //
  //   void(boost::system::error_code error)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments as members in the initiaton function object. However, we should
  // prefer to propagate them as function call arguments as this allows the
  // completion token to optimise how they are passed. For example, a lazy
  // future which defers initiation would need to make a decay-copy of the
  // arguments, but when using a simple callback the arguments can be trivially
  // forwarded straight through.)
  template <typename CompletionHandler>
  void operator()(CompletionHandler&& completion_handler, tcp::socket& socket,
      std::unique_ptr<std::string> encoded_message, std::size_t repeat_count,
      std::unique_ptr<boost::asio::steady_timer> delay_timer) const
  {
    // In this example, the composed operation's intermediate completion
    // handler is implemented as a hand-crafted function object.
    struct intermediate_completion_handler
    {
      // The intermediate completion handler holds a reference to the socket as
      // it is used for multiple async_write operations, as well as for
      // obtaining the I/O executor (see get_executor below).
      tcp::socket& socket_;

      // The allocated buffer for the encoded message. The std::unique_ptr
      // smart pointer is move-only, and as a consequence our intermediate
      // completion handler is also move-only.
      std::unique_ptr<std::string> encoded_message_;

      // The repeat count remaining.
      std::size_t repeat_count_;

      // A steady timer used for introducing a delay.
      std::unique_ptr<boost::asio::steady_timer> delay_timer_;

      // To manage the cycle between the multiple underlying asynchronous
      // operations, our intermediate completion handler is implemented as a
      // state machine.
      enum { starting, waiting, writing } state_;

      // As our composed operation performs multiple underlying I/O operations,
      // we should maintain a work object against the I/O executor. This tells
      // the I/O executor that there is still more work to come in the future.
      boost::asio::executor_work_guard<tcp::socket::executor_type> io_work_;

      // The user-supplied completion handler, called once only on completion
      // of the entire composed operation.
      typename std::decay<CompletionHandler>::type handler_;

      // By having a default value for the second argument, this function call
      // operator matches the completion signature of both the async_write and
      // steady_timer::async_wait operations.
      void operator()(const boost::system::error_code& error, std::size_t = 0)
      {
        if (!error)
        {
          switch (state_)
          {
          case starting:
          case writing:
            if (repeat_count_ > 0)
            {
              --repeat_count_;
              state_ = waiting;
              delay_timer_->expires_after(std::chrono::milliseconds(10));
              delay_timer_->async_wait(std::move(*this));
              return; // Composed operation not yet complete.
            }
            break; // Composed operation complete, continue below.
          case waiting:
            state_ = writing;
            boost::asio::async_write(socket_,
                boost::asio::buffer(*encoded_message_), std::move(*this));
            return; // Composed operation not yet complete.
          }
        }

        // This point is reached only on completion of the entire composed
        // operation.

        // We no longer have any future work coming for the I/O executor.
        io_work_.reset();

        // Deallocate the encoded message and delay timer before calling the
        // user-supplied completion handler.
        encoded_message_.reset();
        delay_timer_.reset();

        // Call the user-supplied handler with the result of the operation.
        handler_(error);
      }

      // It is essential to the correctness of our composed operation that we
      // preserve the executor of the user-supplied completion handler. With a
      // hand-crafted function object we can do this by defining a nested type
      // executor_type and member function get_executor. These obtain the
      // completion handler's associated executor, and default to the I/O
      // executor - in this case the executor of the socket - if the completion
      // handler does not have its own.
      using executor_type = boost::asio::associated_executor_t<
          typename std::decay<CompletionHandler>::type,
          tcp::socket::executor_type>;

      executor_type get_executor() const noexcept
      {
        return boost::asio::get_associated_executor(
            handler_, socket_.get_executor());
      }

      // Although not necessary for correctness, we may also preserve the
      // allocator of the user-supplied completion handler. This is achieved by
      // defining a nested type allocator_type and member function
      // get_allocator. These obtain the completion handler's associated
      // allocator, and default to std::allocator<void> if the completion
      // handler does not have its own.
      using allocator_type = boost::asio::associated_allocator_t<
          typename std::decay<CompletionHandler>::type,
          std::allocator<void>>;

      allocator_type get_allocator() const noexcept
      {
        return boost::asio::get_associated_allocator(
            handler_, std::allocator<void>{});
      }
    };

    // Initiate the underlying async_write operation using our intermediate
    // completion handler.
    auto encoded_message_buffer = boost::asio::buffer(*encoded_message);
    boost::asio::async_write(socket, encoded_message_buffer,
        intermediate_completion_handler{
          socket, std::move(encoded_message),
          repeat_count, std::move(delay_timer),
          intermediate_completion_handler::starting,
          boost::asio::make_work_guard(socket.get_executor()),
          std::forward<CompletionHandler>(completion_handler)});
  }
};

template <typename T, typename CompletionToken>
auto async_write_messages(tcp::socket& socket,
    const T& message, std::size_t repeat_count,
    CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++11 we deduce the type from the call to boost::asio::async_initiate.
  -> decltype(
      boost::asio::async_initiate<
        CompletionToken, void(boost::system::error_code)>(
          async_write_message_initiation(), token, std::ref(socket),
          std::declval<std::unique_ptr<std::string>>(), repeat_count,
          std::declval<std::unique_ptr<boost::asio::steady_timer>>()))
{
  // Encode the message and copy it into an allocated buffer. The buffer will
  // be maintained for the lifetime of the composed asynchronous operation.
  std::ostringstream os;
  os << message;
  std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

  // Create a steady_timer to be used for the delay between messages.
  std::unique_ptr<boost::asio::steady_timer> delay_timer(
      new boost::asio::steady_timer(socket.get_executor()));

  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code)>(
      async_write_message_initiation(), token, std::ref(socket),
      std::move(encoded_message), repeat_count, std::move(delay_timer));
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_messages(socket, "Testing callback\r\n", 5,
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation and its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_write_messages(socket,
      "Testing deferred\r\n", 5, boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_messages(
      socket, "Testing future\r\n", 5, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Messages sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/operations/composed_6") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Messages sent
Messages sent
Messages sent
)ex"));
}
}  // namespace ex_cpp11_operations_composed_6

// libs/asio/example/cpp11/operations/composed_7.cpp
namespace ex_cpp11_operations_composed_7 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_compose function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation shows composition of multiple underlying operations.
// It automatically serialises a message, using its I/O streams insertion
// operator, before sending it N times on the socket. To do this, it must
// allocate a buffer for the encoded message and ensure this buffer's validity
// until all underlying async_write operation complete. A one second delay is
// inserted prior to each write operation, using a steady_timer.

// In this example, the composed operation's logic is implemented as a state
// machine within a hand-crafted function object.
struct async_write_messages_implementation
{
  // The implementation holds a reference to the socket as it is used for
  // multiple async_write operations.
  tcp::socket& socket_;

  // The allocated buffer for the encoded message. The std::unique_ptr smart
  // pointer is move-only, and as a consequence our implementation is also
  // move-only.
  std::unique_ptr<std::string> encoded_message_;

  // The repeat count remaining.
  std::size_t repeat_count_;

  // A steady timer used for introducing a delay.
  std::unique_ptr<boost::asio::steady_timer> delay_timer_;

  // To manage the cycle between the multiple underlying asynchronous
  // operations, our implementation is a state machine.
  enum { starting, waiting, writing } state_;

  // The first argument to our function object's call operator is a reference
  // to the enclosing intermediate completion handler. This intermediate
  // completion handler is provided for us by the boost::asio::async_compose
  // function, and takes care of all the details required to implement a
  // conforming asynchronous operation. When calling an underlying asynchronous
  // operation, we pass it this enclosing intermediate completion handler
  // as the completion token.
  //
  // All arguments after the first must be defaulted to allow the state machine
  // to be started, as well as to allow the completion handler to match the
  // completion signature of both the async_write and steady_timer::async_wait
  // operations.
  template <typename Self>
  void operator()(Self& self,
      const boost::system::error_code& error = boost::system::error_code(),
      std::size_t = 0)
  {
    if (!error)
    {
      switch (state_)
      {
      case starting:
      case writing:
        if (repeat_count_ > 0)
        {
          --repeat_count_;
          state_ = waiting;
          delay_timer_->expires_after(std::chrono::milliseconds(10));
          delay_timer_->async_wait(std::move(self));
          return; // Composed operation not yet complete.
        }
        break; // Composed operation complete, continue below.
      case waiting:
        state_ = writing;
        boost::asio::async_write(socket_,
            boost::asio::buffer(*encoded_message_), std::move(self));
        return; // Composed operation not yet complete.
      }
    }

    // This point is reached only on completion of the entire composed
    // operation.

    // Deallocate the encoded message and delay timer before calling the
    // user-supplied completion handler.
    encoded_message_.reset();
    delay_timer_.reset();

    // Call the user-supplied handler with the result of the operation.
    self.complete(error);
  }
};

template <typename T, typename CompletionToken>
auto async_write_messages(tcp::socket& socket,
    const T& message, std::size_t repeat_count,
    CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++11 we deduce the type from the call to boost::asio::async_compose.
  -> decltype(
      boost::asio::async_compose<
        CompletionToken, void(boost::system::error_code)>(
          std::declval<async_write_messages_implementation>(),
          token, socket))
{
  // Encode the message and copy it into an allocated buffer. The buffer will
  // be maintained for the lifetime of the composed asynchronous operation.
  std::ostringstream os;
  os << message;
  std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

  // Create a steady_timer to be used for the delay between messages.
  std::unique_ptr<boost::asio::steady_timer> delay_timer(
      new boost::asio::steady_timer(socket.get_executor()));

  // The boost::asio::async_compose function takes:
  //
  // - our asynchronous operation implementation,
  // - the completion token,
  // - the completion handler signature, and
  // - any I/O objects (or executors) used by the operation
  //
  // It then wraps our implementation in an intermediate completion handler
  // that meets the requirements of a conforming asynchronous operation. This
  // includes tracking outstanding work against the I/O executors associated
  // with the operation (in this example, this is the socket's executor).
  return boost::asio::async_compose<
    CompletionToken, void(boost::system::error_code)>(
      async_write_messages_implementation{
        socket, std::move(encoded_message),
        repeat_count, std::move(delay_timer),
        async_write_messages_implementation::starting},
      token, socket);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_messages(socket, "Testing callback\r\n", 5,
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation and its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_write_messages(socket,
      "Testing deferred\r\n", 5, boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_messages(
      socket, "Testing future\r\n", 5, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Messages sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/operations/composed_7") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Messages sent
Messages sent
Messages sent
)ex"));
}
}  // namespace ex_cpp11_operations_composed_7

// libs/asio/example/cpp11/operations/composed_8.cpp
namespace ex_cpp11_operations_composed_8 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_compose function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation shows composition of multiple underlying operations,
// using asio's stackless coroutines support to express the flow of control. It
// automatically serialises a message, using its I/O streams insertion
// operator, before sending it N times on the socket. To do this, it must
// allocate a buffer for the encoded message and ensure this buffer's validity
// until all underlying async_write operation complete. A one second delay is
// inserted prior to each write operation, using a steady_timer.

#include <boost/asio/yield.hpp>

// In this example, the composed operation's logic is implemented as a state
// machine within a hand-crafted function object.
struct async_write_messages_implementation
{
  // The implementation holds a reference to the socket as it is used for
  // multiple async_write operations.
  tcp::socket& socket_;

  // The allocated buffer for the encoded message. The std::unique_ptr smart
  // pointer is move-only, and as a consequence our implementation is also
  // move-only.
  std::unique_ptr<std::string> encoded_message_;

  // The repeat count remaining.
  std::size_t repeat_count_;

  // A steady timer used for introducing a delay.
  std::unique_ptr<boost::asio::steady_timer> delay_timer_;

  // The coroutine state.
  boost::asio::coroutine coro_;

  // The first argument to our function object's call operator is a reference
  // to the enclosing intermediate completion handler. This intermediate
  // completion handler is provided for us by the boost::asio::async_compose
  // function, and takes care of all the details required to implement a
  // conforming asynchronous operation. When calling an underlying asynchronous
  // operation, we pass it this enclosing intermediate completion handler
  // as the completion token.
  //
  // All arguments after the first must be defaulted to allow the state machine
  // to be started, as well as to allow the completion handler to match the
  // completion signature of both the async_write and steady_timer::async_wait
  // operations.
  template <typename Self>
  void operator()(Self& self,
      const boost::system::error_code& error = boost::system::error_code(),
      std::size_t = 0)
  {
    reenter (coro_)
    {
      while (repeat_count_ > 0)
      {
        --repeat_count_;

        delay_timer_->expires_after(std::chrono::milliseconds(10));
        yield delay_timer_->async_wait(std::move(self));
        if (error)
          break;

        yield boost::asio::async_write(socket_,
            boost::asio::buffer(*encoded_message_), std::move(self));
        if (error)
          break;
      }

      // Deallocate the encoded message and delay timer before calling the
      // user-supplied completion handler.
      encoded_message_.reset();
      delay_timer_.reset();

      // Call the user-supplied handler with the result of the operation.
      self.complete(error);
    }
  }
};

#include <boost/asio/unyield.hpp>

template <typename T, typename CompletionToken>
auto async_write_messages(tcp::socket& socket,
    const T& message, std::size_t repeat_count,
    CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++11 we deduce the type from the call to boost::asio::async_compose.
  -> decltype(
      boost::asio::async_compose<
        CompletionToken, void(boost::system::error_code)>(
          std::declval<async_write_messages_implementation>(),
          token, socket))
{
  // Encode the message and copy it into an allocated buffer. The buffer will
  // be maintained for the lifetime of the composed asynchronous operation.
  std::ostringstream os;
  os << message;
  std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

  // Create a steady_timer to be used for the delay between messages.
  std::unique_ptr<boost::asio::steady_timer> delay_timer(
      new boost::asio::steady_timer(socket.get_executor()));

  // The boost::asio::async_compose function takes:
  //
  // - our asynchronous operation implementation,
  // - the completion token,
  // - the completion handler signature, and
  // - any I/O objects (or executors) used by the operation
  //
  // It then wraps our implementation in an intermediate completion handler
  // that meets the requirements of a conforming asynchronous operation. This
  // includes tracking outstanding work against the I/O executors associated
  // with the operation (in this example, this is the socket's executor).
  return boost::asio::async_compose<
    CompletionToken, void(boost::system::error_code)>(
      async_write_messages_implementation{socket,
        std::move(encoded_message), repeat_count,
        std::move(delay_timer), boost::asio::coroutine()},
      token, socket);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_messages(socket, "Testing callback\r\n", 5,
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation and its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_write_messages(socket,
      "Testing deferred\r\n", 5, boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_messages(
      socket, "Testing future\r\n", 5, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Messages sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/operations/composed_8") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Messages sent
Messages sent
Messages sent
)ex"));
}
}  // namespace ex_cpp11_operations_composed_8

// libs/asio/example/cpp11/parallel_group/ranged_wait_for_all.cpp
namespace ex_cpp11_parallel_group_ranged_wait_for_all {
#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor out(ctx, ::dup(STDOUT_FILENO));
  boost::asio::posix::stream_descriptor err(ctx, ::dup(STDERR_FILENO));

  using op_type = decltype(
      out.async_write_some(boost::asio::buffer("", 0))
    );

  std::vector<op_type> ops;

  ops.push_back(
      out.async_write_some(boost::asio::buffer("first\r\n", 7))
    );

  ops.push_back(
      err.async_write_some(boost::asio::buffer("second\r\n", 8))
    );

  boost::asio::experimental::make_parallel_group(ops).async_wait(
      boost::asio::experimental::wait_for_all(),
      [](
          std::vector<std::size_t> completion_order,
          std::vector<boost::system::error_code> ec,
          std::vector<std::size_t> n
      )
      {
        for (std::size_t i = 0; i < completion_order.size(); ++i)
        {
          std::size_t idx = completion_order[i];
          ex_out << "operation " << idx << " finished: ";
          ex_out << ec[idx] << ", " << n[idx] << "\n";
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp11/parallel_group/ranged_wait_for_all") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(std::string_view("first\r\n"
        "operation 0 finished: system:0, 7\n"
        "operation 1 finished: system:0, 8\n", 75)));
#endif
}
}  // namespace ex_cpp11_parallel_group_ranged_wait_for_all

// libs/asio/example/cpp11/parallel_group/wait_for_all.cpp
namespace ex_cpp11_parallel_group_wait_for_all {
#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));
  boost::asio::steady_timer timer(ctx, std::chrono::seconds(5));

  char data[1024];

  boost::asio::experimental::make_parallel_group(
      in.async_read_some(boost::asio::buffer(data)),
      timer.async_wait()
    ).async_wait(
      boost::asio::experimental::wait_for_all(),
      [](
          std::array<std::size_t, 2> completion_order,
          boost::system::error_code ec1, std::size_t n1,
          boost::system::error_code ec2
      )
      {
        switch (completion_order[0])
        {
        case 0:
          {
            ex_out << "descriptor finished: " << ec1 << ", " << n1 << "\n";
          }
          break;
        case 1:
          {
            ex_out << "timer finished: " << ec2 << "\n";
          }
          break;
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp11/parallel_group/wait_for_all") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(descriptor finished: asio.misc:2, 0
)ex"));
#endif
}
}  // namespace ex_cpp11_parallel_group_wait_for_all

// libs/asio/example/cpp11/parallel_group/wait_for_one.cpp
namespace ex_cpp11_parallel_group_wait_for_one {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));
  boost::asio::steady_timer timer(ctx, std::chrono::seconds(5));

  char data[1024];

  boost::asio::experimental::make_parallel_group(
      in.async_read_some(boost::asio::buffer(data)),
      timer.async_wait()
    ).async_wait(
      boost::asio::experimental::wait_for_one(),
      [](
          std::array<std::size_t, 2> completion_order,
          boost::system::error_code ec1, std::size_t n1,
          boost::system::error_code ec2
      )
      {
        switch (completion_order[0])
        {
        case 0:
          {
            ex_out << "descriptor finished: " << ec1 << ", " << n1 << "\n";
          }
          break;
        case 1:
          {
            ex_out << "timer finished: " << ec2 << "\n";
          }
          break;
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp11/parallel_group/wait_for_one") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(descriptor finished: asio.misc:2, 0
)ex"));
}
}  // namespace ex_cpp11_parallel_group_wait_for_one

// libs/asio/example/cpp11/parallel_group/wait_for_one_error.cpp
namespace ex_cpp11_parallel_group_wait_for_one_error {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));
  boost::asio::steady_timer timer(ctx, std::chrono::seconds(5));

  char data[1024];

  boost::asio::experimental::make_parallel_group(
      in.async_read_some(boost::asio::buffer(data)),
      timer.async_wait()
    ).async_wait(
      boost::asio::experimental::wait_for_one_error(),
      [](
          std::array<std::size_t, 2> completion_order,
          boost::system::error_code ec1, std::size_t n1,
          boost::system::error_code ec2
      )
      {
        switch (completion_order[0])
        {
        case 0:
          {
            ex_out << "descriptor finished: " << ec1 << ", " << n1 << "\n";
          }
          break;
        case 1:
          {
            ex_out << "timer finished: " << ec2 << "\n";
          }
          break;
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp11/parallel_group/wait_for_one_error") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(descriptor finished: asio.misc:2, 0
)ex"));
}
}  // namespace ex_cpp11_parallel_group_wait_for_one_error

// libs/asio/example/cpp11/parallel_group/wait_for_one_success.cpp
namespace ex_cpp11_parallel_group_wait_for_one_success {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));
  boost::asio::steady_timer timer(ctx, std::chrono::seconds(5));

  char data[1024];

  boost::asio::experimental::make_parallel_group(
      in.async_read_some(boost::asio::buffer(data)),
      timer.async_wait()
    ).async_wait(
      boost::asio::experimental::wait_for_one_success(),
      [](
          std::array<std::size_t, 2> completion_order,
          boost::system::error_code ec1, std::size_t n1,
          boost::system::error_code ec2
      )
      {
        switch (completion_order[0])
        {
        case 0:
          {
            ex_out << "descriptor finished: " << ec1 << ", " << n1 << "\n";
          }
          break;
        case 1:
          {
            ex_out << "timer finished: " << ec2 << "\n";
          }
          break;
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp11/parallel_group/wait_for_one_success") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(descriptor finished: asio.misc:2, 0
)ex"));
}
}  // namespace ex_cpp11_parallel_group_wait_for_one_success

// libs/asio/example/cpp11/timers/time_t_timer.cpp
namespace ex_cpp11_timers_time_t_timer {
struct time_t_clock
{
  // The duration type.
  typedef std::chrono::steady_clock::duration duration;

  // The duration's underlying arithmetic representation.
  typedef duration::rep rep;

  // The ratio representing the duration's tick period.
  typedef duration::period period;

  // An absolute time point represented using the clock.
  typedef std::chrono::time_point<time_t_clock> time_point;

  // The clock is not monotonically increasing.
  static constexpr bool is_steady = false;

  // Get the current time.
  static time_point now() noexcept
  {
    return time_point() + std::chrono::seconds(std::time(0));
  }
};

// The boost::asio::basic_waitable_timer template accepts an optional WaitTraits
// template parameter. The underlying time_t clock has one-second granularity,
// so these traits may be customised to reduce the latency between the clock
// ticking over and a wait operation's completion. When the timeout is near
// (less than one second away) we poll the clock more frequently to detect the
// time change closer to when it occurs. The user can select the appropriate
// trade off between accuracy and the increased CPU cost of polling. In extreme
// cases, a zero duration may be returned to make the timers as accurate as
// possible, albeit with 100% CPU usage.
struct time_t_wait_traits
{
  // Determine how long until the clock should be next polled to determine
  // whether the duration has elapsed.
  static time_t_clock::duration to_wait_duration(
      const time_t_clock::duration& d)
  {
    if (d > std::chrono::seconds(1))
      return d - std::chrono::seconds(1);
    else if (d > std::chrono::seconds(0))
      return std::chrono::milliseconds(10);
    else
      return std::chrono::seconds(0);
  }

  // Determine how long until the clock should be next polled to determine
  // whether the absolute time has been reached.
  static time_t_clock::duration to_wait_duration(
      const time_t_clock::time_point& t)
  {
    return to_wait_duration(t - time_t_clock::now());
  }
};

typedef boost::asio::basic_waitable_timer<
  time_t_clock, time_t_wait_traits> time_t_timer;

int example_main()
{
  try
  {
    boost::asio::io_context io_context;

    time_t_timer timer(io_context);

    timer.expires_after(std::chrono::seconds(5));
    ex_out << "Starting synchronous wait\n";
    timer.wait();
    ex_out << "Finished synchronous wait\n";

    timer.expires_after(std::chrono::seconds(5));
    ex_out << "Starting asynchronous wait\n";
    timer.async_wait(
        [](const boost::system::error_code& /*error*/)
        {
          ex_out << "timeout\n";
        });
    io_context.run();
    ex_out << "Finished asynchronous wait\n";
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/timers/time_t_timer") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Starting synchronous wait
Finished synchronous wait
Starting asynchronous wait
timeout
Finished asynchronous wait
)ex"));
}
}  // namespace ex_cpp11_timers_time_t_timer

// libs/asio/example/cpp11/tutorial/timer1/timer.cpp
namespace ex_cpp11_tutorial_timer1_timer {
int example_main()
{
  boost::asio::io_context io;

  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(5));
  t.wait();

  ex_out << "Hello, world!" << std::endl;

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/timer1/timer") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Hello, world!
)ex"));
}
}  // namespace ex_cpp11_tutorial_timer1_timer

// libs/asio/example/cpp11/tutorial/timer2/timer.cpp
namespace ex_cpp11_tutorial_timer2_timer {
void print(const boost::system::error_code& /*e*/)
{
  ex_out << "Hello, world!" << std::endl;
}

int example_main()
{
  boost::asio::io_context io;

  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(5));
  t.async_wait(&print);

  io.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/timer2/timer") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Hello, world!
)ex"));
}
}  // namespace ex_cpp11_tutorial_timer2_timer

// libs/asio/example/cpp11/tutorial/timer3/timer.cpp
namespace ex_cpp11_tutorial_timer3_timer {
void print(const boost::system::error_code& /*e*/,
    boost::asio::steady_timer* t, int* count)
{
  if (*count < 5)
  {
    ex_out << *count << std::endl;
    ++(*count);

    t->expires_at(t->expiry() + boost::asio::chrono::seconds(1));
    t->async_wait(std::bind(print,
          boost::asio::placeholders::error, t, count));
  }
}

int example_main()
{
  boost::asio::io_context io;

  int count = 0;
  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(1));
  t.async_wait(std::bind(print,
        boost::asio::placeholders::error, &t, &count));

  io.run();

  ex_out << "Final count is " << count << std::endl;

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/timer3/timer") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(0
1
2
3
4
Final count is 5
)ex"));
}
}  // namespace ex_cpp11_tutorial_timer3_timer

// libs/asio/example/cpp11/tutorial/timer4/timer.cpp
namespace ex_cpp11_tutorial_timer4_timer {
class printer
{
public:
  printer(boost::asio::io_context& io)
    : timer_(io, boost::asio::chrono::seconds(1)),
      count_(0)
  {
    timer_.async_wait(std::bind(&printer::print, this));
  }

  ~printer()
  {
    ex_out << "Final count is " << count_ << std::endl;
  }

  void print()
  {
    if (count_ < 5)
    {
      ex_out << count_ << std::endl;
      ++count_;

      timer_.expires_at(timer_.expiry() + boost::asio::chrono::seconds(1));
      timer_.async_wait(std::bind(&printer::print, this));
    }
  }

private:
  boost::asio::steady_timer timer_;
  int count_;
};

int example_main()
{
  boost::asio::io_context io;
  printer p(io);
  io.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/timer4/timer") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(0
1
2
3
4
Final count is 5
)ex"));
}
}  // namespace ex_cpp11_tutorial_timer4_timer

// libs/asio/example/cpp11/tutorial/timer5/timer.cpp
namespace ex_cpp11_tutorial_timer5_timer {
class printer
{
public:
  printer(boost::asio::io_context& io)
    : strand_(boost::asio::make_strand(io)),
      timer1_(io, boost::asio::chrono::seconds(1)),
      timer2_(io, boost::asio::chrono::seconds(1)),
      count_(0)
  {
    timer1_.async_wait(boost::asio::bind_executor(strand_,
          std::bind(&printer::print1, this)));

    timer2_.async_wait(boost::asio::bind_executor(strand_,
          std::bind(&printer::print2, this)));
  }

  ~printer()
  {
    ex_out << "Final count is " << count_ << std::endl;
  }

  void print1()
  {
    if (count_ < 10)
    {
      ex_out << "Timer 1: " << count_ << std::endl;
      ++count_;

      timer1_.expires_at(timer1_.expiry() + boost::asio::chrono::seconds(1));

      timer1_.async_wait(boost::asio::bind_executor(strand_,
            std::bind(&printer::print1, this)));
    }
  }

  void print2()
  {
    if (count_ < 10)
    {
      ex_out << "Timer 2: " << count_ << std::endl;
      ++count_;

      timer2_.expires_at(timer2_.expiry() + boost::asio::chrono::seconds(1));

      timer2_.async_wait(boost::asio::bind_executor(strand_,
            std::bind(&printer::print2, this)));
    }
  }

private:
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::asio::steady_timer timer1_;
  boost::asio::steady_timer timer2_;
  int count_;
};

int example_main()
{
  boost::asio::io_context io;
  printer p(io);
  std::thread t([&]{ io.run(); });
  io.run();
  t.join();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/timer5/timer") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Timer 1: 0
Timer 2: 1
Timer 1: 2
Timer 2: 3
Timer 1: 4
Timer 2: 5
Timer 1: 6
Timer 2: 7
Timer 1: 8
Timer 2: 9
Final count is 10
)ex"));
}
}  // namespace ex_cpp11_tutorial_timer5_timer

#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)  // the program uses what only exists there (see the file header)
// libs/asio/example/cpp11/type_erasure/main.cpp
namespace ex_cpp11_type_erasure_main {
#undef LINE_READER_HPP
//
// line_reader.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef LINE_READER_HPP
#define LINE_READER_HPP


class line_reader
{
private:
  virtual void async_read_line_impl(std::string prompt,
      boost::asio::any_completion_handler<void(boost::system::error_code, std::string)> handler) = 0;

  struct initiate_read_line
  {
    template <typename Handler>
    void operator()(Handler handler, line_reader* self, std::string prompt)
    {
      self->async_read_line_impl(std::move(prompt), std::move(handler));
    }
  };

public:
  virtual ~line_reader() {}

  template <typename CompletionToken>
  auto async_read_line(std::string prompt, CompletionToken&& token)
    -> decltype(
        boost::asio::async_initiate<CompletionToken, void(boost::system::error_code, std::string)>(
          initiate_read_line(), token, this, prompt))
  {
    return boost::asio::async_initiate<CompletionToken, void(boost::system::error_code, std::string)>(
        initiate_read_line(), token, this, prompt);
  }
};

#endif // LINE_READER_HPP
// ---- end line_reader.hpp ----
// ---- inlined sleep.hpp ----
#undef SLEEP_HPP
//
// sleep.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SLEEP_HPP
#define SLEEP_HPP


void async_sleep_impl(
    boost::asio::any_completion_handler<void(boost::system::error_code)> handler,
    boost::asio::any_io_executor ex, std::chrono::nanoseconds duration);

template <typename CompletionToken>
inline auto async_sleep(boost::asio::any_io_executor ex,
    std::chrono::nanoseconds duration, CompletionToken&& token)
  -> decltype(
      boost::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
        async_sleep_impl, token, std::move(ex), duration))
{
  return boost::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
      async_sleep_impl, token, std::move(ex), duration);
}

#endif // SLEEP_HPP
// ---- end sleep.hpp ----
// ---- inlined stdin_line_reader.hpp ----
#undef STDIN_LINE_READER_HPP
//
// stdin_line_reader.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STDIN_LINE_READER_HPP
#define STDIN_LINE_READER_HPP


class stdin_line_reader : public line_reader
{
public:
  explicit stdin_line_reader(boost::asio::any_io_executor ex);

private:
  void async_read_line_impl(std::string prompt,
      boost::asio::any_completion_handler<void(boost::system::error_code, std::string)> handler) override;

  boost::asio::posix::stream_descriptor stdin_;
  std::string buffer_;
};

#endif
// ---- end stdin_line_reader.hpp ----

#include <boost/asio/yield.hpp>

class read_loop : boost::asio::coroutine
{
public:
  read_loop(boost::asio::any_io_executor e, line_reader& r)
    : executor(std::move(e)),
      reader(r)
  {
  }

  void operator()(boost::system::error_code ec = {}, std::string line = {})
  {
    reenter (this)
    {
      for (i = 0; i < 10; ++i)
      {
        yield reader.async_read_line("Enter something: ", *this);
        if (ec) break;
        ex_out << line;
        yield async_sleep(executor, std::chrono::seconds(1), *this);
        if (ec) break;
      }
    }
  }

private:
  boost::asio::any_io_executor executor;
  line_reader& reader;
  int i;
};

#include <boost/asio/unyield.hpp>

int example_main()
{
  boost::asio::io_context ctx{1};
  stdin_line_reader reader{ctx.get_executor()};
  read_loop(ctx.get_executor(), reader)();
  ctx.run();
    return 0;  // main's implicit return
}
// ---- merged sleep.cpp (the same program) ----
//
// sleep.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


void async_sleep_impl(
    boost::asio::any_completion_handler<void(boost::system::error_code)> handler,
    boost::asio::any_io_executor ex, std::chrono::nanoseconds duration)
{
  auto timer = std::make_shared<boost::asio::steady_timer>(ex, duration);
  timer->async_wait(boost::asio::consign(std::move(handler), timer));
}
// ---- merged stdin_line_reader.cpp (the same program) ----
//
// stdin_line_reader.cpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


stdin_line_reader::stdin_line_reader(boost::asio::any_io_executor ex)
  : stdin_(ex, ex_stdin_fd(ex_in.str()))
{
}

void stdin_line_reader::async_read_line_impl(std::string prompt,
    boost::asio::any_completion_handler<void(boost::system::error_code, std::string)> handler)
{
  ex_out << prompt;
  ex_out.flush();

  boost::asio::async_read_until(stdin_, boost::asio::dynamic_buffer(buffer_), '\n',
      boost::asio::deferred(
        [this](boost::system::error_code ec, std::size_t n)
        {
          if (!ec)
          {
            std::string result = buffer_.substr(0, n);
            buffer_.erase(0, n);
            return boost::asio::deferred.values(ec, std::move(result));
          }
          else
          {
            return boost::asio::deferred.values(ec, std::string{});
          }
        }
      )
    )(std::move(handler));
}

TEST_CASE("example: cpp11/type_erasure/main") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(line one
line two
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter something: line one
Enter something: line two
Enter something: )ex"));
}
}  // namespace ex_cpp11_type_erasure_main
#endif

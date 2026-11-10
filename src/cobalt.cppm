// mcpplibs.mwrap.boost.cobalt — module facade over generated/exports/cobalt.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
// boost/cobalt.hpp without boost/cobalt/main.hpp, which defines main
// (tools/export_policy.toml [cobalt]).
#include <boost/cobalt/async_for.hpp>
#include <boost/cobalt/channel.hpp>
#include <boost/cobalt/concepts.hpp>
#include <boost/cobalt/config.hpp>
#include <boost/cobalt/detached.hpp>
#include <boost/cobalt/error.hpp>
#include <boost/cobalt/gather.hpp>
#include <boost/cobalt/generator.hpp>
#include <boost/cobalt/join.hpp>
#include <boost/cobalt/op.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/cobalt/race.hpp>
#include <boost/cobalt/run.hpp>
#include <boost/cobalt/spawn.hpp>
#include <boost/cobalt/task.hpp>
#include <boost/cobalt/this_coro.hpp>
#include <boost/cobalt/this_thread.hpp>
#include <boost/cobalt/thread.hpp>
#include <boost/cobalt/wait_group.hpp>
#include <boost/cobalt/with.hpp>
// M11 platform guard (POSIX): asio reactor/hash_map details were only
// reached through windows-flavored chains in the mingw snapshot (POSIX
// cobalt selects the epoll reactor); include them explicitly so the
// shared-surface `using` lines resolve.
#include <boost/asio/detail/hash_map.hpp>
#include <boost/asio/detail/null_reactor.hpp>
#include <boost/asio/detail/select_reactor.hpp>
// fd_set_adapter / reactor_op_queue / socket_select_interrupter are
// per-OS-selected headers with no outer platform guard; the reactor
// headers above self-guard empty on the epoll/kqueue paths (their
// entities are guarded _WIN32 in the .inc instead).
#include <boost/asio/detail/fd_set_adapter.hpp>
#include <boost/asio/detail/reactor_op_queue.hpp>
#include <boost/asio/detail/socket_select_interrupter.hpp>
#include <boost/cobalt/composition.hpp>
#include <boost/cobalt/experimental/context.hpp>
#include <boost/cobalt/experimental/yield_context.hpp>
#include <boost/cobalt/io.hpp>

export module mcpplibs.mwrap.boost.cobalt;

export import mcpplibs.mwrap.boost.asio;
export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.callable_traits;
export import mcpplibs.mwrap.boost.circular_buffer;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.container;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.endian;
export import mcpplibs.mwrap.boost.intrusive;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.smart_ptr;
export import mcpplibs.mwrap.boost.static_string;
export import mcpplibs.mwrap.boost.system;
export import mcpplibs.mwrap.boost.throw_exception;
export import mcpplibs.mwrap.boost.utility;
export import mcpplibs.mwrap.boost.variant2;

#include "../generated/exports/cobalt.inc"


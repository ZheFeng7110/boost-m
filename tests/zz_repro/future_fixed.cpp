// Experiment: the same, with a throw of the type checked (never called) here.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.asio;

[[maybe_unused]] static void repro_sema_throw() { throw boost::system::system_error(boost::system::error_code()); }

#include "future_case.hpp"

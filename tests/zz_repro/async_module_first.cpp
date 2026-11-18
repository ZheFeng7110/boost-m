// Experiment: the asio module imported before std.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import mcpplibs.mwrap.boost.asio;
import std;

#include "async_case.hpp"

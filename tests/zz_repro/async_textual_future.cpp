// Experiment: <future> included textually before both imports.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <future>
import std;
import mcpplibs.mwrap.boost.asio;

#include "async_case.hpp"

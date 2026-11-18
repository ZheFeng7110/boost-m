// Experiment: std::async with the math module (whose GMF has <future>) only.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.math;

#include "async_case.hpp"

// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file support/lambda.hpp
/// \brief Placeholders definition to help in bimap modify function

#ifndef BOOST_BIMAP_SUPPORT_LAMBDA_HPP
#define BOOST_BIMAP_SUPPORT_LAMBDA_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/lambda/lambda.hpp>

namespace boost {
namespace bimaps {

/*
boost::lambda::placeholder1_type & _first  = boost::lambda::_1;
boost::lambda::placeholder2_type & _second = boost::lambda::_2;

boost::lambda::placeholder1_type & _left   = boost::lambda::_1;
boost::lambda::placeholder2_type & _right  = boost::lambda::_2;
*/

// boost-m vendored edit: the references lived in an anonymous namespace
// (internal linkage), so the bimap module could not export them; like
// boost::lambda::_1 (patches/lambda.patch) they become inline constexpr.
// Replay after re-running import_boost.
inline constexpr boost::lambda::placeholder1_type & _key  = boost::lambda::_1;
inline constexpr boost::lambda::placeholder1_type & _data = boost::lambda::_1;

} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_SUPPORT_LAMBDA_HPP


//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2015-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

// boost-m vendored edit: upstream reuses pmr/vector.hpp's include guard here,
// so a TU that includes devector.hpp first loses boost::container::pmr::vector
// (the container module's GMF did: pmr::vector / vector_of were never
// exported). Replay after re-running import_boost.
#ifndef BOOST_CONTAINER_PMR_DEVECTOR_HPP
#define BOOST_CONTAINER_PMR_DEVECTOR_HPP

#if defined (_MSC_VER)
#  pragma once 
#endif

#include <boost/container/devector.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>

namespace boost {
namespace container {
namespace pmr {

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

template <
  typename T,
  typename Options = void
>
using devector = boost::container::devector<T, polymorphic_allocator<T>, Options >;

#endif

//! A portable metafunction to obtain a vector
//! that uses a polymorphic allocator
template <
  typename T,
  typename Options = void
>
struct devector_of
{
   typedef boost::container::devector
      < T
      , polymorphic_allocator<T>
      , Options > type;
};

}  //namespace pmr {
}  //namespace container {
}  //namespace boost {

#endif   //BOOST_CONTAINER_PMR_DEVECTOR_HPP

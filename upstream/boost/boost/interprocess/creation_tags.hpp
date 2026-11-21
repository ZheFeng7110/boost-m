//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_CREATION_TAGS_HPP
#define BOOST_INTERPROCESS_CREATION_TAGS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

namespace boost {
namespace interprocess {

//!Tag to indicate that the resource must
//!be only created
struct create_only_t {};

//!Tag to indicate that the resource must
//!be only opened
struct open_only_t {};

//!Tag to indicate that the resource must
//!be only opened for reading
struct open_read_only_t {};

//!Tag to indicate that the resource must
//!be only opened privately for reading
struct open_read_private_t {};

//!Tag to indicate that the resource must
//!be only opened for reading
struct open_copy_on_write_t {};

//!Tag to indicate that the resource must
//!be created. If already created, it must be opened.
struct open_or_create_t {};

//!Value to indicate that the resource must
//!be only created
// boost-m vendored edit: BOOST_INLINE_CONSTEXPR (was static const: internal
// linkage, not exportable from a module).
BOOST_INLINE_CONSTEXPR create_only_t    create_only    = create_only_t();

//!Value to indicate that the resource must
//!be only opened
BOOST_INLINE_CONSTEXPR open_only_t      open_only      = open_only_t();

//!Value to indicate that the resource must
//!be only opened for reading
BOOST_INLINE_CONSTEXPR open_read_only_t open_read_only = open_read_only_t();

//!Value to indicate that the resource must
//!be created. If already created, it must be opened.
BOOST_INLINE_CONSTEXPR open_or_create_t open_or_create = open_or_create_t();

//!Value to indicate that the resource must
//!be only opened for reading
BOOST_INLINE_CONSTEXPR open_copy_on_write_t open_copy_on_write = open_copy_on_write_t();

namespace ipcdetail {

enum create_enum_t
{  DoCreate, DoOpen, DoOpenOrCreate   };

}  //namespace ipcdetail {

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_CREATION_TAGS_HPP


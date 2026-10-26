// M3 final form (derived from the tools/gen_exports.py draft; hand-finalized)
module;
#include <boost/range.hpp>
#include <boost/range/adaptor/define_adaptor.hpp>
#include <boost/range/adaptor/ref_unwrapped.hpp>
#include <boost/range/adaptor/type_erased.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm/swap_ranges.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/as_array.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/const_reverse_iterator.hpp>
#include <boost/range/counting_range.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/istream_range.hpp>
#include <boost/range/iterator_range_hash.hpp>
#include <boost/range/join.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/result_iterator.hpp>
#include <boost/range/reverse_result_iterator.hpp>

export module mcpplibs.mwrap.boost.range;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.tuple;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/range.inc"


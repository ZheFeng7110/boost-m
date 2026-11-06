// mcpplibs.mwrap.boost.sort — module facade over generated/exports/sort.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/sort/block_indirect_sort/blk_detail/constants.hpp>
#include <boost/sort/common/deque_cnc.hpp>
#include <boost/sort/common/file_vector.hpp>
#include <boost/sort/common/int_array.hpp>
#include <boost/sort/common/rearrange.hpp>
#include <boost/sort/common/scheduler.hpp>
#include <boost/sort/common/time_measure.hpp>
#include <boost/sort/insert_sort/insert_sort.hpp>
#include <boost/sort/sort.hpp>

export module mcpplibs.mwrap.boost.sort;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/sort.inc"


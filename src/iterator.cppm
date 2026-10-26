// M3 final form (derived from the tools/gen_exports.py draft; hand-finalized)
module;
#include <boost/iterator.hpp>
#include <boost/iterator/advance.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/distance.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/function_input_iterator.hpp>
#include <boost/iterator/function_output_iterator.hpp>
#include <boost/iterator/generator_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/is_iterator.hpp>
#include <boost/iterator/iterator_archetypes.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/iterator/minimum_category.hpp>
#include <boost/iterator/new_iterator_tests.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/iterator/shared_container_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/zip_iterator.hpp>

export module mcpplibs.mwrap.boost.iterator;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/iterator.inc"


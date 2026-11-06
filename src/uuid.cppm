// mcpplibs.mwrap.boost.uuid — module facade over generated/exports/uuid.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/uuid.hpp>
#include <boost/uuid/basic_name_generator.hpp>
#include <boost/uuid/entropy_error.hpp>
#include <boost/uuid/invalid_uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/uuid/uuid_serialize.hpp>

export module mcpplibs.mwrap.boost.uuid;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.throw_exception;

#include "../generated/exports/uuid.inc"


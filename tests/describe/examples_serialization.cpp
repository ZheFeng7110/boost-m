// boost.describe with the serialization module — libs/describe/example/
// serialization.cpp: described classes serialized through a generic
// serialize() written with describe_members, archives from the module.
// Output must equal the original program's.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/describe.hpp>
#include <cassert>
#include <boost/config.hpp>
import std;
import mcpplibs.mwrap.boost.serialization;
import mcpplibs.mwrap.boost.mp11;
import mcpplibs.mwrap.boost.core;

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\n';
    return 0;
}

// libs/describe/example/serialization.cpp
namespace ex_serialization {
#define _CRT_SECURE_NO_WARNINGS


namespace app
{

template<class Archive, class T,
    class D1 = boost::describe::describe_bases<T, boost::describe::mod_public>,
    class D2 = boost::describe::describe_bases<T,
        boost::describe::mod_protected | boost::describe::mod_private>,
    class D3 = boost::describe::describe_members<T,
        boost::describe::mod_public | boost::describe::mod_protected>,
    class D4 = boost::describe::describe_members<T, boost::describe::mod_private>,
    class En = std::enable_if_t< boost::mp11::mp_empty<D2>::value &&
        boost::mp11::mp_empty<D4>::value && !std::is_union<T>::value> >

    void serialize( Archive & ar, T & t, boost::serialization::version_type )
{
    int k = 0;

    // public bases: use base_object

    boost::mp11::mp_for_each<D1>([&](auto D){

        using B = typename decltype(D)::type;

        char name[ 32 ];
        std::sprintf( name, "base.%d", ++k );

        ar & boost::make_nvp( name, boost::serialization::base_object<B>( t ) );

    });

    // public (and protected) members

    boost::mp11::mp_for_each<D3>([&](auto D){

        ar & boost::make_nvp( D.name, t.*D.pointer );

    });
}

struct A1
{
    int x;
};

BOOST_DESCRIBE_STRUCT(A1, (), (x))

struct A2
{
    int y;
};

BOOST_DESCRIBE_STRUCT(A2, (), (y))

struct B: public A1, public A2
{
    // these constructors aren't needed in C++17
    B(): A1(), A2() {}
    B( int x, int y ): A1{ x }, A2{ y } {}
};

BOOST_DESCRIBE_STRUCT(B, (A1, A2), ())

struct C
{
    std::vector<B> v;
};

BOOST_DESCRIBE_STRUCT(C, (), (v))

} // namespace app


int example_main()
{
    app::C c1{ { { 1, 2 }, { 3, 4 }, { 5, 6 } } };

    std::ostringstream os;

    {
        boost::archive::xml_oarchive ar( os );
        ar << boost::make_nvp( "c1", c1 );
    }

    std::string s = os.str();

    ex_out << s << std::endl;

    app::C c2;

    {
        std::istringstream is( s );
        boost::archive::xml_iarchive ar( is );
        ar >> boost::make_nvp( "c2", c2 );
    }

    {
        boost::archive::text_oarchive ar( ex_out );
        ar << c2;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: serialization") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<!DOCTYPE boost_serialization>
<boost_serialization signature="serialization::archive" version="20">
<c1 class_id="0" tracking_level="0" version="0">
	<v class_id="1" tracking_level="0" version="0">
		<count>3</count>
		<item_version>0</item_version>
		<item class_id="2" tracking_level="0" version="0">
			<base.1 class_id="3" tracking_level="0" version="0">
				<x>1</x>
			</base.1>
			<base.2 class_id="4" tracking_level="0" version="0">
				<y>2</y>
			</base.2>
		</item>
		<item>
			<base.1>
				<x>3</x>
			</base.1>
			<base.2>
				<y>4</y>
			</base.2>
		</item>
		<item>
			<base.1>
				<x>5</x>
			</base.1>
			<base.2>
				<y>6</y>
			</base.2>
		</item>
	</v>
</c1>
</boost_serialization>


22 serialization::archive 20 0 0 0 0 3 0 0 0 0 0 1 0 0 2 3 4 5 6
)ex"));
}
}  // namespace ex_serialization

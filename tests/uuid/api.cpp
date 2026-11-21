// mcpplibs.mwrap.boost.uuid — every documented component of Boost.UUID
// (libs/uuid/doc/uuid/*.adoc): the uuid class (construction, iteration, data,
// size, is_nil, variant, version, time-based fields, swap, operators, hashing,
// serialization), the generators (nil, string, name md5/sha1, random,
// basic_random, time v1/v6/v7), uuid_clock, namespaces, constants and the I/O
// functions. Cases are "doc: <page>#<section>".
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.uuid;
import mcpplibs.mwrap.boost.lexical_cast;
import mcpplibs.mwrap.boost.random;
import mcpplibs.mwrap.boost.serialization;

using namespace boost::uuids;

namespace {
constexpr uuid sample = uuid_from_string("01234567-89ab-cdef-0123-456789abcdef");
std::string str(const uuid& u) { return to_string(u); }
}  // namespace

// ---- examples ---------------------------------------------------------------

namespace {
class object {
public:
    object() : tag(random_generator()()), state(0) {}
    explicit object(int state) : tag(random_generator()()), state(state) {}
    object(object const& rhs) = default;
    bool operator==(object const& rhs) const { return tag == rhs.tag; }
    object& operator=(object const& rhs) = default;
    int get_state() const { return state; }
    void set_state(int new_state) { state = new_state; }
private:
    uuid tag;
    int state;
};
}  // namespace

TEST_CASE("doc: examples#examples") {
    object o1(1);
    object o2 = o1;
    o2.set_state(2);
    CHECK(o1 == o2);
    object o3(3);
    CHECK(o1 != o3);
    CHECK(o2 != o3);
}

TEST_CASE("doc: examples#byte-extraction") {
    uuid u = sample;
    std::vector<std::uint8_t> v(u.size());
    std::copy(u.begin(), u.end(), v.begin());
    CHECK(v.front() == 0x01);
    CHECK(v.back() == 0xef);
}

// ---- uuid ---------------------------------------------------------------------

TEST_CASE("doc: uuid#constructors") {
    constexpr uuid nil;
    static_assert(nil.is_nil());
    constexpr std::uint8_t bytes[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    constexpr uuid u(bytes);
    static_assert(*u.begin() == 1);
    CHECK(str(u) == "01020304-0506-0708-090a-0b0c0d0e0f10");
}
TEST_CASE("doc: uuid#iteration") {
    uuid u;
    std::iota(u.begin(), u.end(), std::uint8_t(0));
    CHECK(std::distance(u.begin(), u.end()) == 16);
    CHECK(*(u.end() - 1) == 15);
}
TEST_CASE("doc: uuid#data") {
    uuid u = sample;
    CHECK(u.data()[0] == 0x01);
    u.data()[0] = 0xff;
    CHECK(str(u).substr(0, 2) == "ff");
}
TEST_CASE("doc: uuid#size") {
    static_assert(uuid::static_size() == 16);
    CHECK(sample.size() == 16);
}
TEST_CASE("doc: uuid#is-nil") {
    CHECK(uuid().is_nil());
    CHECK_FALSE(sample.is_nil());
}
TEST_CASE("doc: uuid#variant") {
    CHECK(random_generator()().variant() == uuid::variant_rfc_4122);
    uuid u;
    u.data()[8] = 0x00;
    CHECK(u.variant() == uuid::variant_ncs);
}
TEST_CASE("doc: uuid#version") {
    CHECK(random_generator()().version() == uuid::version_random_number_based);
    CHECK(name_generator_md5(ns::dns())("x").version() == uuid::version_name_based_md5);
    CHECK(name_generator_sha1(ns::dns())("x").version() == uuid::version_name_based_sha1);
    CHECK(time_generator_v1()().version() == uuid::version_time_based);
    CHECK(time_generator_v6()().version() == uuid::version_time_based_v6);
    CHECK(time_generator_v7()().version() == uuid::version_time_based_v7);
    CHECK(uuid().version() == uuid::version_unknown);
}
TEST_CASE("doc: uuid#time-based-fields") {
    uuid u1 = time_generator_v1()();
    CHECK(u1.timestamp_v1() == uuid_clock::to_timestamp(u1.time_point_v1()));
    uuid u6 = time_generator_v6()();
    CHECK(u6.timestamp_v6() == uuid_clock::to_timestamp(u6.time_point_v6()));
    uuid u7 = time_generator_v7()();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    auto t7 = std::chrono::duration_cast<std::chrono::milliseconds>(u7.time_point_v7().time_since_epoch());
    CHECK(std::abs((ms - t7).count()) < 60000);
    static_assert(std::is_same_v<uuid::node_type, std::array<std::uint8_t, 6>>);
    CHECK(u1.node_identifier().size() == 6);
    (void)u1.clock_seq();
}
TEST_CASE("doc: uuid#swap") {
    uuid a = sample, b;
    a.swap(b);
    CHECK(a.is_nil());
    CHECK(b == sample);
}
TEST_CASE("doc: uuid#free-swap") {
    uuid a = sample, b;
    swap(a, b);
    CHECK(b == sample);
}
TEST_CASE("doc: uuid#operators") {
    uuid a = sample, b = uuid_from_string("ffffffff-ffff-ffff-ffff-ffffffffffff");
    CHECK(a < b);
    CHECK(a <= b);
    CHECK(b > a);
    CHECK(b >= a);
    CHECK(a != b);
    CHECK((a <=> b) == std::strong_ordering::less);
    std::set<uuid> s{b, a};
    CHECK(*s.begin() == a);
}
TEST_CASE("doc: uuid#hash-value") {
    CHECK(hash_value(sample) == hash_value(uuid(sample)));
    CHECK(hash_value(sample) != hash_value(uuid()));
}
TEST_CASE("doc: uuid#std-hash") {
    std::unordered_set<uuid> s{sample, uuid()};
    CHECK(s.count(sample) == 1);
    CHECK(std::hash<uuid>()(sample) == std::hash<uuid>()(sample));
}
TEST_CASE("doc: uuid#serialization") {
    std::stringstream ss;
    {
        boost::archive::text_oarchive oa(ss);
        uuid u = sample;
        oa << u;
    }
    uuid back;
    {
        boost::archive::text_iarchive ia(ss);
        ia >> back;
    }
    CHECK(back == sample);
}

// ---- I/O ------------------------------------------------------------------------

TEST_CASE("doc: uuid_io#stream-insertion") {
    uuid u1 = random_generator()();
    std::ostringstream os;
    os << u1;
    std::string s1 = boost::lexical_cast<std::string>(u1);
    CHECK(os.str() == s1);
    CHECK(s1.size() == 36);
    std::wostringstream wos;
    wos << sample;
    CHECK(wos.str() == L"01234567-89ab-cdef-0123-456789abcdef");
}
TEST_CASE("doc: uuid_io#stream-extraction") {
    uuid u1 = random_generator()();
    std::stringstream ss;
    ss << u1;
    uuid u2 = boost::lexical_cast<uuid>(ss.str());
    CHECK(u1 == u2);
    std::istringstream bad("not-a-uuid");
    uuid u3;
    bad >> u3;
    CHECK(bad.fail());
}
TEST_CASE("doc: uuid_io#to-chars") {
    char buf[37];
    char* end = to_chars(sample, buf);
    CHECK(end == buf + 36);
    CHECK(std::string(buf) == "01234567-89ab-cdef-0123-456789abcdef");  // the array form terminates
    char small[10];
    CHECK_FALSE(to_chars(sample, small, small + 10));
    std::string out;
    to_chars(sample, std::back_inserter(out));
    CHECK(out.size() == 36);
    static_assert([] {
        char b[37]{};
        to_chars(sample, b);
        return b[8] == '-';
    }());
}
TEST_CASE("doc: uuid_io#to-string") {
    CHECK(to_string(sample) == "01234567-89ab-cdef-0123-456789abcdef");
    CHECK(to_wstring(sample) == L"01234567-89ab-cdef-0123-456789abcdef");
}
TEST_CASE("doc: uuid_io#from-chars") {
    std::string_view s = "01234567-89ab-cdef-0123-456789abcdef";
    uuid u;
    auto r = from_chars(s.data(), s.data() + s.size(), u);
    CHECK(static_cast<bool>(r));
    CHECK(u == sample);
    std::string_view bad = "01234567-89ab-cdef-0123-456789abcdeX";
    auto r2 = from_chars(bad.data(), bad.data() + bad.size(), u);
    CHECK(r2.ec == from_chars_error::hex_digit_expected);
}
TEST_CASE("doc: uuid_io#from-chars-result") {
    std::string_view s = "0123456789abcdef0123456789abcdef";  // no dashes
    uuid u;
    auto r = from_chars(s.data(), s.data() + s.size(), u);
    CHECK(r.ec == from_chars_error::dash_expected);
    CHECK_FALSE(static_cast<bool>(r));
    std::string_view tail = "01234567-89ab-cdef-0123-456789abcdefXX";
    auto r2 = from_chars(tail.data(), tail.data() + tail.size(), u);
    CHECK(r2.ec == from_chars_error::none);
    CHECK(r2.ptr == tail.data() + 36);
}
TEST_CASE("doc: uuid_io#uuid-from-string") {
    constexpr uuid u = uuid_from_string("01234567-89ab-cdef-0123-456789abcdef");
    static_assert(u.data()[1] == 0x23);
    CHECK(uuid_from_string(std::string("01234567-89ab-cdef-0123-456789abcdef")) == sample);
    CHECK_THROWS_AS(uuid_from_string("xyz"), std::runtime_error);
}

// ---- generators -------------------------------------------------------------------

TEST_CASE("doc: nil_generator#nil-generator") {
    nil_generator gen;
    CHECK(gen().is_nil());
}
TEST_CASE("doc: nil_generator#nil-uuid") {
    constexpr uuid u = nil_uuid();
    static_assert(u.is_nil());
}
TEST_CASE("doc: string_generator#string-generator") {
    string_generator gen;
    uuid u1 = gen(std::string("0123456789abcdef0123456789abcdef"));
    uuid u2 = gen(std::wstring(L"01234567-89AB-CDEF-0123-456789ABCDEF"));
    CHECK(u1 == u2);
    uuid u3 = gen("{01234567-89ab-cdef-0123-456789abcdef}");
    uuid u4 = gen(L"01234567-89ab-cdef-0123-456789abcdef");
    CHECK(u3 == u4);
    CHECK(u1 == u3);
    std::string text = "{0123456789abcdef0123456789abcdef}";
    CHECK(gen(text.begin(), text.end()) == u1);
    CHECK_THROWS_AS(gen("0123"), std::runtime_error);
}
TEST_CASE("doc: name_generator_md5#name-generator-md5") {
    name_generator_md5 gen(ns::dns());
    uuid u = gen("boost.org");
    CHECK(u.version() == uuid::version_name_based_md5);
    CHECK(gen("boost.org") == u);  // deterministic
    CHECK(gen(std::string("boost.org")) == u);
    CHECK(gen("boost.org", 9) == u);  // buffer + byte count
}
TEST_CASE("doc: name_generator_sha1#name-generator-sha1") {
    name_generator_sha1 gen(ns::dns());
    CHECK(str(gen("boost.org")) == "0043f363-bbb4-5369-840a-322df6ec1926");
    CHECK(str(gen(L"boost.org")) == "c31c5016-3493-5dc2-8484-5813d495cc18");
    CHECK(str(gen(u"boost.org")) == "0043f363-bbb4-5369-840a-322df6ec1926");
    static_assert(std::is_same_v<name_generator, name_generator_sha1>);
    CHECK(name_generator_latest(ns::url())("x").version() >= uuid::version_name_based_sha1);
}
TEST_CASE("doc: namespaces#namespaces") {
    CHECK(str(ns::dns()) == "6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    CHECK(str(ns::url()) == "6ba7b811-9dad-11d1-80b4-00c04fd430c8");
    CHECK(str(ns::oid()) == "6ba7b812-9dad-11d1-80b4-00c04fd430c8");
    CHECK(str(ns::x500dn()) == "6ba7b814-9dad-11d1-80b4-00c04fd430c8");
}
TEST_CASE("doc: random_generator#random-generator") {
    random_generator gen;
    uuid a = gen(), b = gen();
    CHECK(a != b);
    CHECK(a.version() == uuid::version_random_number_based);
    random_generator_mt19937 mt;
    CHECK(mt().variant() == uuid::variant_rfc_4122);
    random_generator_pure pure;
    CHECK_FALSE(pure().is_nil());
}
TEST_CASE("doc: basic_random_generator#basic-random-generator") {
    basic_random_generator<boost::mt19937> bulkgen;
    std::set<uuid> seen;
    for (int i = 0; i < 1000; ++i) seen.insert(bulkgen());
    CHECK(seen.size() == 1000);
    boost::mt19937 engine(42), engine2(42);
    basic_random_generator<boost::mt19937> by_ref(engine), by_ptr(&engine2);
    CHECK(by_ref() == by_ptr());  // same seed, same sequence
}
TEST_CASE("doc: time_generator_v1#constructors") {
    time_generator_v1 gen;
    std::array<std::uint8_t, 6> node{1, 2, 3, 4, 5, 6};
    time_generator_v1::state_type state{0, 0x1234};
    time_generator_v1 fixed(node, state);
    uuid u = fixed();
    CHECK(u.node_identifier() == node);
    (void)gen();
}
TEST_CASE("doc: time_generator_v1#operator") {
    time_generator_v1 gen;
    uuid a = gen(), b = gen();
    CHECK(a != b);
    CHECK(a.timestamp_v1() <= b.timestamp_v1());
}
TEST_CASE("doc: time_generator_v7#constructors") {
    time_generator_v7 a;
    time_generator_v7 b(a);
    time_generator_v7 c(std::move(b));
    CHECK(c().version() == uuid::version_time_based_v7);
}
TEST_CASE("doc: time_generator_v7#assignment") {
    time_generator_v7 a, b;
    b = a;
    b = std::move(a);
    CHECK_FALSE(b().is_nil());
}
TEST_CASE("doc: time_generator_v7#operator") {
    time_generator_v7 gen;
    uuid prev = gen();
    for (int i = 0; i < 100; ++i) {
        uuid next = gen();
        CHECK(prev < next);  // v7 values sort by creation time
        prev = next;
    }
}
TEST_CASE("doc: uuid_clock#now") {
    auto t = uuid_clock::now();
    CHECK(t.time_since_epoch().count() > 0);
    static_assert(!uuid_clock::is_steady);
}
TEST_CASE("doc: uuid_clock#from-sys") {
    auto sys = std::chrono::system_clock::now();
    auto u = uuid_clock::from_sys(sys);
    auto back = uuid_clock::to_sys(u);
    CHECK(std::chrono::abs(back - std::chrono::time_point_cast<uuid_clock::duration>(sys)).count() == 0);
}
TEST_CASE("doc: uuid_clock#to-sys") {
    // The Gregorian epoch of RFC 4122 timestamps is 1582-10-15.
    auto epoch = uuid_clock::to_sys(uuid_clock::time_point{});
    auto days = std::chrono::floor<std::chrono::days>(epoch);
    CHECK(std::chrono::year_month_day(days) == std::chrono::year{1582} / std::chrono::October / 15);
}
TEST_CASE("doc: uuid_clock#from-timestamp") {
    auto tp = uuid_clock::from_timestamp(12345);
    CHECK(tp.time_since_epoch().count() == 12345);
}
TEST_CASE("doc: uuid_clock#to-timestamp") {
    uuid u = time_generator_v1()();
    CHECK(u.timestamp_v1() == uuid_clock::to_timestamp(u.time_point_v1()));
}
TEST_CASE("doc: constants#constants") {
    // <boost/uuid/constants.hpp>: nil_uuid() and the namespaces.
    CHECK(nil_uuid().is_nil());
    CHECK(nil_uuid() == uuid());
    CHECK_FALSE(ns::dns().is_nil());
}
TEST_CASE("doc: invalid_uuid#invalid-uuid") {
    try {
        (void)uuid_from_string("nope");
        FAIL("expected an exception");
    } catch (const std::runtime_error& e) {
        CHECK(std::string(e.what()).size() > 0);
    }
    static_assert(std::is_base_of_v<std::runtime_error, invalid_uuid>);
    try {
        (void)uuid_from_string("01234567-89ab-cdef-0123-456789abcdeX");
    } catch (const invalid_uuid& e) {
        CHECK(e.position() == 35);
        CHECK(e.error() == from_chars_error::hex_digit_expected);
    }
    invalid_uuid made(3, from_chars_error::dash_expected);
    CHECK(made.position() == 3);
}

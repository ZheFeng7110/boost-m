// mcpplibs.mwrap.boost.crc — user-chosen CRC parameters and user data:
// crc_optimal / crc_basic instantiated with parameters the library does not
// predefine (CRC-8, CRC-32C, CRC-64/XZ from the CRC catalogue), optimal and
// basic agreeing on them, and CRCs over user-defined record types.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.crc;

namespace user {
using crc8 = boost::crc_optimal<8, 0x07, 0, 0, false, false>;
using crc32c = boost::crc_optimal<32, 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true>;
using crc64_xz = boost::crc_optimal<64, 0x42F0E1EBA9EA3693ull, ~0ull, ~0ull, true, true>;

struct record {
    std::uint32_t id;
    std::uint16_t flags;
    char name[6];
};

template <class Crc>
auto checksum_of(const record& r) {
    Crc c;
    c.process_bytes(&r, sizeof r);
    return c.checksum();
}

// A user byte sink that feeds a CRC computer through its function-call form.
template <class Crc>
class crc_sink {
public:
    void write(std::span<const std::byte> bytes) {
        for (std::byte b : bytes) crc_(static_cast<unsigned char>(b));
    }
    auto value() const { return crc_.checksum(); }
private:
    Crc crc_;
};
}  // namespace user

TEST_CASE("catalogue parameters the library does not predefine") {
    const char data[] = "123456789";
    user::crc8 c8;
    c8.process_bytes(data, 9);
    CHECK(c8.checksum() == 0xF4u);
    user::crc32c c32;
    c32.process_bytes(data, 9);
    CHECK(c32.checksum() == 0xE3069283u);
    user::crc64_xz c64;
    c64.process_bytes(data, 9);
    CHECK(c64.checksum() == 0x995DC9BBDF1939FAull);
}

TEST_CASE("crc_basic agrees with crc_optimal on user parameters") {
    const char data[] = "The quick brown fox";
    boost::crc_basic<32> basic(0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true);
    basic.process_bytes(data, sizeof data - 1);
    user::crc32c optimal;
    optimal.process_bytes(data, sizeof data - 1);
    CHECK(basic.checksum() == optimal.checksum());
    CHECK(boost::crc<32, 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true>(data, sizeof data - 1) ==
          optimal.checksum());
}

TEST_CASE("CRCs over user record types and a user sink") {
    user::record a{7, 1, "alpha"};
    user::record b = a;
    CHECK(user::checksum_of<user::crc32c>(a) == user::checksum_of<user::crc32c>(b));
    b.flags = 2;
    CHECK(user::checksum_of<user::crc32c>(a) != user::checksum_of<user::crc32c>(b));
    user::crc_sink<boost::crc_32_type> sink;
    const std::string text = "123456789";
    sink.write(std::as_bytes(std::span(text)));
    CHECK(sink.value() == 0xCBF43926u);
}

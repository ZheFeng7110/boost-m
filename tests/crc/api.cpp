// mcpplibs.mwrap.boost.crc — the documented interface: boost::crc(),
// augmented_crc(), crc_basic (theoretical computer), crc_optimal (table
// driven), the predefined samples checked against the CRC catalogue values for
// "123456789", and the member functions of both computer templates.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.crc;

namespace {
const char check_data[] = "123456789";
constexpr std::size_t check_len = 9;
}  // namespace

TEST_CASE("doc: crc_function") {
    CHECK(boost::crc<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true>(check_data, check_len) ==
          0xCBF43926u);
    CHECK(boost::crc<16, 0x8005, 0, 0, true, true>(check_data, check_len) == 0xBB3Du);
    CHECK(boost::crc<16, 0x1021, 0xFFFF, 0, false, false>(check_data, check_len) == 0x29B1u);
}

TEST_CASE("doc: acrc_function") {
    // An augmented CRC appends Bits zero bits to the message: the remainder of
    // message + its own CRC is zero.
    unsigned char msg[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 0, 0};
    auto c = boost::augmented_crc<16, 0x1021u>(msg, sizeof msg);
    CHECK(c == boost::crc<16, 0x1021, 0, 0, false, false>(check_data, check_len));
    msg[9] = static_cast<unsigned char>(c >> 8);
    msg[10] = static_cast<unsigned char>(c & 0xFF);
    CHECK(boost::augmented_crc<16, 0x1021u>(msg, sizeof msg) == 0u);
}

TEST_CASE("doc: crc_basic") {
    boost::crc_basic<16> c(0x1021, 0xFFFF, 0, false, false);
    CHECK(c.get_truncated_polynominal() == 0x1021);
    CHECK(c.get_initial_remainder() == 0xFFFF);
    CHECK(c.get_final_xor_value() == 0);
    CHECK_FALSE(c.get_reflect_input());
    CHECK_FALSE(c.get_reflect_remainder());
    c.process_bytes(check_data, check_len);
    CHECK(c.checksum() == 0x29B1u);
    c.reset();
    CHECK(c.get_interim_remainder() == 0xFFFF);
    c.process_block(check_data, check_data + check_len);
    CHECK(c.checksum() == 0x29B1u);
    c.reset(0);
    for (char ch : std::string_view(check_data, check_len))
        c.process_byte(static_cast<unsigned char>(ch));
    CHECK(c.checksum() == 0x31C3u);  // initial remainder 0: XMODEM
    boost::crc_basic<16> bits(0x1021, 0xFFFF, 0, false, false);
    bits.process_bits(0xA5, 8);
    bits.process_bit(true);
    CHECK(bits.get_interim_remainder() != 0xFFFF);
}

TEST_CASE("doc: crc_optimal") {
    boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> c;
    static_assert(decltype(c)::bit_count == 32);
    CHECK(c.get_truncated_polynominal() == 0x04C11DB7u);
    CHECK(c.get_reflect_input());
    c = std::for_each(check_data, check_data + check_len, c);
    CHECK(c() == 0xCBF43926u);
    c.reset();
    c.process_bytes(check_data, 4);
    c.process_block(check_data + 4, check_data + check_len);
    CHECK(c.checksum() == 0xCBF43926u);
    c.reset();
    c(static_cast<unsigned char>('1'));
    c.process_byte(static_cast<unsigned char>('2'));
    CHECK(c.get_interim_remainder() != 0xFFFFFFFFu);
}

TEST_CASE("doc: crc_samples") {
    auto run = [](auto computer) {
        computer.process_bytes(check_data, check_len);
        return static_cast<unsigned long>(computer.checksum());
    };
    CHECK(run(boost::crc_16_type()) == 0xBB3Du);
    CHECK(run(boost::crc_ccitt_false_t()) == 0x29B1u);
    CHECK(run(boost::crc_ccitt_type()) == 0x29B1u);
    CHECK(run(boost::crc_ccitt_true_t()) == 0x2189u);
    CHECK(run(boost::crc_xmodem_t()) == 0x31C3u);
    CHECK(run(boost::crc_xmodem_type()) == run(boost::crc_optimal<16, 0x8408, 0, 0, true, true>()));
    CHECK(run(boost::crc_32_type()) == 0xCBF43926u);
}

// mcpplibs.mwrap.boost.crc — the official examples of Boost.CRC as test
// cases: libs/crc/crc_example.cpp (CRC-32 of files, read in blocks) and the
// three snippets of libs/crc/doc/crc_examples.cpp. <cstdarg> is included for
// the va_* macros, which no module can export.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdarg>
import std;
import mcpplibs.mwrap.boost.crc;

TEST_CASE("example: crc_example") {
    const auto dir = std::filesystem::temp_directory_path() /
                     ("boost-m-crc-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directories(dir);
    const auto a = dir / "a.bin", b = dir / "b.bin";
    std::ofstream(a, std::ios::binary) << "12345";
    std::ofstream(b, std::ios::binary) << "6789";
    constexpr std::streamsize buffer_size = 4;  // smaller than a file: several reads
    boost::crc_32_type result;
    for (const auto& path : {a, b}) {
        std::ifstream ifs(path, std::ios_base::binary);
        REQUIRE(ifs);
        do {
            char buffer[buffer_size];
            ifs.read(buffer, buffer_size);
            result.process_bytes(buffer, static_cast<std::size_t>(ifs.gcount()));
        } while (ifs);
    }
    std::ostringstream out;
    out << std::hex << std::uppercase << result.checksum();
    CHECK(out.str() == "CBF43926");  // CRC-32 of "123456789"
    std::filesystem::remove_all(dir);
}

namespace {
std::pair<unsigned, unsigned> crc_16_and_xmodem(void const* b, std::size_t l) {
    std::pair<unsigned, unsigned> result;
    boost::crc_basic<16> crc1(0x8005u, 0u, 0u, true, true);
    crc1.process_bytes(b, l);
    result.first = crc1.checksum();
    crc1 = boost::crc_basic<16>(0x8408u, crc1.get_initial_remainder(),
                                crc1.get_final_xor_value(), crc1.get_reflect_input(),
                                crc1.get_reflect_remainder());
    crc1.process_bytes(b, l);
    result.second = crc1.checksum();
    return result;
}

unsigned combined_crc_16(unsigned block_count, ...) {
    boost::crc_basic<16> crc1(0x8005u, 0u, 0u, true, true);
    va_list ap;
    va_start(ap, block_count);
    while (block_count--) {
        void const* const bs = va_arg(ap, void const*);
        std::size_t const bl = va_arg(ap, std::size_t);
        crc1.process_bytes(bs, bl);
    }
    va_end(ap);
    return crc1.checksum();
}

unsigned combined_acrc_16(int block_count, ...) {
    va_list ap;
    unsigned result = 0u;
    va_start(ap, block_count);
    if (block_count > 0) {
        void const* bs = va_arg(ap, void const*);
        std::size_t bl = va_arg(ap, std::size_t);
        result = boost::augmented_crc<16, 0x1021u>(bs, bl);
        while (--block_count) {
            bs = va_arg(ap, void const*);
            bl = va_arg(ap, std::size_t);
            result = boost::augmented_crc<16, 0x1021u>(bs, bl, result);
        }
    }
    va_end(ap);
    return result;
}
}  // namespace

TEST_CASE("example: crc_examples") {
    static const char data[] = "123456789";
    // crc_basic_reuse: one object re-parameterized by assignment.
    auto r = crc_16_and_xmodem(data, 9);
    boost::crc_16_type crc16;
    crc16.process_bytes(data, 9);
    CHECK(r.first == crc16.checksum());
    boost::crc_xmodem_type xm;
    xm.process_bytes(data, 9);
    CHECK(r.second == xm.checksum());
    // crc_piecemeal_run: the data does not have to be in one block.
    CHECK(combined_crc_16(3u, static_cast<void const*>(data), std::size_t{2},
                          static_cast<void const*>(data + 2), std::size_t{3},
                          static_cast<void const*>(data + 5), std::size_t{4}) == crc16.checksum());
    // acrc_piecemeal_run: augmented_crc chained through its initial remainder.
    unsigned whole = boost::augmented_crc<16, 0x1021u>(data, 9);
    CHECK(combined_acrc_16(2, static_cast<void const*>(data), std::size_t{4},
                           static_cast<void const*>(data + 4), std::size_t{5}) == whole);
    CHECK(combined_acrc_16(0) == 0u);
}

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <catch2/catch_test_macros.hpp>

#include <Sts1CobcSw/Blake2s/BLAKE2s.h>

#include <array>
#include <bit>


using sts1cobcsw::Byte;
using sts1cobcsw::operator""_b;


TEST_CASE("Blake2s")  // NOLINT(cert-err58-cpp)
{
    auto blake = BLAKE2s();
    static constexpr auto key = std::array<Byte, 4>{};
    auto hash = std::array<Byte, 8>{};
    {
        auto data = std::array<Byte, 1>{};
        blake.reset(key.data(), key.size(), hash.size());
        blake.update(data.data(), data.size());
        blake.finalize(hash.data(), hash.size());
        auto expected = sts1cobcsw::Serialize<std::endian::big>(0xd25e7043bc976e5d);
        CHECK(hash == expected);
    }
    {
        auto data = std::array{0x01_b, 0x02_b, 0x03_b, 0x04_b};
        blake.reset(key.data(), key.size(), hash.size());
        blake.update(data.data(), data.size());
        blake.finalize(hash.data(), hash.size());
        auto expected = sts1cobcsw::Serialize<std::endian::big>(0x93fc139248ab69b5);
        CHECK(hash == expected);
    }
}

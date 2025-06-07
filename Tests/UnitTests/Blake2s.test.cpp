#include <Sts1CobcSw/Blake2s/Blake2s.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>


namespace blake2s = sts1cobcsw::blake2s;

using sts1cobcsw::Byte;
using sts1cobcsw::operator""_b;


TEST_CASE("Blake2s")  // NOLINT(cert-err58-cpp)
{
    {
        auto data = std::array<Byte, 1>{};
        auto hash = blake2s::ComputeHash(data);
        auto expected = sts1cobcsw::Serialize<std::endian::big>(0xd25e7043bc976e5d);
        CHECK(hash == expected);
    }
    {
        auto data = std::array{0x01_b, 0x02_b, 0x03_b, 0x04_b};
        auto hash = blake2s::ComputeHash(data);
        auto expected = sts1cobcsw::Serialize<std::endian::big>(0x93fc139248ab69b5);
        CHECK(hash == expected);
    }
}

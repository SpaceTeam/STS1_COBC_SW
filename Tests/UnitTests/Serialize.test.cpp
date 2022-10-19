#include <Sts1CobcSw/Serialize/Serialize.hpp>

#include <catch2/catch_test_macros.hpp>
#include <type_safe/types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>


namespace ts = type_safe;

using sts1cobcsw::serialize::Serialize;


TEST_CASE("TriviallySerializable")
{
    using sts1cobcsw::serialize::TriviallySerializable;

    struct EmptyStruct
    {
    };
    struct SingleInt32
    {
        std::int32_t i;
    };

    // POD types are TriviallySerializable
    REQUIRE(TriviallySerializable<std::byte>);
    REQUIRE(TriviallySerializable<char>);
    REQUIRE(TriviallySerializable<unsigned char>);
    REQUIRE(TriviallySerializable<short>);
    REQUIRE(TriviallySerializable<unsigned short>);
    REQUIRE(TriviallySerializable<int>);
    REQUIRE(TriviallySerializable<unsigned int>);
    REQUIRE(TriviallySerializable<long>);
    REQUIRE(TriviallySerializable<unsigned long>);
    REQUIRE(TriviallySerializable<float>);
    REQUIRE(TriviallySerializable<double>);
    // So are type_safe integer and bool types
    REQUIRE(TriviallySerializable<ts::int8_t>);
    REQUIRE(TriviallySerializable<ts::uint16_t>);
    REQUIRE(TriviallySerializable<ts::size_t>);
    REQUIRE(TriviallySerializable<ts::bool_t>);
    // Pointers and arrays are not TriviallySerializable
    REQUIRE(not TriviallySerializable<char *>);
    REQUIRE(not TriviallySerializable<int[]>);  // NOLINT
    REQUIRE(not TriviallySerializable<std::array<double, 3>>);
    // User-defined types aren't either
    REQUIRE(not TriviallySerializable<EmptyStruct>);
    REQUIRE(not TriviallySerializable<SingleInt32>);
}


TEST_CASE("Serialize TriviallySerializable types")
{
    using ts::operator""_i8;
    using ts::operator""_u16;

    auto byteBuffer = Serialize(std::byte{0xAA});
    auto int8Buffer = Serialize(-4_i8);
    auto uint16Buffer = Serialize(11_u16);
    auto int32Buffer = Serialize(std::int32_t{-2});
    [[maybe_unused]] auto boolBuffer = Serialize(/*t=*/true);

    REQUIRE(std::is_same_v<decltype(byteBuffer), std::array<std::byte, sizeof(std::byte)>>);
    REQUIRE(std::is_same_v<decltype(int8Buffer), std::array<std::byte, sizeof(ts::int8_t)>>);
    REQUIRE(std::is_same_v<decltype(uint16Buffer), std::array<std::byte, sizeof(ts::uint16_t)>>);
    REQUIRE(std::is_same_v<decltype(int32Buffer), std::array<std::byte, sizeof(std::int32_t)>>);
    REQUIRE(std::is_same_v<decltype(boolBuffer), std::array<std::byte, sizeof(bool)>>);

    auto byteIsCorrectlySerialized = byteBuffer[0] == std::byte{0xAA};
    auto int8IsCorrectlySerialized = int8Buffer[0] == std::byte{0xFC};
    auto uint16IsCorrectlySerialized =
        (uint16Buffer[0] == std::byte{0x0B} and uint16Buffer[1] == std::byte{0x00});
    auto int32IsCorrectlySerialized =
        (int32Buffer[0] == std::byte{0xFE} and int32Buffer[1] == std::byte{0xFF}
         and int32Buffer[2] == std::byte{0xFF} and int32Buffer[3] == std::byte{0xFF});
    REQUIRE(byteIsCorrectlySerialized);
    REQUIRE(int8IsCorrectlySerialized);
    REQUIRE(uint16IsCorrectlySerialized);
    REQUIRE(int32IsCorrectlySerialized);
}

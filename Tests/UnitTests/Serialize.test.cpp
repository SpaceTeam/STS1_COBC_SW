#include <Sts1CobcSw/Serialize/Serialize.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>


using sts1cobcsw::serialize::Serialize;


TEST_CASE("TriviallySerializable")
{
    using sts1cobcsw::serialize::TriviallySerializable;

    struct EmptyStruct
    {
    };
    struct SingleInt32
    {
        int32_t i;
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
    // Pointers and arrays are not
    REQUIRE(not TriviallySerializable<char *>);
    REQUIRE(not TriviallySerializable<int[]>);  // NOLINT
    REQUIRE(not TriviallySerializable<std::array<double, 3>>);
    // User-defined types aren't either
    REQUIRE(not TriviallySerializable<EmptyStruct>);
    REQUIRE(not TriviallySerializable<SingleInt32>);
    // TODO: Check type_safe::uint16_t et al.
}


TEST_CASE("Serialize TriviallySerializable types")
{
    auto b = std::byte{0xAA};
    uint16_t u16 = 3;
    int32_t i32 = -2;

    auto bBuffer = Serialize(b);
    auto u16Buffer = Serialize(u16);
    auto i32Buffer = Serialize(i32);

    REQUIRE(std::is_same<decltype(bBuffer), std::array<std::byte, sizeof(std::byte)>>::value);
    REQUIRE(std::is_same<decltype(u16Buffer), std::array<std::byte, sizeof(uint16_t)>>::value);
    REQUIRE(std::is_same<decltype(i32Buffer), std::array<std::byte, sizeof(int32_t)>>::value);

    auto bIsCorrectlySerialized = bBuffer[0] == std::byte{0xAA};
    auto u16IsCorrectlySerialized =
        (u16Buffer[0] == std::byte{0x03} and u16Buffer[1] == std::byte{0x00});
    auto i32IsCorrectlySerialized =
        (i32Buffer[0] == std::byte{0xFE} and i32Buffer[1] == std::byte{0xFF}
         and i32Buffer[2] == std::byte{0xFF} and i32Buffer[3] == std::byte{0xFF});
    REQUIRE(bIsCorrectlySerialized);
    REQUIRE(u16IsCorrectlySerialized);
    REQUIRE(i32IsCorrectlySerialized);
}

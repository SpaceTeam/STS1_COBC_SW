#include <Sts1CobcSw/Serial/Serial.hpp>

#include <catch2/catch_test_macros.hpp>
#include <type_safe/types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>


namespace ts = type_safe;

using ts::operator""_i8;
using ts::operator""_u16;
using ts::operator""_i32;

using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::operator""_B;
using sts1cobcsw::serial::Serialize;
using sts1cobcsw::serial::SerializeTo;
using sts1cobcsw::serial::serialSize;


TEST_CASE("TriviallySerializable")
{
    using sts1cobcsw::serial::TriviallySerializable;

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
    REQUIRE(TriviallySerializable<bool>);
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
    auto byteBuffer = Serialize(std::byte{0xAA});
    auto int8Buffer = Serialize(-4_i8);
    auto uint16Buffer = Serialize(11_u16);
    auto int32Buffer = Serialize(std::int32_t{-2});
    [[maybe_unused]] auto boolBuffer = Serialize(true);  // NOLINT(bugprone-argument-comment)

    REQUIRE(std::is_same_v<decltype(byteBuffer), std::array<Byte, sizeof(std::byte)>>);
    REQUIRE(std::is_same_v<decltype(int8Buffer), std::array<Byte, sizeof(ts::int8_t)>>);
    REQUIRE(std::is_same_v<decltype(uint16Buffer), std::array<Byte, sizeof(ts::uint16_t)>>);
    REQUIRE(std::is_same_v<decltype(int32Buffer), std::array<Byte, sizeof(std::int32_t)>>);
    REQUIRE(std::is_same_v<decltype(boolBuffer), std::array<Byte, sizeof(bool)>>);

    auto byteIsCorrectlySerialized = byteBuffer[0] == 0xAA_B;
    auto int8IsCorrectlySerialized = int8Buffer[0] == 0xFC_B;
    auto uint16IsCorrectlySerialized = (uint16Buffer[0] == 0x0B_B and uint16Buffer[1] == 0x00_B);
    auto int32IsCorrectlySerialized = (int32Buffer[0] == 0xFE_B and int32Buffer[1] == 0xFF_B
                                       and int32Buffer[2] == 0xFF_B and int32Buffer[3] == 0xFF_B);
    REQUIRE(byteIsCorrectlySerialized);
    REQUIRE(int8IsCorrectlySerialized);
    REQUIRE(uint16IsCorrectlySerialized);
    REQUIRE(int32IsCorrectlySerialized);
}


// The following shows everything that is necessary to serialize a user-defined type
struct S
{
    ts::uint16_t i16 = 0_u16;
    ts::int32_t u32 = 0_i32;
};


namespace sts1cobcsw::serial
{
// 1. Add a specialization of the variable template serialSize<> which computes the buffer size
//    necessary to hold a serialized S.
template<>
constexpr std::size_t serialSize<S> = totalSerialSize<decltype(S::i16), decltype(S::u32)>;


// 2. Add a specialization for the function template SerializeTo<>() which defines how S is
//    serialized to the given memory destination. The returned pointer must point to the next free
//    byte in memory.
template<>
constexpr auto SerializeTo<S>(Byte * destination, S const & data) -> Byte *
{
    destination = SerializeTo(destination, data.i16);
    destination = SerializeTo(destination, data.u32);
    return destination;
}
}


TEST_CASE("Serialize user-defined types")
{
    auto sBuffer = Serialize(S{0xABCD_u16, 0x12345678_i32});

    REQUIRE(std::is_same_v<decltype(sBuffer), std::array<Byte, 2 + 4>>);

    auto structIsCorrectlySerialized =
        (sBuffer[0] == 0xCD_B and sBuffer[1] == 0xAB_B and sBuffer[2] == 0x78_B
         and sBuffer[3] == 0x56_B and sBuffer[4] == 0x34_B and sBuffer[5] == 0x12_B);
    REQUIRE(structIsCorrectlySerialized);
}

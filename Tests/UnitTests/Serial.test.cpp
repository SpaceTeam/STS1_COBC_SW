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
using sts1cobcsw::serial::Deserialize;
using sts1cobcsw::serial::Serialize;


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
    auto byteBuffer = Serialize(0xAA_B);
    auto int8Buffer = Serialize(-4_i8);
    auto uint16Buffer = Serialize(11_u16);
    auto int32Buffer = Serialize(std::int32_t{-2});
    [[maybe_unused]] auto boolBuffer = Serialize(true);  // NOLINT(bugprone-argument-comment)

    REQUIRE(std::is_same_v<decltype(byteBuffer), std::array<Byte, sizeof(std::byte)>>);
    REQUIRE(std::is_same_v<decltype(int8Buffer), std::array<Byte, sizeof(ts::int8_t)>>);
    REQUIRE(std::is_same_v<decltype(uint16Buffer), std::array<Byte, sizeof(ts::uint16_t)>>);
    REQUIRE(std::is_same_v<decltype(int32Buffer), std::array<Byte, sizeof(std::int32_t)>>);
    REQUIRE(std::is_same_v<decltype(boolBuffer), std::array<Byte, sizeof(bool)>>);

    // REQUIRE magic can't handle std::byte, so we cast
    REQUIRE(std::uint8_t(byteBuffer[0]) == 0xAA);
    REQUIRE(std::uint8_t(int8Buffer[0]) == 0xFC);
    REQUIRE(std::uint8_t(uint16Buffer[0]) == 0x0B);
    REQUIRE(std::uint8_t(uint16Buffer[1]) == 0x00);
    REQUIRE(std::uint8_t(int32Buffer[0]) == 0xFE);
    REQUIRE(std::uint8_t(int32Buffer[1]) == 0xFF);
    REQUIRE(std::uint8_t(int32Buffer[2]) == 0xFF);
    REQUIRE(std::uint8_t(int32Buffer[3]) == 0xFF);
}


TEST_CASE("Deserialize TriviallySerializable types")
{
    auto buffer = std::array{0x01_B, 0x02_B, 0x03_B, 0x04_B};

    auto int32 = Deserialize<ts::int32_t>(buffer);
    auto uint16 = Deserialize<std::uint16_t>(std::span<Byte, 2>(buffer.begin(), 2));
    auto int8 = Deserialize<ts::int8_t>(std::span<Byte, 1>(buffer.begin() + 2, 1));

    REQUIRE(int32.get() == (4U << 24U) + (3U << 16U) + (2U << 8U) + 1U);
    REQUIRE(uint16 == (2U << 8U) + 1);
    REQUIRE(int8.get() == 3);
}


TEST_CASE("Deserialize is the inverse of Serialize")
{
    auto cBuffer = Serialize('x');
    auto int8Buffer = Serialize(std::int8_t{-56});
    auto uint16Buffer = Serialize(std::uint16_t{3333});
    auto int32Buffer = Serialize(-123456_i32);
    auto booleanBuffer = Serialize(ts::bool_t{true});  // NOLINT(bugprone-argument-comment)

    auto character = Deserialize<char>(std::span(cBuffer));
    auto int8 = Deserialize<std::int8_t>(std::span(int8Buffer));
    auto uint16 = Deserialize<std::uint16_t>(std::span(uint16Buffer));
    auto int32 = Deserialize<ts::int32_t>(std::span(int32Buffer));
    auto boolean = Deserialize<ts::bool_t>(std::span(booleanBuffer));

    REQUIRE(character == 'x');
    REQUIRE(int8 == -56);
    REQUIRE(uint16 == 3333);
    REQUIRE(int32.get() == -123456);
    REQUIRE(boolean == true);
}


// The following shows everything that is necessary to (de-)serialize a user-defined type
struct S
{
    ts::uint16_t u16 = 0_u16;
    ts::int32_t i32 = 0_i32;
};


namespace sts1cobcsw::serial
{
// 1. Add a specialization of the variable template serialSize<> which computes the buffer size
//    necessary to hold a serialized S.
template<>
constexpr std::size_t serialSize<S> = totalSerialSize<decltype(S::u16), decltype(S::i32)>;
// You could also write
// template<>
// constexpr std::size_t serialSize<S> = totalSerialSize<ts::uint16_t, ts::int32_t>;
}


// 2. Overload SerializeTo() to define how S is serialized to the given memory destination. The
//    returned pointer must point to the next free byte in memory.
auto SerializeTo(Byte * destination, S const & data) -> Byte *
{
    destination = sts1cobcsw::serial::SerializeTo(destination, data.u16);
    destination = sts1cobcsw::serial::SerializeTo(destination, data.i32);
    return destination;
}


auto DeserializeFrom(Byte * source, S * data) -> Byte *
{
    source = sts1cobcsw::serial::DeserializeFrom(source, &(data->u16));
    source = sts1cobcsw::serial::DeserializeFrom(source, &(data->i32));
    return source;
}


TEST_CASE("(De-)Serialize user-defined types")
{
    auto sBuffer = Serialize(S{.u16 = 0xABCD_u16, .i32 = 0x12345678_i32});
    REQUIRE(std::is_same_v<decltype(sBuffer), std::array<Byte, 2 + 4>>);

    // REQUIRE magic can't handle std::byte, so we cast
    REQUIRE(std::uint8_t(sBuffer[0]) == 0xCD);
    REQUIRE(std::uint8_t(sBuffer[1]) == 0xAB);
    REQUIRE(std::uint8_t(sBuffer[2]) == 0x78);
    REQUIRE(std::uint8_t(sBuffer[3]) == 0x56);
    REQUIRE(std::uint8_t(sBuffer[4]) == 0x34);
    REQUIRE(std::uint8_t(sBuffer[5]) == 0x12);

    auto s = Deserialize<S>(sBuffer);
    REQUIRE(s.u16.get() == 0xABCD);
    REQUIRE(s.i32.get() == 0x12345678);

    auto a = std::array<Byte, 6>{};
    auto s2 = Deserialize<S>(a);
    REQUIRE(s2.u16.get() == 0);
    REQUIRE(s2.i32.get() == 0);
}

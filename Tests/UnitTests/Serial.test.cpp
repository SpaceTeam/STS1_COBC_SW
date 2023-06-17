#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <catch2/catch_test_macros.hpp>
#include <type_safe/types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>


namespace ts = type_safe;

using ts::operator""_i8;
using ts::operator""_u8;
using ts::operator""_i16;
using ts::operator""_u16;
using ts::operator""_i32;
using ts::operator""_u32;
using ts::operator""_i64;
using ts::operator""_u64;

using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::operator""_b;
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
    REQUIRE(TriviallySerializable<ts::uint8_t>);
    REQUIRE(TriviallySerializable<ts::int16_t>);
    REQUIRE(TriviallySerializable<ts::uint32_t>);
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
    auto int32Buffer = Serialize(-2_i32);
    auto uint64Buffer = Serialize(0x0102030405060708_u64);
    [[maybe_unused]] auto boolBuffer = Serialize(true);  // NOLINT(bugprone-argument-comment)

    REQUIRE(std::is_same_v<decltype(byteBuffer), std::array<Byte, sizeof(std::byte)>>);
    REQUIRE(std::is_same_v<decltype(int8Buffer), std::array<Byte, sizeof(ts::int8_t)>>);
    REQUIRE(std::is_same_v<decltype(uint16Buffer), std::array<Byte, sizeof(ts::uint16_t)>>);
    REQUIRE(std::is_same_v<decltype(int32Buffer), std::array<Byte, sizeof(ts::int32_t)>>);
    REQUIRE(std::is_same_v<decltype(uint64Buffer), std::array<Byte, sizeof(ts::uint64_t)>>);
    REQUIRE(std::is_same_v<decltype(boolBuffer), std::array<Byte, sizeof(bool)>>);

    // REQUIRE magic can't handle std::byte, so we cast
    REQUIRE(int(byteBuffer[0]) == 0xAA);
    REQUIRE(int(int8Buffer[0]) == 0xFC);

    REQUIRE(int(uint16Buffer[0]) == 0x0B);
    REQUIRE(int(uint16Buffer[1]) == 0x00);

    REQUIRE(int(int32Buffer[0]) == 0xFE);
    REQUIRE(int(int32Buffer[1]) == 0xFF);
    REQUIRE(int(int32Buffer[2]) == 0xFF);
    REQUIRE(int(int32Buffer[3]) == 0xFF);

    REQUIRE(int(uint64Buffer[0]) == 0x08);
    REQUIRE(int(uint64Buffer[1]) == 0x07);
    REQUIRE(int(uint64Buffer[2]) == 0x06);
    REQUIRE(int(uint64Buffer[3]) == 0x05);
    REQUIRE(int(uint64Buffer[4]) == 0x04);
    REQUIRE(int(uint64Buffer[5]) == 0x03);
    REQUIRE(int(uint64Buffer[6]) == 0x02);
    REQUIRE(int(uint64Buffer[7]) == 0x01);
}


TEST_CASE("Deserialize TriviallySerializable types")
{
    auto buffer = std::array{0x01_b, 0x02_b, 0x03_b, 0x04_b};
    auto int32 = Deserialize<std::int32_t>(buffer);
    auto uint16 = Deserialize<std::uint16_t>(std::span(buffer).first<sizeof(std::uint16_t)>());
    auto int8 = Deserialize<std::int8_t>(std::span(buffer).subspan<2, sizeof(std::int8_t)>());

    REQUIRE(int32 == (4U << 24U) + (3U << 16U) + (2U << 8U) + 1U);
    REQUIRE(uint16 == (2U << 8U) + 1);
    REQUIRE(int8 == 3);
}


TEST_CASE("Deserialize() is the inverse of Serialize()")
{
    auto cBuffer = Serialize('x');
    auto uint8Buffer = Serialize(56_u8);
    auto int16Buffer = Serialize(-3333_i16);
    auto uint32Buffer = Serialize(123456_u32);
    auto int64Buffer = Serialize(-999999_i64);
    auto booleanBuffer = Serialize(ts::bool_t{true});  // NOLINT(bugprone-argument-comment)

    auto character = Deserialize<char>(cBuffer);
    auto uint8 = Deserialize<ts::uint8_t>(uint8Buffer);
    auto int16 = Deserialize<ts::int16_t>(int16Buffer);
    auto uint32 = Deserialize<ts::uint32_t>(uint32Buffer);
    auto int64 = Deserialize<ts::int64_t>(int64Buffer);
    auto boolean = Deserialize<ts::bool_t>(booleanBuffer);

    REQUIRE(character == 'x');
    REQUIRE(uint8.get() == 56);
    REQUIRE(int16.get() == -3333);
    REQUIRE(uint32.get() == 123456);
    REQUIRE(int64.get() == -999999);
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
auto SerializeTo(void * destination, S const & data) -> void *
{
    destination = sts1cobcsw::serial::SerializeTo(destination, data.u16);
    destination = sts1cobcsw::serial::SerializeTo(destination, data.i32);
    return destination;
}


auto DeserializeFrom(void const * source, S * data) -> void const *
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
    REQUIRE(int(sBuffer[0]) == 0xCD);
    REQUIRE(int(sBuffer[1]) == 0xAB);
    REQUIRE(int(sBuffer[2]) == 0x78);
    REQUIRE(int(sBuffer[3]) == 0x56);
    REQUIRE(int(sBuffer[4]) == 0x34);
    REQUIRE(int(sBuffer[5]) == 0x12);

    auto s = Deserialize<S>(sBuffer);
    REQUIRE(s.u16.get() == 0xABCD);
    REQUIRE(s.i32.get() == 0x12345678);
}

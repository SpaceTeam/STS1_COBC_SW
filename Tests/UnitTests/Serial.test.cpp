#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

using sts1cobcsw::Byte;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)
using sts1cobcsw::Deserialize;
using sts1cobcsw::Serialize;


TEST_CASE("TriviallySerializable")
{
    using sts1cobcsw::TriviallySerializable;

    struct EmptyStruct
    {
    };
    struct SingleInt32
    {
        std::int32_t i = 0;
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
    // Pointers and arrays are not TriviallySerializable
    REQUIRE(not TriviallySerializable<char *>);
    REQUIRE(not TriviallySerializable<int[]>);  // NOLINT
    REQUIRE(not TriviallySerializable<std::array<double, 3>>);
    // User-defined types aren't either
    REQUIRE(not TriviallySerializable<EmptyStruct>);
    REQUIRE(not TriviallySerializable<SingleInt32>);
}


TEST_CASE("HasEndianness")
{
    using sts1cobcsw::HasEndianness;

    struct EmptyStruct
    {
    };
    struct SingleInt32
    {
        std::int32_t i = 0;
    };

    // POD types are HasEndianness
    REQUIRE(HasEndianness<std::byte>);
    REQUIRE(HasEndianness<char>);
    REQUIRE(HasEndianness<unsigned char>);
    REQUIRE(HasEndianness<short>);
    REQUIRE(HasEndianness<unsigned short>);
    REQUIRE(HasEndianness<int>);
    REQUIRE(HasEndianness<unsigned int>);
    REQUIRE(HasEndianness<long>);
    REQUIRE(HasEndianness<unsigned long>);
    REQUIRE(HasEndianness<bool>);
    // Floats, type_safe bools, pointers, and arrays are not HasEndianness
    REQUIRE(not HasEndianness<float>);
    REQUIRE(not HasEndianness<double>);
    REQUIRE(not HasEndianness<char *>);
    REQUIRE(not HasEndianness<int[]>);  // NOLINT
    REQUIRE(not HasEndianness<std::array<double, 3>>);
    // User-defined types aren't either
    REQUIRE(not HasEndianness<EmptyStruct>);
    REQUIRE(not HasEndianness<SingleInt32>);
}


TEST_CASE("Serialize TriviallySerializable types (default endian)")
{
    auto byteBuffer = Serialize(std::byte{0xAA});
    auto int8Buffer = Serialize(static_cast<std::int8_t>(-4));
    auto uint16Buffer = Serialize(static_cast<std::uint16_t>(11));
    auto int32Buffer = Serialize(static_cast<std::int32_t>(-2));
    auto uint64Buffer = Serialize(static_cast<std::uint64_t>(0x0102030405060708));
    [[maybe_unused]] auto boolBuffer = Serialize(true);  // NOLINT(bugprone-argument-comment)

    REQUIRE(std::is_same_v<decltype(byteBuffer), std::array<Byte, sizeof(std::byte)>>);
    REQUIRE(std::is_same_v<decltype(int8Buffer), std::array<Byte, sizeof(std::int8_t)>>);
    REQUIRE(std::is_same_v<decltype(uint16Buffer), std::array<Byte, sizeof(std::uint16_t)>>);
    REQUIRE(std::is_same_v<decltype(int32Buffer), std::array<Byte, sizeof(std::int32_t)>>);
    REQUIRE(std::is_same_v<decltype(uint64Buffer), std::array<Byte, sizeof(std::uint64_t)>>);
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


TEST_CASE("Serialize TriviallySerializable types (big endian)")
{
    using std::endian;

    auto byteBuffer = Serialize<endian::big>(std::byte{0xAA});
    auto int8Buffer = Serialize<endian::big>(static_cast<std::int8_t>(-4));
    auto uint16Buffer = Serialize<endian::big>(static_cast<std::uint16_t>(11));
    auto int32Buffer = Serialize<endian::big>(-2);
    auto uint64Buffer = Serialize<endian::big>(static_cast<std::uint64_t>(0x0102030405060708));

    // REQUIRE magic can't handle std::byte, so we cast
    REQUIRE(int(byteBuffer[0]) == 0xAA);
    REQUIRE(int(int8Buffer[0]) == 0xFC);

    REQUIRE(int(uint16Buffer[0]) == 0x00);
    REQUIRE(int(uint16Buffer[1]) == 0x0B);

    REQUIRE(int(int32Buffer[0]) == 0xFF);
    REQUIRE(int(int32Buffer[1]) == 0xFF);
    REQUIRE(int(int32Buffer[2]) == 0xFF);
    REQUIRE(int(int32Buffer[3]) == 0xFE);

    REQUIRE(int(uint64Buffer[0]) == 0x01);
    REQUIRE(int(uint64Buffer[1]) == 0x02);
    REQUIRE(int(uint64Buffer[2]) == 0x03);
    REQUIRE(int(uint64Buffer[3]) == 0x04);
    REQUIRE(int(uint64Buffer[4]) == 0x05);
    REQUIRE(int(uint64Buffer[5]) == 0x06);
    REQUIRE(int(uint64Buffer[6]) == 0x07);
    REQUIRE(int(uint64Buffer[7]) == 0x08);
}


TEST_CASE("Deserialize TriviallySerializable types (default endian)")
{
    auto buffer = std::array{0x01_b, 0x02_b, 0x03_b, 0x04_b};
    auto int32 = Deserialize<std::int32_t>(buffer);
    auto uint16 = Deserialize<std::uint16_t>(std::span(buffer).first<sizeof(std::uint16_t)>());
    auto int8 = Deserialize<std::int8_t>(std::span(buffer).subspan<2, sizeof(std::int8_t)>());

    REQUIRE(int32 == (4U << 24U) + (3U << 16U) + (2U << 8U) + 1U);
    REQUIRE(uint16 == (2U << 8U) + 1);
    REQUIRE(int8 == 3);
}


TEST_CASE("Deserialize TriviallySerializable types (big endian)")
{
    using std::endian;

    auto buffer = std::array{0x01_b, 0x02_b, 0x03_b, 0x04_b};
    auto int32 = Deserialize<endian::big, std::int32_t>(buffer);
    auto uint16 = Deserialize<endian::big, std::uint16_t>(std::span(buffer).first<2>());
    auto int8 = Deserialize<endian::big, std::int8_t>(std::span(buffer).subspan<2, 1>());

    REQUIRE(int32 == (1U << 24U) + (2U << 16U) + (3U << 8U) + 4U);
    REQUIRE(uint16 == (1U << 8U) + 2);
    REQUIRE(int8 == 3);
}


TEST_CASE("Deserialize() is the inverse of Serialize() (default endian)")
{
    auto cBuffer = Serialize('x');
    auto uint8Buffer = Serialize(static_cast<std::uint8_t>(56));
    auto int16Buffer = Serialize(static_cast<std::int16_t>(-3333));
    auto uint32Buffer = Serialize(static_cast<std::uint32_t>(123456));
    auto int64Buffer = Serialize(static_cast<std::int64_t>(-999999));
    auto booleanBuffer = Serialize(true);  // NOLINT(bugprone-argument-comment)

    auto character = Deserialize<char>(cBuffer);
    auto uint8 = Deserialize<std::uint8_t>(uint8Buffer);
    auto int16 = Deserialize<std::int16_t>(int16Buffer);
    auto uint32 = Deserialize<std::uint32_t>(uint32Buffer);
    auto int64 = Deserialize<std::int64_t>(int64Buffer);
    auto boolean = Deserialize<bool>(booleanBuffer);

    REQUIRE(character == 'x');
    REQUIRE(uint8 == 56);
    REQUIRE(int16 == -3333);
    REQUIRE(uint32 == 123456);
    REQUIRE(int64 == -999999);
    REQUIRE(boolean == true);
}


TEST_CASE("Deserialize() is the inverse of Serialize() (big endian)")
{
    using std::endian;

    auto cBuffer = Serialize<endian::big>('x');
    auto uint8Buffer = Serialize<endian::big>(static_cast<std::uint8_t>(56));
    auto int16Buffer = Serialize<endian::big>(static_cast<std::int16_t>(-3333));
    auto uint32Buffer = Serialize<endian::big>(static_cast<std::uint32_t>(123456));
    auto int64Buffer = Serialize<endian::big>(static_cast<std::int64_t>(-999999));
    auto booleanBuffer = Serialize<endian::big>(true);  // NOLINT(bugprone-argument-comment)

    auto character = Deserialize<endian::big, char>(cBuffer);
    auto uint8 = Deserialize<endian::big, std::uint8_t>(uint8Buffer);
    auto int16 = Deserialize<endian::big, std::int16_t>(int16Buffer);
    auto uint32 = Deserialize<endian::big, std::uint32_t>(uint32Buffer);
    auto int64 = Deserialize<endian::big, std::int64_t>(int64Buffer);
    auto boolean = Deserialize<endian::big, bool>(booleanBuffer);

    REQUIRE(character == 'x');
    REQUIRE(uint8 == 56);
    REQUIRE(int16 == -3333);
    REQUIRE(uint32 == 123456);
    REQUIRE(int64 == -999999);
    REQUIRE(boolean == true);
}


// The following shows everything that is necessary to (de-)serialize a user-defined type
struct S
{
    std::uint16_t u16 = 0;
    std::int32_t i32 = 0;
};


namespace sts1cobcsw
{
// 1. Add a specialization of the variable template serialSize<> which computes the buffer size
//    necessary to hold a serialized S.
template<>
constexpr std::size_t serialSize<S> = totalSerialSize<decltype(S::u16), decltype(S::i32)>;
// You could also write
// template<>
// constexpr std::size_t serialSize<S> = totalSerialSize<std::uint16_t, std::int32_t>;
}


// 2. Overload SerializeTo() to define how S is serialized to the given memory destination. The
//    returned pointer must point to the next free byte in memory.
template<std::endian endianness>
auto SerializeTo(void * destination, S const & data) -> void *
{
    destination = sts1cobcsw::SerializeTo<endianness>(destination, data.u16);
    destination = sts1cobcsw::SerializeTo<endianness>(destination, data.i32);
    return destination;
}

template<std::endian endianness>
auto DeserializeFrom(void const * source, S * data) -> void const *
{
    source = sts1cobcsw::DeserializeFrom<endianness>(source, &(data->u16));
    source = sts1cobcsw::DeserializeFrom<endianness>(source, &(data->i32));
    return source;
}


TEST_CASE("(De-)Serialize user-defined types (default endian)")
{
    auto sBuffer = Serialize(S{.u16 = 0xABCD, .i32 = 0x12345678});
    REQUIRE(std::is_same_v<decltype(sBuffer), std::array<Byte, 2 + 4>>);

    // REQUIRE magic can't handle std::byte, so we cast
    REQUIRE(int(sBuffer[0]) == 0xCD);
    REQUIRE(int(sBuffer[1]) == 0xAB);
    REQUIRE(int(sBuffer[2]) == 0x78);
    REQUIRE(int(sBuffer[3]) == 0x56);
    REQUIRE(int(sBuffer[4]) == 0x34);
    REQUIRE(int(sBuffer[5]) == 0x12);

    auto s = Deserialize<S>(sBuffer);
    REQUIRE(s.u16 == 0xABCD);
    REQUIRE(s.i32 == 0x12345678);
}


TEST_CASE("(De-)Serialize user-defined types (big endian)")
{
    auto sBuffer = Serialize<std::endian::big>(S{.u16 = 0xABCD, .i32 = 0x12345678});
    REQUIRE(std::is_same_v<decltype(sBuffer), std::array<Byte, 2 + 4>>);

    // REQUIRE magic can't handle std::byte, so we cast
    REQUIRE(int(sBuffer[0]) == 0xAB);
    REQUIRE(int(sBuffer[1]) == 0xCD);
    REQUIRE(int(sBuffer[2]) == 0x12);
    REQUIRE(int(sBuffer[3]) == 0x34);
    REQUIRE(int(sBuffer[4]) == 0x56);
    REQUIRE(int(sBuffer[5]) == 0x78);

    auto s = Deserialize<std::endian::big, S>(sBuffer);
    REQUIRE(s.u16 == 0xABCD);
    REQUIRE(s.i32 == 0x12345678);
}

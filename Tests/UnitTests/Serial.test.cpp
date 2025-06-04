#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>


using sts1cobcsw::Byte;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)
using sts1cobcsw::Deserialize;
using sts1cobcsw::DeserializeFrom;
using sts1cobcsw::Serialize;
using sts1cobcsw::serialSize;
using sts1cobcsw::totalSerialSize;
using sts1cobcsw::UInt;


static_assert(serialSize<UInt<0>> == 1);
static_assert(serialSize<UInt<1>> == 1);
static_assert(serialSize<UInt<8>> == 1);
static_assert(serialSize<UInt<9>> == 2);
static_assert(serialSize<UInt<16>> == 2);
static_assert(serialSize<UInt<17>> == 4);
static_assert(serialSize<UInt<32>> == 4);
static_assert(serialSize<UInt<33>> == 8);
static_assert(serialSize<UInt<64>> == 8);

static_assert(totalSerialSize<UInt<0>, UInt<1>, UInt<2>, UInt<3>> == 1);
static_assert(totalSerialSize<UInt<0>, UInt<1>, UInt<2>, UInt<3>, UInt<4>> == 2);
static_assert(totalSerialSize<UInt<16>, UInt<1>> == 3);
static_assert(totalSerialSize<UInt<16>, UInt<8>, UInt<2>> == 4);
static_assert(totalSerialSize<UInt<32>, UInt<3>> == 5);
static_assert(totalSerialSize<UInt<32>, UInt<8>, UInt<4>> == 6);
static_assert(totalSerialSize<UInt<64>, UInt<1>> == 9);


TEST_CASE("TriviallySerializable and HasEndianness")  // NOLINT(cert-err58-cpp)
{
    using sts1cobcsw::HasEndianness;
    using sts1cobcsw::TriviallySerializable;

    enum ClassicEnum
    {
        one,
    };
    enum class ScopedEnum
    {
        one,
    };
    struct EmptyStruct
    {
    };
    struct SingleInt32
    {
        std::int32_t i = 0;
    };

    SECTION("TriviallySerializable")
    {
        // POD types and enums are TriviallySerializable
        STATIC_CHECK(TriviallySerializable<std::byte>);
        STATIC_CHECK(TriviallySerializable<char>);
        STATIC_CHECK(TriviallySerializable<unsigned char>);
        STATIC_CHECK(TriviallySerializable<short>);
        STATIC_CHECK(TriviallySerializable<unsigned short>);
        STATIC_CHECK(TriviallySerializable<int>);
        STATIC_CHECK(TriviallySerializable<unsigned int>);
        STATIC_CHECK(TriviallySerializable<long>);
        STATIC_CHECK(TriviallySerializable<unsigned long>);
        STATIC_CHECK(TriviallySerializable<float>);
        STATIC_CHECK(TriviallySerializable<double>);
        STATIC_CHECK(TriviallySerializable<bool>);
        STATIC_CHECK(TriviallySerializable<ClassicEnum>);
        STATIC_CHECK(TriviallySerializable<ScopedEnum>);
        // Pointers and arrays are not TriviallySerializable
        STATIC_CHECK(TriviallySerializable<char *> == false);
        STATIC_CHECK(TriviallySerializable<int[]> == false);  // NOLINT
        STATIC_CHECK(TriviallySerializable<std::array<double, 3>> == false);
        // UInt<> and user-defined types aren't either
        STATIC_CHECK(TriviallySerializable<UInt<0>> == false);
        STATIC_CHECK(TriviallySerializable<UInt<1>> == false);
        STATIC_CHECK(TriviallySerializable<UInt<64>> == false);
        STATIC_CHECK(TriviallySerializable<EmptyStruct> == false);
        STATIC_CHECK(TriviallySerializable<SingleInt32> == false);
    }

    SECTION("HasEndianness")
    {
        // POD types and enums are HasEndianness
        STATIC_CHECK(HasEndianness<std::byte>);
        STATIC_CHECK(HasEndianness<char>);
        STATIC_CHECK(HasEndianness<unsigned char>);
        STATIC_CHECK(HasEndianness<short>);
        STATIC_CHECK(HasEndianness<unsigned short>);
        STATIC_CHECK(HasEndianness<int>);
        STATIC_CHECK(HasEndianness<unsigned int>);
        STATIC_CHECK(HasEndianness<long>);
        STATIC_CHECK(HasEndianness<unsigned long>);
        STATIC_CHECK(HasEndianness<bool>);
        STATIC_CHECK(HasEndianness<ClassicEnum>);
        STATIC_CHECK(HasEndianness<ScopedEnum>);
        // Floats, type_safe bools, pointers, and arrays are not HasEndianness
        STATIC_CHECK(HasEndianness<float> == false);
        STATIC_CHECK(HasEndianness<double> == false);
        STATIC_CHECK(HasEndianness<char *> == false);
        STATIC_CHECK(HasEndianness<int[]> == false);  // NOLINT
        STATIC_CHECK(HasEndianness<std::array<double, 3>> == false);
        // UInt<> and user-defined types aren't either
        STATIC_CHECK(HasEndianness<UInt<0>> == false);
        STATIC_CHECK(HasEndianness<UInt<1>> == false);
        STATIC_CHECK(HasEndianness<UInt<64>> == false);
        STATIC_CHECK(HasEndianness<EmptyStruct> == false);
        STATIC_CHECK(HasEndianness<SingleInt32> == false);
    }
}


TEST_CASE("Serialize TriviallySerializable types (default endian)")
{
    auto byteBuffer = Serialize(std::byte{0xAA});
    auto int8Buffer = Serialize(static_cast<std::int8_t>(-4));
    auto uint16Buffer = Serialize(static_cast<std::uint16_t>(11));
    auto int32Buffer = Serialize(static_cast<std::int32_t>(-2));
    auto uint64Buffer = Serialize(static_cast<std::uint64_t>(0x0102030405060708));
    [[maybe_unused]] auto boolBuffer = Serialize(true);  // NOLINT(bugprone-argument-comment)

    STATIC_CHECK(std::is_same_v<decltype(byteBuffer), std::array<Byte, sizeof(std::byte)>>);
    STATIC_CHECK(std::is_same_v<decltype(int8Buffer), std::array<Byte, sizeof(std::int8_t)>>);
    STATIC_CHECK(std::is_same_v<decltype(uint16Buffer), std::array<Byte, sizeof(std::uint16_t)>>);
    STATIC_CHECK(std::is_same_v<decltype(int32Buffer), std::array<Byte, sizeof(std::int32_t)>>);
    STATIC_CHECK(std::is_same_v<decltype(uint64Buffer), std::array<Byte, sizeof(std::uint64_t)>>);
    STATIC_CHECK(std::is_same_v<decltype(boolBuffer), std::array<Byte, sizeof(bool)>>);

    // CHECK magic can't handle std::byte, so we cast
    CHECK(int(byteBuffer[0]) == 0xAA);
    CHECK(int(int8Buffer[0]) == 0xFC);

    CHECK(int(uint16Buffer[0]) == 0x0B);
    CHECK(int(uint16Buffer[1]) == 0x00);

    CHECK(int(int32Buffer[0]) == 0xFE);
    CHECK(int(int32Buffer[1]) == 0xFF);
    CHECK(int(int32Buffer[2]) == 0xFF);
    CHECK(int(int32Buffer[3]) == 0xFF);

    CHECK(int(uint64Buffer[0]) == 0x08);
    CHECK(int(uint64Buffer[1]) == 0x07);
    CHECK(int(uint64Buffer[2]) == 0x06);
    CHECK(int(uint64Buffer[3]) == 0x05);
    CHECK(int(uint64Buffer[4]) == 0x04);
    CHECK(int(uint64Buffer[5]) == 0x03);
    CHECK(int(uint64Buffer[6]) == 0x02);
    CHECK(int(uint64Buffer[7]) == 0x01);
}


TEST_CASE("Serialize TriviallySerializable types (big endian)")
{
    using std::endian;

    auto byteBuffer = Serialize<endian::big>(std::byte{0xAA});
    auto int8Buffer = Serialize<endian::big>(static_cast<std::int8_t>(-4));
    auto uint16Buffer = Serialize<endian::big>(static_cast<std::uint16_t>(11));
    auto int32Buffer = Serialize<endian::big>(-2);
    auto uint64Buffer = Serialize<endian::big>(static_cast<std::uint64_t>(0x0102030405060708));

    // CHECK magic can't handle std::byte, so we cast
    CHECK(int(byteBuffer[0]) == 0xAA);
    CHECK(int(int8Buffer[0]) == 0xFC);

    CHECK(int(uint16Buffer[0]) == 0x00);
    CHECK(int(uint16Buffer[1]) == 0x0B);

    CHECK(int(int32Buffer[0]) == 0xFF);
    CHECK(int(int32Buffer[1]) == 0xFF);
    CHECK(int(int32Buffer[2]) == 0xFF);
    CHECK(int(int32Buffer[3]) == 0xFE);

    CHECK(int(uint64Buffer[0]) == 0x01);
    CHECK(int(uint64Buffer[1]) == 0x02);
    CHECK(int(uint64Buffer[2]) == 0x03);
    CHECK(int(uint64Buffer[3]) == 0x04);
    CHECK(int(uint64Buffer[4]) == 0x05);
    CHECK(int(uint64Buffer[5]) == 0x06);
    CHECK(int(uint64Buffer[6]) == 0x07);
    CHECK(int(uint64Buffer[7]) == 0x08);
}


TEST_CASE("Serialize std::array (default endian)")
{
    auto uint16Buffer = Serialize(std::array<std::uint16_t, 3>{0x0001, 0x0203, 0x0405});
    auto int32Buffer = Serialize(std::array<std::int32_t, 2>{0x0809'0A0B, 0x0C0D'0E0F});
    auto uint64Buffer = Serialize(std::array<std::uint64_t, 1>{0x1011'1213'1415'1617});

    STATIC_CHECK(
        std::is_same_v<decltype(uint16Buffer), std::array<Byte, 3 * sizeof(std::uint16_t)>>);
    STATIC_CHECK(std::is_same_v<decltype(int32Buffer), std::array<Byte, 2 * sizeof(std::int32_t)>>);
    STATIC_CHECK(std::is_same_v<decltype(uint64Buffer), std::array<Byte, sizeof(std::uint64_t)>>);

    CHECK(uint16Buffer == std::array{0x01_b, 0x00_b, 0x03_b, 0x02_b, 0x05_b, 0x04_b});
    CHECK(int32Buffer
          == std::array{0x0B_b, 0x0A_b, 0x09_b, 0x08_b, 0x0F_b, 0x0E_b, 0x0D_b, 0x0C_b});
    CHECK(uint64Buffer
          == std::array{0x17_b, 0x16_b, 0x15_b, 0x14_b, 0x13_b, 0x12_b, 0x11_b, 0x10_b});
}


TEST_CASE("Serialize std::array (big endian)")
{
    auto uint16Buffer =
        Serialize<std::endian::big>(std::array<std::uint16_t, 3>{0xABCD, 0x1234, 0xFFEE});
    auto int32Buffer =
        Serialize<std::endian::big>(std::array<std::int32_t, 2>{0x0907'0503, 0x2244'6688});
    auto uint64Buffer =
        Serialize<std::endian::big>(std::array<std::uint64_t, 1>{0xFEDC'BA98'7654'3210});

    STATIC_CHECK(
        std::is_same_v<decltype(uint16Buffer), std::array<Byte, 3 * sizeof(std::uint16_t)>>);
    STATIC_CHECK(std::is_same_v<decltype(int32Buffer), std::array<Byte, 2 * sizeof(std::int32_t)>>);
    STATIC_CHECK(std::is_same_v<decltype(uint64Buffer), std::array<Byte, sizeof(std::uint64_t)>>);

    CHECK(uint16Buffer == std::array{0xAB_b, 0xCD_b, 0x12_b, 0x34_b, 0xFF_b, 0xEE_b});
    CHECK(int32Buffer
          == std::array{0x09_b, 0x07_b, 0x05_b, 0x03_b, 0x22_b, 0x44_b, 0x66_b, 0x88_b});
    CHECK(uint64Buffer
          == std::array{0xFE_b, 0xDC_b, 0xBA_b, 0x98_b, 0x76_b, 0x54_b, 0x32_b, 0x10_b});
}


TEST_CASE("Serialize UInts (big endian)")
{
    auto buffer = std::array<Byte, 2>{};
    (void)SerializeTo<std::endian::big>(
        &buffer, UInt<1>(1), UInt<2>(0), UInt<3>(0b111), UInt<4>(0), UInt<5>(0b1'1111), UInt<1>(0));
    CHECK(buffer[0] == 0b1001'1100_b);
    CHECK(buffer[1] == 0b0011'1110_b);
}


TEST_CASE("Deserialize TriviallySerializable types (default endian)")
{
    auto buffer = std::array{0x01_b, 0x02_b, 0x03_b, 0x04_b};
    auto int32 = Deserialize<std::int32_t>(buffer);
    auto uint16 = Deserialize<std::uint16_t>(std::span(buffer).first<sizeof(std::uint16_t)>());
    auto int8 = Deserialize<std::int8_t>(std::span(buffer).subspan<2, sizeof(std::int8_t)>());

    CHECK(int32 == (4U << 24U) + (3U << 16U) + (2U << 8U) + 1U);
    CHECK(uint16 == (2U << 8U) + 1);
    CHECK(int8 == 3);
}


TEST_CASE("Deserialize TriviallySerializable types (big endian)")
{
    using std::endian;

    auto buffer = std::array{0x01_b, 0x02_b, 0x03_b, 0x04_b};
    auto int32 = Deserialize<endian::big, std::int32_t>(buffer);
    auto uint16 = Deserialize<endian::big, std::uint16_t>(std::span(buffer).first<2>());
    auto int8 = Deserialize<endian::big, std::int8_t>(std::span(buffer).subspan<2, 1>());

    CHECK(int32 == (1U << 24U) + (2U << 16U) + (3U << 8U) + 4U);
    CHECK(uint16 == (1U << 8U) + 2);
    CHECK(int8 == 3);
}


TEST_CASE("Deserialize std::array (default endian)")
{
    auto buffer = std::array{0x05_b, 0x00_b, 0x07_b, 0x00_b, 0x09_b, 0x00_b, 0xFF_b, 0xFF_b};

    auto int16Array = Deserialize<std::array<std::int16_t, 4>>(buffer);
    CHECK(int16Array[0] == 5);
    CHECK(int16Array[1] == 7);
    CHECK(int16Array[2] == 9);
    CHECK(int16Array[3] == -1);

    auto uint32Array = Deserialize<std::array<std::uint32_t, 2>>(buffer);
    CHECK(uint32Array[0] == 0x00070005U);
    CHECK(uint32Array[1] == 0xFFFF0009U);

    auto uint64Array = Deserialize<std::array<std::uint64_t, 1>>(buffer);
    CHECK(uint64Array[0] == 0xFFFF000900070005U);
}


TEST_CASE("Deserialize std::array (big endian)")
{
    auto buffer = std::array{0x00_b, 0x02_b, 0x00_b, 0x04_b, 0x00_b, 0x06_b, 0xFF_b, 0xFE_b};

    auto int16Array = Deserialize<std::endian::big, std::array<std::int16_t, 4>>(buffer);
    CHECK(int16Array[0] == 2);
    CHECK(int16Array[1] == 4);
    CHECK(int16Array[2] == 6);
    CHECK(int16Array[3] == -2);

    auto uint32Array = Deserialize<std::endian::big, std::array<std::uint32_t, 2>>(buffer);
    CHECK(uint32Array[0] == 0x00020004U);
    CHECK(uint32Array[1] == 0x0006FFFEU);

    auto uint64Array = Deserialize<std::endian::big, std::array<std::uint64_t, 1>>(buffer);
    CHECK(uint64Array[0] == 0x000200040006FFFEU);
}


TEST_CASE("Deserialize etl::vector (little endian)")
{
    auto buffer = std::array{0x05_b, 0x00_b, 0x07_b, 0x00_b, 0xFF_b, 0xFF_b};

    auto i16Vector = etl::vector<std::int16_t, 3>{};
    i16Vector.uninitialized_resize(1);
    (void)DeserializeFrom<std::endian::little>(buffer.data(), &i16Vector);
    CHECK(i16Vector[0] == 5);

    i16Vector.uninitialized_resize(2);
    (void)DeserializeFrom<std::endian::little>(buffer.data(), &i16Vector);
    CHECK(i16Vector[0] == 5);
    CHECK(i16Vector[1] == 7);

    i16Vector.uninitialized_resize(3);
    (void)DeserializeFrom<std::endian::little>(buffer.data(), &i16Vector);
    CHECK(i16Vector[0] == 5);
    CHECK(i16Vector[1] == 7);
    CHECK(i16Vector[2] == -1);
}


TEST_CASE("Deserialize etl::vector (big endian)")
{
    auto buffer = std::array{0x00_b, 0x02_b, 0x00_b, 0x04_b, 0xFF_b, 0xFE_b};
    auto i16Vector = etl::vector<std::int16_t, 3>{};

    i16Vector.uninitialized_resize(1);
    (void)DeserializeFrom<std::endian::big>(buffer.data(), &i16Vector);
    CHECK(i16Vector[0] == 2);

    i16Vector.uninitialized_resize(2);
    (void)DeserializeFrom<std::endian::big>(buffer.data(), &i16Vector);
    CHECK(i16Vector[0] == 2);
    CHECK(i16Vector[1] == 4);

    i16Vector.uninitialized_resize(3);
    (void)DeserializeFrom<std::endian::big>(buffer.data(), &i16Vector);
    CHECK(i16Vector[0] == 2);
    CHECK(i16Vector[1] == 4);
    CHECK(i16Vector[2] == -2);
}


TEST_CASE("Deserialize UInts (big endian)")
{
    auto uint1 = UInt<1>{};
    auto uint2 = UInt<2>{};
    auto uint3 = UInt<3>{};
    auto uint4 = UInt<4>{};
    auto uint5 = UInt<5>{};
    auto uint6 = UInt<1>{};

    auto buffer = std::array{0b1001'1100_b, 0b0011'1110_b};
    (void)DeserializeFrom<std::endian::big>(
        buffer.data(), &uint1, &uint2, &uint3, &uint4, &uint5, &uint6);
    CHECK(uint1.ToUnderlying() == 1);
    CHECK(uint2.ToUnderlying() == 0);
    CHECK(uint3.ToUnderlying() == 0b111);
    CHECK(uint4.ToUnderlying() == 0);
    CHECK(uint5.ToUnderlying() == 0b1'1111);
    CHECK(uint6.ToUnderlying() == 0);

    buffer = {0b0110'0011_b, 0b1100'0001_b};
    (void)DeserializeFrom<std::endian::big>(
        buffer.data(), &uint1, &uint2, &uint3, &uint4, &uint5, &uint6);
    CHECK(uint1.ToUnderlying() == 0);
    CHECK(uint2.ToUnderlying() == 0b11);
    CHECK(uint3.ToUnderlying() == 0);
    CHECK(uint4.ToUnderlying() == 0b1111);
    CHECK(uint5.ToUnderlying() == 0);
    CHECK(uint6.ToUnderlying() == 0b1);
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

    CHECK(character == 'x');
    CHECK(uint8 == 56);
    CHECK(int16 == -3333);
    CHECK(uint32 == 123456);
    CHECK(int64 == -999999);
    CHECK(boolean == true);
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

    CHECK(character == 'x');
    CHECK(uint8 == 56);
    CHECK(int16 == -3333);
    CHECK(uint32 == 123456);
    CHECK(int64 == -999999);
    CHECK(boolean == true);
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
    STATIC_CHECK(std::is_same_v<decltype(sBuffer), std::array<Byte, 2 + 4>>);

    // CHECK magic can't handle std::byte, so we cast
    CHECK(int(sBuffer[0]) == 0xCD);
    CHECK(int(sBuffer[1]) == 0xAB);
    CHECK(int(sBuffer[2]) == 0x78);
    CHECK(int(sBuffer[3]) == 0x56);
    CHECK(int(sBuffer[4]) == 0x34);
    CHECK(int(sBuffer[5]) == 0x12);

    auto s = Deserialize<S>(sBuffer);
    CHECK(s.u16 == 0xABCD);
    CHECK(s.i32 == 0x12345678);
}


TEST_CASE("(De-)Serialize user-defined types (big endian)")
{
    auto sBuffer = Serialize<std::endian::big>(S{.u16 = 0xABCD, .i32 = 0x12345678});
    STATIC_CHECK(std::is_same_v<decltype(sBuffer), std::array<Byte, 2 + 4>>);

    // CHECK magic can't handle std::byte, so we cast
    CHECK(int(sBuffer[0]) == 0xAB);
    CHECK(int(sBuffer[1]) == 0xCD);
    CHECK(int(sBuffer[2]) == 0x12);
    CHECK(int(sBuffer[3]) == 0x34);
    CHECK(int(sBuffer[4]) == 0x56);
    CHECK(int(sBuffer[5]) == 0x78);

    auto s = Deserialize<std::endian::big, S>(sBuffer);
    CHECK(s.u16 == 0xABCD);
    CHECK(s.i32 == 0x12345678);
}


TEST_CASE("(De-)Serialize std::array of user-defined types (default endian)")
{
    auto sBuffer = Serialize(std::array{
        S{.u16 = 0xDEAD, .i32 = 0x4433'2211},
        S{.u16 = 0xBEEF, .i32 = 0x1234'5678}
    });
    STATIC_CHECK(std::is_same_v<decltype(sBuffer), std::array<Byte, 2 * (2 + 4)>>);
    // clang-format off
    CHECK(sBuffer == std::array{0xAD_b, 0xDE_b, 0x11_b, 0x22_b, 0x33_b, 0x44_b,
                                0xEF_b, 0xBE_b, 0x78_b, 0x56_b, 0x34_b, 0x12_b});
    // clang-format on

    auto sArray = Deserialize<std::array<S, 2>>(sBuffer);
    CHECK(sArray[0].u16 == 0xDEAD);
    CHECK(sArray[0].i32 == 0x4433'2211);
    CHECK(sArray[1].u16 == 0xBEEF);
    CHECK(sArray[1].i32 == 0x1234'5678);
}


TEST_CASE("(De-)Serialize std::array of user-defined types (big endian)")
{
    auto sBuffer = Serialize<std::endian::big>(std::array{
        S{.u16 = 0xDEAD, .i32 = 0x4433'2211},
        S{.u16 = 0xBEEF, .i32 = 0x1234'5678}
    });
    STATIC_CHECK(std::is_same_v<decltype(sBuffer), std::array<Byte, 2 * (2 + 4)>>);
    // clang-format off
    CHECK(sBuffer == std::array{0xDE_b, 0xAD_b, 0x44_b, 0x33_b, 0x22_b, 0x11_b,
                                0xBE_b, 0xEF_b, 0x12_b, 0x34_b, 0x56_b, 0x78_b});
    // clang-format on

    auto sArray = Deserialize<std::endian::big, std::array<S, 2>>(sBuffer);
    CHECK(sArray[0].u16 == 0xDEAD);
    CHECK(sArray[0].i32 == 0x4433'2211);
    CHECK(sArray[1].u16 == 0xBEEF);
    CHECK(sArray[1].i32 == 0x1234'5678);
}


TEST_CASE("Deserialize etl::vector of user-defined types (little endian)")
{
    // clang-format off
    auto sBuffer = std::array{0xAD_b, 0xDE_b, 0x11_b, 0x22_b, 0x33_b, 0x44_b,
                              0xEF_b, 0xBE_b, 0x78_b, 0x56_b, 0x34_b, 0x12_b};
    // clang-format on
    auto sVector = etl::vector<S, 2>{};

    sVector.uninitialized_resize(1);
    (void)DeserializeFrom<std::endian::big>(sBuffer.data(), &sVector);
    CHECK(sVector[0].u16 == 0xADDE);
    CHECK(sVector[0].i32 == 0x1122'3344);

    sVector.uninitialized_resize(2);
    (void)DeserializeFrom<std::endian::little>(sBuffer.data(), &sVector);
    CHECK(sVector[0].u16 == 0xDEAD);
    CHECK(sVector[0].i32 == 0x4433'2211);
    CHECK(sVector[1].u16 == 0xBEEF);
    CHECK(sVector[1].i32 == 0x1234'5678);
}

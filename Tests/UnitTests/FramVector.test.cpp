#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>


namespace fram = sts1cobcsw::fram;

using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)
using sts1cobcsw::totalSerialSize;


struct S
{
    std::uint16_t u16 = 0;
    std::int32_t i32 = 0;
    std::uint8_t u8 = 0;

    friend auto operator==(S const & lhs, S const & rhs) -> bool = default;
};


namespace sts1cobcsw
{
template<>
constexpr std::size_t serialSize<S> =
    totalSerialSize<decltype(S::u16), decltype(S::i32), decltype(S::u8)>;
}

template<std::endian endianness>
auto SerializeTo(void * destination, S const & data) -> void *;
template<std::endian endianness>
auto DeserializeFrom(void const * source, S * data) -> void const *;

// Define FRAM sections for testing
inline constexpr auto metadataSize = fram::Size(3 * 4);
inline constexpr auto charSection1 =
    sts1cobcsw::Section<fram::Address(0), metadataSize + fram::Size(3)>{};
inline constexpr auto charSection2 =
    sts1cobcsw::Section<charSection1.end, metadataSize + fram::Size(3)>{};
inline constexpr auto sSection =
    sts1cobcsw::Section<charSection2.end, metadataSize + fram::Size(4 * totalSerialSize<S>)>{};

// Instantiate FramVector with different configurations
inline constexpr auto charVector1 = sts1cobcsw::FramVector<char, charSection1, 2>{};
inline constexpr auto charVector2 = sts1cobcsw::FramVector<char, charSection2, 2>{};
inline constexpr auto sVector = sts1cobcsw::FramVector<S, sSection, 4>{};

static constexpr auto charVector1StartAddress = value_of(metadataSize);
static constexpr auto charVector2StartAddress = value_of(charVector1.section.end + metadataSize);

// Static assertions to ensure correct configurations
static_assert(std::is_same_v<decltype(charVector1)::ValueType, char>);
static_assert(charVector1.FramCapacity() == 3);
static_assert(charVector1.CacheCapacity() == 2);
static_assert(sVector.FramCapacity() == 4);
static_assert(sVector.CacheCapacity() == 4);


auto RunUnitTest() -> void
{
    using fram::ram::memory;

    fram::ram::SetAllDoFunctions();
    fram::Initialize();
    fram::framIsWorking.Store(true);
    memory.fill(0x00_b);  // Clear memory

    // SECTION("FRAM is working")
    {
        Require(charVector1.Size() == 0);
        Require(charVector1.IsEmpty());
        Require(not charVector1.IsFull());
        // Reading from an empty vector prints a debug message and returns a default-constructed
        // value
        Require(charVector1.Get(0) == 0x00);

        charVector1.PushBack(0x61);
        Require(charVector1.Size() == 1);
        Require(not charVector1.IsEmpty());
        Require(not charVector1.IsFull());
        Require(charVector1.Get(0) == 0x61);

        charVector1.PushBack(0x62);
        Require(charVector1.Size() == 2);
        Require(not charVector1.IsEmpty());
        Require(not charVector1.IsFull());
        Require(charVector1.Get(1) == 0x62);

        charVector1.PushBack(0x63);
        Require(charVector1.Size() == 3);
        Require(not charVector1.IsEmpty());
        Require(charVector1.IsFull());
        Require(charVector1.Get(2) == 0x63);

        // PushBack() writes to memory
        Require(fram::ram::memory[charVector1StartAddress + 0] == 0x61_b);
        Require(fram::ram::memory[charVector1StartAddress + 1] == 0x62_b);
        Require(fram::ram::memory[charVector1StartAddress + 2] == 0x63_b);

        // PushBack() does nothing, except for printing a debug message, if the vector is full
        Require(charVector1.IsFull());
        charVector1.PushBack(0x64);
        Require(charVector1.Size() == 3);
        Require(charVector1.Get(0) == 0x61);
        Require(charVector1.Get(1) == 0x62);
        Require(charVector1.Get(2) == 0x63);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        Require(charVector1.Get(17) == 0x63);
    }

    // SECTION("FRAM is not working")
    {
        memory.fill(0x00_b);
        fram::framIsWorking.Store(false);

        // Even though we reset the FRAM memory to zero, the cached values are still there
        Require(charVector1.Size() == charVector1.CacheCapacity());
        Require(charVector1.Get(0) == 0x61);
        Require(charVector1.Get(1) == 0x62);

        Require(charVector2.Size() == 0);
        Require(charVector2.IsEmpty());
        Require(not charVector2.IsFull());
        // Reading from an empty vector prints a debug message and returns a default-constructed
        // value
        Require(charVector2.Get(0) == 0x00);

        charVector2.PushBack(11);
        Require(charVector2.Size() == 1);
        Require(not charVector2.IsEmpty());
        Require(not charVector2.IsFull());
        Require(charVector2.Get(0) == 11);

        charVector2.PushBack(12);
        Require(charVector2.Size() == 2);
        Require(not charVector2.IsEmpty());
        Require(charVector2.IsFull());
        Require(charVector2.Get(1) == 12);

        // PushBack() does not write to memory
        Require(fram::ram::memory[charVector2StartAddress + 0] == 0x00_b);
        Require(fram::ram::memory[charVector2StartAddress + 1] == 0x00_b);

        // PushBack() does nothing, except for printing a debug message, if the vector is full
        Require(charVector2.IsFull());
        charVector2.PushBack(13);
        Require(charVector2.Size() == 2);
        Require(charVector2.Get(0) == 11);
        Require(charVector2.Get(1) == 12);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        Require(charVector2.Get(17) == 12);

        charVector2.Clear();
        Require(charVector2.Size() == 0);
        Require(charVector2.IsEmpty());
    }

    // SECTION("Custom type")
    {
        memory.fill(0x00_b);
        fram::framIsWorking.Store(true);

        auto s1 = S{.u16 = 1, .i32 = 100, .u8 = 10};
        auto s2 = S{.u16 = 2, .i32 = 200, .u8 = 20};
        auto s3 = S{.u16 = 3, .i32 = 300, .u8 = 30};
        auto s4 = S{.u16 = 4, .i32 = 400, .u8 = 40};
        auto s5 = S{.u16 = 5, .i32 = 500, .u8 = 50};

        Require(sVector.Size() == 0);
        Require(sVector.IsEmpty());
        Require(not sVector.IsFull());
        // Reading from an empty vector prints a debug message and returns a default-constructed
        // value
        Require(sVector.Get(0) == S{});

        sVector.PushBack(s1);
        Require(sVector.Size() == 1);
        Require(sVector.Get(0) == s1);

        sVector.PushBack(s2);
        Require(sVector.Size() == 2);
        Require(sVector.Get(0) == s1);

        sVector.PushBack(s3);
        sVector.PushBack(s4);
        Require(sVector.Size() == 4);
        Require(sVector.IsFull());

        sVector.PushBack(s5);
        Require(sVector.Size() == 4);
        Require(sVector.Get(3) == s4);

        sVector.Clear();
        Require(sVector.Size() == 0);
        Require(sVector.IsEmpty());
    }
}


template<std::endian endianness>
auto SerializeTo(void * destination, S const & data) -> void *
{
    destination = sts1cobcsw::SerializeTo<endianness>(destination, data.u16);
    destination = sts1cobcsw::SerializeTo<endianness>(destination, data.i32);
    destination = sts1cobcsw::SerializeTo<endianness>(destination, data.u8);
    return destination;
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, S * data) -> void const *
{
    source = sts1cobcsw::DeserializeFrom<endianness>(source, &(data->u16));
    source = sts1cobcsw::DeserializeFrom<endianness>(source, &(data->i32));
    source = sts1cobcsw::DeserializeFrom<endianness>(source, &(data->u8));
    return source;
}

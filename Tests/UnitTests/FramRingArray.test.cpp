#include <Sts1CobcSw/FramSections/FramRingArray.hpp>

#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Fram/FramMock.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/type.hpp>

#include <etl/circular_buffer.h>

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

    friend auto operator==(S const & lhs, S const & rhs) -> bool
    {
        return (lhs.i32 == rhs.i32) and (lhs.u16 == rhs.u16) and (lhs.u8 == rhs.u8);
    }
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


inline constexpr auto indexesSize = fram::Size(3 * 2 * sizeof(std::uint32_t));
inline constexpr auto charSection1 =
    sts1cobcsw::Section<fram::Address(0), indexesSize + fram::Size(4)>{};
inline constexpr auto charSection2 =
    sts1cobcsw::Section<charSection1.end, indexesSize + fram::Size(4)>{};
inline constexpr auto sSection =
    sts1cobcsw::Section<charSection2.end, indexesSize + fram::Size(4 * totalSerialSize<S>)>{};

inline constexpr auto charRingArray1 = sts1cobcsw::FramRingArray<char, charSection1, 2>{};
inline constexpr auto charRingArray2 = sts1cobcsw::FramRingArray<char, charSection2, 2>{};
inline constexpr auto sRingArray = sts1cobcsw::FramRingArray<S, sSection, 2>{};


static_assert(std::is_same_v<decltype(charRingArray1)::ValueType, char>);
static_assert(charRingArray1.FramCapacity() == 3);
static_assert(charRingArray1.CacheCapacity() == 2);
static_assert(charRingArray1.section.begin == fram::Address(0));
static_assert(charRingArray1.section.size == fram::Size(28));
static_assert(charRingArray1.section.end == fram::Address(28));

static_assert(std::is_same_v<decltype(sRingArray)::ValueType, S>);
static_assert(sRingArray.FramCapacity() == 3);
static_assert(sRingArray.CacheCapacity() == 2);
static_assert(sRingArray.section.begin == fram::Address(56));
static_assert(sRingArray.section.size == fram::Size(52));
static_assert(sRingArray.section.end == fram::Address(108));


TEST_CASE("FramRingArray")
{
    using fram::ram::memory;

    fram::ram::SetAllDoFunctions();
    fram::Initialize();
    static constexpr auto charRingArray1StartAddress = value_of(indexesSize);
    static constexpr auto sRingArrayStartAddress =
        value_of(charRingArray2.section.end + indexesSize);

    // FRAM is working
    {
        memory.fill(0x00_b);
        fram::framIsWorking.Store(true);

        CHECK(charRingArray1.Size() == 0U);
        // Reading from an empty ring prints a debug message and returns a default-constructed value
        CHECK(charRingArray1.Front() == 0);
        CHECK(charRingArray1.Back() == 0);
        CHECK(charRingArray1.Get(0) == 0);
        // Trying to set an element in an empty ring prints a debug message and does not set
        // anything
        charRingArray1.Set(0, 11);
        CHECK(std::all_of(memory.begin(), memory.end(), [](auto x) { return x == 0_b; }));

        charRingArray1.PushBack(11);
        CHECK(charRingArray1.Size() == 1U);
        CHECK(charRingArray1.Front() == 11);
        CHECK(charRingArray1.Back() == 11);

        charRingArray1.PushBack(12);
        CHECK(charRingArray1.Size() == 2U);
        CHECK(charRingArray1.Front() == 11);
        CHECK(charRingArray1.Back() == 12);

        charRingArray1.PushBack(13);
        CHECK(charRingArray1.Size() == 3U);
        CHECK(charRingArray1.Front() == 11);
        CHECK(charRingArray1.Back() == 13);
        CHECK(charRingArray1.Get(0) == 11);
        CHECK(charRingArray1.Get(1) == 12);
        CHECK(charRingArray1.Get(2) == 13);

        // PushBack() writes to memory
        CHECK(fram::ram::memory[charRingArray1StartAddress + 0] == 11_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 1] == 12_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 2] == 13_b);

        // When pushing to a full ring, the size stays the same and the oldest element is lost
        charRingArray1.PushBack(14);
        CHECK(charRingArray1.Size() == 3U);
        CHECK(charRingArray1.Front() == 12);
        CHECK(charRingArray1.Back() == 14);

        // Only the (size + 2)th element overwrites the first one in memory because we keep a gap of
        // one between begin and end indexes
        charRingArray1.PushBack(15);
        CHECK(charRingArray1.Size() == 3U);
        CHECK(charRingArray1.Front() == 13);
        CHECK(charRingArray1.Back() == 15);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 0] == 15_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 1] == 12_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 2] == 13_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 3] == 14_b);

        // Set() writes to memory
        charRingArray1.Set(0, 21);
        charRingArray1.Set(1, 22);
        // Set() with an out-of-bounds index for the cache prints a debug message and does not write
        // to the cache
        charRingArray1.Set(2, 23);
        CHECK(charRingArray1.Get(0) == 21);
        CHECK(charRingArray1.Get(1) == 22);
        CHECK(charRingArray1.Get(2) == 23);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 0] == 23_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 1] == 12_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 2] == 21_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 3] == 22_b);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        CHECK(charRingArray1.Get(17) == 23);
        // Set() with out-of-bounds index prints a debug message and does not set anything
        charRingArray1.Set(17, 0);
        CHECK(charRingArray1.Get(0) == 21);
        CHECK(charRingArray1.Get(1) == 22);
        CHECK(charRingArray1.Get(2) == 23);

        charRingArray1.FindAndReplace([](auto x) { return x == 22; }, 42);
        CHECK(charRingArray1.Get(0) == 21);
        CHECK(charRingArray1.Get(1) == 42);
        CHECK(charRingArray1.Get(2) == 23);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 3] == 42_b);

        charRingArray1.FindAndReplace([](auto x) { return x == 0; }, 1);
        CHECK(charRingArray1.Get(0) == 21);
        CHECK(charRingArray1.Get(1) == 42);
        CHECK(charRingArray1.Get(2) == 23);
    }

    // FRAM is not working
    {
        memory.fill(0x00_b);
        fram::framIsWorking.Store(false);

        // Even though we reset the FRAM memory to zero, the cached values are still there
        CHECK(charRingArray1.Size() == charRingArray1.CacheCapacity());
        CHECK(charRingArray1.Get(0) == 42);
        CHECK(charRingArray1.Get(1) == 23);

        CHECK(charRingArray2.Size() == 0U);
        // Reading from an empty ring prints a debug message and returns a default-constructed value
        CHECK(charRingArray2.Front() == 0);
        CHECK(charRingArray2.Back() == 0);
        CHECK(charRingArray2.Get(0) == 0);
        // Trying to set an element in an empty ring only prints a debug message
        charRingArray2.Set(0, 11);

        charRingArray2.PushBack(11);
        CHECK(charRingArray2.Size() == 1U);
        CHECK(charRingArray2.Front() == 11);
        CHECK(charRingArray2.Back() == 11);

        charRingArray2.PushBack(12);
        CHECK(charRingArray2.Size() == 2U);
        CHECK(charRingArray2.Front() == 11);
        CHECK(charRingArray2.Back() == 12);

        CHECK(charRingArray2.Get(0) == 11);
        CHECK(charRingArray2.Get(1) == 12);

        // PushBack() does not write to memory
        CHECK(fram::ram::memory[charRingArray1StartAddress + 0] == 0_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 1] == 0_b);

        // When pushing to a full ring, the size stays the same and the oldest element is lost
        charRingArray2.PushBack(13);
        CHECK(charRingArray2.Size() == 2U);
        CHECK(charRingArray2.Front() == 12);
        CHECK(charRingArray2.Back() == 13);

        // Set() does not write to memory
        charRingArray2.Set(0, 21);
        charRingArray2.Set(1, 22);
        CHECK(charRingArray2.Get(0) == 21);
        CHECK(charRingArray2.Get(1) == 22);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 0] == 0_b);
        CHECK(fram::ram::memory[charRingArray1StartAddress + 1] == 0_b);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        CHECK(charRingArray2.Get(17) == 22);
        // Set() with out-of-bounds index prints a debug message and does not set anything
        charRingArray2.Set(17, 0);
        CHECK(charRingArray2.Get(0) == 21);
        CHECK(charRingArray2.Get(1) == 22);

        charRingArray2.FindAndReplace([](auto x) { return x == 22; }, 42);
        CHECK(charRingArray2.Get(0) == 21);
        CHECK(charRingArray2.Get(1) == 42);

        charRingArray2.FindAndReplace([](auto x) { return x == 0; }, 1);
        CHECK(charRingArray2.Get(0) == 21);
        CHECK(charRingArray2.Get(1) == 42);
    }

    // FramRingArray of custom type
    {
        memory.fill(0x00_b);
        fram::framIsWorking.Store(true);

        auto s1 = S{.u16 = 1, .i32 = 1, .u8 = 1};
        auto s2 = S{.u16 = 2, .i32 = 2, .u8 = 2};
        auto s3 = S{.u16 = 3, .i32 = 3, .u8 = 3};
        auto s4 = S{.u16 = 4, .i32 = 4, .u8 = 4};
        auto s5 = S{.u16 = 5, .i32 = 5, .u8 = 5};
        auto s6 = S{.u16 = 6, .i32 = 6, .u8 = 6};
        auto s7 = S{.u16 = 7, .i32 = 7, .u8 = 7};
        auto s8 = S{.u16 = 8, .i32 = 8, .u8 = 8};

        CHECK(sRingArray.Size() == 0U);
        // Reading from an empty ring prints a debug message and returns a default-constructed value
        CHECK(sRingArray.Front() == S{});
        CHECK(sRingArray.Back() == S{});
        CHECK(sRingArray.Get(0) == S{});
        // Trying to set an element in an empty ring prints a debug message and does not set
        // anything
        sRingArray.Set(0, s1);
        CHECK(std::all_of(memory.begin(), memory.end(), [](auto x) { return x == 0_b; }));

        sRingArray.PushBack(s1);
        CHECK(sRingArray.Size() == 1U);
        CHECK(sRingArray.Front() == s1);
        CHECK(sRingArray.Back() == s1);

        sRingArray.PushBack(s2);
        CHECK(sRingArray.Size() == 2U);
        CHECK(sRingArray.Front() == s1);
        CHECK(sRingArray.Back() == s2);

        sRingArray.PushBack(s3);
        CHECK(sRingArray.Size() == 3U);
        CHECK(sRingArray.Front() == s1);
        CHECK(sRingArray.Back() == s3);

        CHECK(sRingArray.Get(0) == s1);
        CHECK(sRingArray.Get(1) == s2);
        CHECK(sRingArray.Get(2) == s3);

        // PushBack() writes to memory
        CHECK(fram::ram::memory[sRingArrayStartAddress + 1] == 1_b);
        CHECK(fram::ram::memory[sRingArrayStartAddress + 5] == 1_b);
        CHECK(fram::ram::memory[sRingArrayStartAddress + 6] == 1_b);

        // When pushing to a full ring, the size stays the same and the oldest element is lost
        sRingArray.PushBack(s4);
        CHECK(sRingArray.Size() == 3U);
        CHECK(sRingArray.Front() == s2);
        CHECK(sRingArray.Back() == s4);

        // Only the (size + 2)th element overwrites the first one in memory because we keep a gap
        // of one between begin and end indexes
        sRingArray.PushBack(s5);
        CHECK(sRingArray.Size() == 3U);
        CHECK(sRingArray.Front() == s3);
        CHECK(sRingArray.Back() == s5);

        // Set() writes to memory
        sRingArray.Set(0, s6);
        sRingArray.Set(1, s7);
        sRingArray.Set(2, s8);
        CHECK(sRingArray.Get(0) == s6);
        CHECK(sRingArray.Get(1) == s7);
        CHECK(sRingArray.Get(2) == s8);

        CHECK(fram::ram::memory[sRingArrayStartAddress + 1] == 8_b);
        CHECK(fram::ram::memory[sRingArrayStartAddress + 1 + totalSerialSize<S>] == 2_b);
        CHECK(fram::ram::memory[sRingArrayStartAddress + 1 + 2 * totalSerialSize<S>] == 6_b);
        CHECK(fram::ram::memory[sRingArrayStartAddress + 1 + 3 * totalSerialSize<S>] == 7_b);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        CHECK(sRingArray.Get(17) == s8);
        // Set() with out-of-bounds index prints a debug message and does not set anything
        sRingArray.Set(17, s1);
        CHECK(sRingArray.Get(0) == s6);
        CHECK(sRingArray.Get(1) == s7);
        CHECK(sRingArray.Get(2) == s8);

        sRingArray.FindAndReplace([](auto const & x) { return x.u16 == 6; }, s2);
        CHECK(sRingArray.Get(0) == s2);
        CHECK(sRingArray.Get(1) == s7);
        CHECK(sRingArray.Get(2) == s8);
        CHECK(fram::ram::memory[sRingArrayStartAddress + 1 + 2 * totalSerialSize<S>] == 2_b);

        sRingArray.FindAndReplace([](auto const & x) { return x.i32 == 32; }, s1);
        CHECK(sRingArray.Get(0) == s2);
        CHECK(sRingArray.Get(1) == s7);
        CHECK(sRingArray.Get(2) == s8);
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

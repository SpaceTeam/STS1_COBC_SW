#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FramSections/RingArray.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <strong_type/equality.hpp>
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


inline constexpr auto charSection =
    sts1cobcsw::Section<fram::Address(0), fram::Size(3 * 2 * sizeof(std::size_t) + 4)>{};
inline constexpr auto sSection =
    sts1cobcsw::Section<charSection.end,
                        fram::Size(3 * 2 * sizeof(std::size_t) + 4 * totalSerialSize<S>)>{};

inline constexpr auto charRingArray = sts1cobcsw::RingArray<char, charSection, 2>{};
inline constexpr auto sRingArray = sts1cobcsw::RingArray<S, sSection, 2>{};


static_assert(std::is_same_v<decltype(charRingArray)::ValueType, char>);
static_assert(charRingArray.FramCapacity() == 3);
static_assert(charRingArray.CacheCapacity() == 2);
static_assert(charRingArray.section.begin == fram::Address(0));
static_assert(charRingArray.section.size == fram::Size(28));
static_assert(charRingArray.section.end == fram::Address(28));

static_assert(std::is_same_v<decltype(sRingArray)::ValueType, S>);
static_assert(sRingArray.FramCapacity() == 3);
static_assert(sRingArray.CacheCapacity() == 2);
static_assert(sRingArray.section.begin == fram::Address(28));
static_assert(sRingArray.section.size == fram::Size(52));
static_assert(sRingArray.section.end == fram::Address(80));


auto RunUnitTest() -> void
{
    using fram::ram::memory;

    fram::ram::SetAllDoFunctions();
    fram::Initialize();
    static constexpr auto charRingArrayStartAddress = 3 * 2 * sizeof(std::size_t);
    static constexpr auto sRingArrayStartAddress =
        value_of(charRingArray.section.end) + 3 * 2 * sizeof(std::size_t);

    // SECTION("FRAM is working")
    {
        memory.fill(0x00_b);
        fram::framIsWorking.Store(true);

        Require(charRingArray.Size() == 0);

        // Trying to set an element in an empty ring prints a debug message and does not set
        // anything
        charRingArray.Set(0, 11);
        Require(std::all_of(memory.begin(), memory.end(), [](auto x) { return x == 0_b; }));

        charRingArray.PushBack(11);
        Require(charRingArray.Size() == 1);
        Require(charRingArray.Front() == 11);
        Require(charRingArray.Back() == 11);

        charRingArray.PushBack(12);
        Require(charRingArray.Size() == 2);
        Require(charRingArray.Front() == 11);
        Require(charRingArray.Back() == 12);

        charRingArray.PushBack(13);
        Require(charRingArray.Size() == 3);
        Require(charRingArray.Front() == 11);
        Require(charRingArray.Back() == 13);
        Require(charRingArray.Get(0) == 11);
        Require(charRingArray.Get(1) == 12);
        Require(charRingArray.Get(2) == 13);

        // PushBack() writes to memory
        Require(fram::ram::memory[charRingArrayStartAddress + 0] == 11_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 1] == 12_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 2] == 13_b);

        // When pushing to a full ring, the size stays the same and the oldest element is lost
        charRingArray.PushBack(14);
        Require(charRingArray.Size() == 3);
        Require(charRingArray.Front() == 12);
        Require(charRingArray.Back() == 14);

        // Only the (size + 2)th element overwrites the first one in memory because we keep a gap of
        // one between begin and end indexes
        charRingArray.PushBack(15);
        Require(charRingArray.Size() == 3);
        Require(charRingArray.Front() == 13);
        Require(charRingArray.Back() == 15);
        Require(fram::ram::memory[charRingArrayStartAddress + 0] == 15_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 1] == 12_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 2] == 13_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 3] == 14_b);

        // Set() writes to memory
        charRingArray.Set(0, 21);
        charRingArray.Set(1, 22);
        charRingArray.Set(2, 23);
        Require(charRingArray.Get(0) == 21);
        Require(charRingArray.Get(1) == 22);
        Require(charRingArray.Get(2) == 23);
        Require(fram::ram::memory[charRingArrayStartAddress + 0] == 23_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 1] == 12_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 2] == 21_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 3] == 22_b);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        Require(charRingArray.Get(17) == 23);
        // Set() with out-of-bounds index prints a debug message and does not set anything
        charRingArray.Set(17, 0);
        Require(charRingArray.Get(0) == 21);
        Require(charRingArray.Get(1) == 22);
        Require(charRingArray.Get(2) == 23);
    }

    // SECTION("FRAM is not working")
    {
        memory.fill(0x00_b);
        fram::framIsWorking.Store(false);

        Require(charRingArray.Size() == 0);
        // Trying to set an element in an empty ring prints a debug message
        charRingArray.Set(0, 11);

        charRingArray.PushBack(11);
        Require(charRingArray.Size() == 1);
        Require(charRingArray.Front() == 11);
        Require(charRingArray.Back() == 11);

        charRingArray.PushBack(12);
        Require(charRingArray.Size() == 2);
        Require(charRingArray.Front() == 11);
        Require(charRingArray.Back() == 12);

        Require(charRingArray.Get(0) == 11);
        Require(charRingArray.Get(1) == 12);

        // PushBack() does not write to memory
        Require(fram::ram::memory[charRingArrayStartAddress + 0] == 0_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 1] == 0_b);

        // When pushing to a full ring, the size stays the same and the oldest element is lost
        charRingArray.PushBack(13);
        Require(charRingArray.Size() == 2);
        Require(charRingArray.Front() == 12);
        Require(charRingArray.Back() == 13);

        // Set() does not write to memory
        charRingArray.Set(0, 21);
        charRingArray.Set(1, 22);
        Require(charRingArray.Get(0) == 21);
        Require(charRingArray.Get(1) == 22);
        Require(fram::ram::memory[charRingArrayStartAddress + 0] == 0_b);
        Require(fram::ram::memory[charRingArrayStartAddress + 1] == 0_b);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        Require(charRingArray.Get(17) == 22);
        // Set() with out-of-bounds index prints a debug message and does not set anything
        charRingArray.Set(17, 0);
        Require(charRingArray.Get(0) == 21);
        Require(charRingArray.Get(1) == 22);
    }

    // SECTION("RingArray of custom type")
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

        Require(sRingArray.Size() == 0);

        // Trying to set an element in an empty ring prints a debug message and does not set
        // anything
        sRingArray.Set(0, s1);
        Require(std::all_of(memory.begin(), memory.end(), [](auto x) { return x == 0_b; }));

        sRingArray.PushBack(s1);
        Require(sRingArray.Size() == 1);
        Require(sRingArray.Front() == s1);
        Require(sRingArray.Back() == s1);

        sRingArray.PushBack(s2);
        Require(sRingArray.Size() == 2);
        Require(sRingArray.Front() == s1);
        Require(sRingArray.Back() == s2);

        sRingArray.PushBack(s3);
        Require(sRingArray.Size() == 3);
        Require(sRingArray.Front() == s1);
        Require(sRingArray.Back() == s3);

        Require(sRingArray.Get(0) == s1);
        Require(sRingArray.Get(1) == s2);
        Require(sRingArray.Get(2) == s3);

        // PushBack() writes to memory
        Require(fram::ram::memory[sRingArrayStartAddress + 0] == 1_b);
        Require(fram::ram::memory[sRingArrayStartAddress + sizeof(S::u16)] == 1_b);
        Require(fram::ram::memory[sRingArrayStartAddress + sizeof(S::u16) + sizeof(S::i32)] == 1_b);

        // When pushing to a full ring, the size stays the same and the oldest element is lost
        sRingArray.PushBack(s4);
        Require(sRingArray.Size() == 3);
        Require(sRingArray.Front() == s2);
        Require(sRingArray.Back() == s4);

        // Only the (size + 2)th element overwrites the first one in memory because we keep a gap
        // of one between begin and end indexes
        sRingArray.PushBack(s5);
        Require(sRingArray.Size() == 3);
        Require(sRingArray.Front() == s3);
        Require(sRingArray.Back() == s5);

        // Set() writes to memory
        sRingArray.Set(0, s6);
        sRingArray.Set(1, s7);
        sRingArray.Set(2, s8);
        Require(sRingArray.Get(0) == s6);
        Require(sRingArray.Get(1) == s7);
        Require(sRingArray.Get(2) == s8);

        Require(fram::ram::memory[sRingArrayStartAddress + 0] == 8_b);
        Require(fram::ram::memory[sRingArrayStartAddress + totalSerialSize<S>] == 2_b);
        Require(fram::ram::memory[sRingArrayStartAddress + 2 * totalSerialSize<S>] == 6_b);
        Require(fram::ram::memory[sRingArrayStartAddress + 3 * totalSerialSize<S>] == 7_b);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        Require(sRingArray.Get(17) == s8);
        // Set() with out-of-bounds index prints a debug message and does not set anything
        sRingArray.Set(17, s1);
        Require(sRingArray.Get(0) == s6);
        Require(sRingArray.Get(1) == s7);
        Require(sRingArray.Get(2) == s8);
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

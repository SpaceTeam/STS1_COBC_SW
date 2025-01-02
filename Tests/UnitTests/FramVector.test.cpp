#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

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

// Define FRAM sections for testing
inline constexpr auto charSection1 =
    sts1cobcsw::Section<fram::Address(0),
                        fram::Size(3 * 4 + 5 * totalSerialSize<char>)>{};  // Adjust size as needed

inline constexpr auto charSection2 =
    sts1cobcsw::Section<fram::Address(charSection1.end),
                        fram::Size(3 * 4 + 5 * totalSerialSize<char>)>{};  // Adjust size as needed
inline constexpr auto sSection =
    sts1cobcsw::Section<charSection2.end, fram::Size(3 * 4 + 4 * totalSerialSize<S>)>{};

// Instantiate FramVector with different configurations
inline constexpr auto charVector1 = sts1cobcsw::FramVector<char, charSection1, 2>{};
inline constexpr auto charVector2 = sts1cobcsw::FramVector<char, charSection2, 2>{};
inline constexpr auto sVector = sts1cobcsw::FramVector<S, sSection, 4>{};

static constexpr auto charVector1StartAddress = 3 * 4;
static constexpr auto charVector2StartAddress = value_of(charVector1.section.end) + 3 * 4;

// Static assertions to ensure correct configurations
static_assert(std::is_same_v<decltype(charVector1)::ValueType, char>);
static_assert(charVector1.FramCapacity() == 5);
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

    // SECTION("Initialization")
    {
        Require(charVector1.Size() == 0);
        Require(not charVector1.Full());
        Require(charVector1.Empty());
        Require(charVector2.Size() == 0);
        Require(sVector.Size() == 0);
    }

    // SECTION("Push operations")
    {
        charVector1.PushBack('a');
        Require(charVector1.Size() == 1);
        Require(charVector1.Get(0) == 'a');
        Require(not charVector1.Empty());

        charVector1.PushBack('b');
        Require(charVector1.Size() == 2);
        Require(charVector1.Get(1) == 'b');

        Require(fram::ram::memory[charVector1StartAddress + 0] == 0x61_b);
        Require(fram::ram::memory[charVector1StartAddress + 1] == 0x62_b);

        charVector1.PushBack('c');
        charVector1.PushBack('d');
        charVector1.PushBack('e');
        Require(charVector1.Size() == 5);
        Require(charVector1.Full());
        Require(not charVector1.Empty());
        Require(charVector1.Get(4) == 'e');

        // Attempt to PushBack() beyond cache capacity (should print a debug message)
        charVector1.PushBack('f');
        Require(charVector1.Size() == 5);
        Require(charVector1.Get(4) == 'e');
    }


    // SECTION("Custom Type")
    {
        // Test PushBack() for custom type S
        S s1{.u16 = 1, .i32 = 100, .u8 = 10};
        S s2{.u16 = 2, .i32 = 200, .u8 = 20};
        S s3{.u16 = 3, .i32 = 300, .u8 = 30};
        S s4{.u16 = 4, .i32 = 400, .u8 = 40};
        S s5{.u16 = 5, .i32 = 500, .u8 = 50};

        sVector.PushBack(s1);
        Require(sVector.Size() == 1);
        Require(sVector.Get(0) == s1);

        sVector.PushBack(s2);
        Require(sVector.Size() == 2);
        Require(sVector.Get(0) == s1);

        sVector.PushBack(s3);
        sVector.PushBack(s4);
        Require(sVector.Size() == 4);
        Require(sVector.Full());

        sVector.PushBack(s5);
        Require(sVector.Size() == 4);
        Require(sVector.Get(3) == s4);
    }

    // SECTION("FRAM is not working")
    {
        memory.fill(0x00_b);
        fram::framIsWorking.Store(false);

        // Even though we reset the FRAM memory to zero, the cached values are still there
        Require(charVector1.Size() == charVector1.CacheCapacity());
        Require(charVector1.Get(0) == 'a');
        Require(charVector1.Get(1) == 'b');

        Require(charVector2.Size() == 0);
        charVector2.PushBack(11);
        Require(charVector2.Size() == 1);

        charVector2.PushBack(12);
        Require(charVector2.Size() == 2);
        Require(charVector2.Get(0) == 11);
        Require(charVector2.Get(1) == 12);

        // PushBack() does not write to memory
        Require(fram::ram::memory[charVector2StartAddress + 0] == 0x00_b);
        Require(fram::ram::memory[charVector2StartAddress + 1] == 0x00_b);
        Require(fram::ram::memory[charVector2StartAddress + 1] == 0x00_b);
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

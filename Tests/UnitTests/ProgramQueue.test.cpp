#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FramSections/ProgramQueue.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <bit>

#include "Sts1CobcSw/Serial/Serial.hpp"
#include "Sts1CobcSw/Utility/Debug.hpp"


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
inline constexpr auto dummySection =
    sts1cobcsw::Section<fram::Address(0),
                        fram::Size(3 * 4 + 5 * totalSerialSize<char>)>{};  // Adjust size as needed
inline constexpr auto sSection =
    sts1cobcsw::Section<dummySection.end, fram::Size(3 * 4 + 4 * totalSerialSize<S>)>{};


// Instantiate ProgramQueue with different configurations
inline constexpr auto programQueueChar = sts1cobcsw::ProgramQueue<char, dummySection, 5>{};
inline constexpr auto programQueueS = sts1cobcsw::ProgramQueue<S, sSection, 4>{};

// Static assertions to ensure correct configurations
static_assert(std::is_same_v<decltype(programQueueChar)::ValueType, char>);
static_assert(programQueueChar.FramCapacity() == 5);
static_assert(programQueueChar.CacheCapacity() == 5);
static_assert(programQueueS.FramCapacity() == 4);
static_assert(programQueueS.CacheCapacity() == 4);


auto RunUnitTest() -> void
{
    using fram::ram::memory;
    fram::ram::SetAllDoFunctions();

    Require(programQueueChar.Size() == 0);
    Require(not programQueueChar.Full());
    Require(programQueueChar.Empty());

    fram::framIsWorking.Store(true);
    memory.fill(0x00_b);  // Clear memory


    // Test PushBack for char
    DEBUG_PRINT("Capcity, %u", programQueueChar.FramCapacity());
    Require(programQueueChar.PushBack('a'));
    Require(programQueueChar.Size() == 1);
    Require(programQueueChar.Get(0) == 'a');

    Require(programQueueChar.PushBack('b'));
    Require(programQueueChar.Size() == 2);
    Require(programQueueChar.Get(1) == 'b');


    // Fill cache
    Require(programQueueChar.PushBack('c'));
    Require(programQueueChar.PushBack('d'));
    Require(programQueueChar.PushBack('e'));
    Require(programQueueChar.Size() == 5);
    Require(programQueueChar.Full());
    Require(not programQueueChar.Empty());
    Require(programQueueChar.Get(4) == 'e');

    // Attempt to PushBack beyond cache capacity (should fail)
    Require(not programQueueChar.PushBack('f'));
    Require(programQueueChar.Size() == 5);
    Require(programQueueChar.Get(4) == 'e');

    // Test PushBack for custom type S
    S s1{.u16 = 1, .i32 = 100, .u8 = 10};
    S s2{.u16 = 2, .i32 = 200, .u8 = 20};
    S s3{.u16 = 3, .i32 = 300, .u8 = 30};
    S s4{.u16 = 4, .i32 = 400, .u8 = 40};
    S s5{.u16 = 5, .i32 = 500, .u8 = 50};

    Require(programQueueS.PushBack(s1));
    Require(programQueueS.Size() == 1);
    Require(programQueueS.Get(0) == s1);

    Require(programQueueS.PushBack(s2));
    Require(programQueueS.Size() == 2);
    Require(programQueueS.Get(0) == s1);

    Require(programQueueS.PushBack(s3));
    Require(programQueueS.PushBack(s4));
    Require(programQueueS.Size() == 4);
    Require(programQueueS.Full());
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

#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Periphery/FramRingArray.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <strong_type/equality.hpp>
#include <strong_type/type.hpp>

#include <array>
#include <cstddef>
#include <type_traits>


namespace fram = sts1cobcsw::fram;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


inline constexpr auto section =
    sts1cobcsw::Section<fram::Address(0), fram::Size(3 * 2 * sizeof(std::size_t) + 4)>{};
inline constexpr auto charRingArray = sts1cobcsw::RingArray<char, section>{};

static_assert(std::is_same_v<decltype(charRingArray)::ValueType, char>);
static_assert(charRingArray.Capacity() == 3);
static_assert(charRingArray.section.begin == fram::Address(0));
static_assert(charRingArray.section.end == fram::Address(28));
static_assert(charRingArray.section.size == fram::Size(28));


auto RunUnitTest() -> void
{
    fram::ram::SetAllDoFunctions();
    fram::ram::memory.fill(0x00_b);
    fram::Initialize();

    Require(charRingArray.Size() == 0);

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

    // Out-of-bounds access returns the last element and prints a debug message
    Require(charRingArray.Get(17) == 13);

    // PushBack writes to memory
    constexpr auto ringArrayStartAddress = 3 * 2 * sizeof(std::size_t);
    Require(fram::ram::memory[ringArrayStartAddress] == 11_b);
    Require(fram::ram::memory[ringArrayStartAddress + 1] == 12_b);
    Require(fram::ram::memory[ringArrayStartAddress + 2] == 13_b);

    // When pushing to a full ring, the size stays the same and the oldest element is lost
    charRingArray.PushBack(14);
    Require(charRingArray.Size() == 3);
    Require(charRingArray.Front() == 12);
    Require(charRingArray.Back() == 14);

    // Only the (size + 2)th element overwrites the first one in memory because we keep a gap of one
    // between begin and end indexes
    charRingArray.PushBack(15);
    Require(charRingArray.Size() == 3);
    Require(charRingArray.Front() == 13);
    Require(charRingArray.Back() == 15);
    Require(fram::ram::memory[ringArrayStartAddress] == 15_b);
    Require(fram::ram::memory[ringArrayStartAddress + 1] == 12_b);
    Require(fram::ram::memory[ringArrayStartAddress + 2] == 13_b);
    Require(fram::ram::memory[ringArrayStartAddress + 3] == 14_b);

    // TODO: Add tests with custom types
}

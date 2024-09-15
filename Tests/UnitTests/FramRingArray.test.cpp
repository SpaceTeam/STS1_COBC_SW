#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <strong_type/equality.hpp>
#include <strong_type/type.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>


namespace fram = sts1cobcsw::fram;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)


inline constexpr auto section =
    sts1cobcsw::Section<fram::Address(0), fram::Size(3 * 2 * sizeof(std::size_t) + 4)>{};
inline constexpr auto charRingArray = sts1cobcsw::RingArray<char, section, 2>{};

static_assert(std::is_same_v<decltype(charRingArray)::ValueType, char>);
static_assert(charRingArray.FramCapacity() == 3);
static_assert(charRingArray.CacheCapacity() == 2);
static_assert(charRingArray.section.begin == fram::Address(0));
static_assert(charRingArray.section.end == fram::Address(28));
static_assert(charRingArray.section.size == fram::Size(28));


auto RunUnitTest() -> void
{
    using fram::ram::memory;

    fram::ram::SetAllDoFunctions();
    fram::Initialize();
    static constexpr auto ringArrayStartAddress = 3 * 2 * sizeof(std::size_t);


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

        // PushBack writes to memory
        Require(fram::ram::memory[ringArrayStartAddress + 0] == 11_b);
        Require(fram::ram::memory[ringArrayStartAddress + 1] == 12_b);
        Require(fram::ram::memory[ringArrayStartAddress + 2] == 13_b);

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
        Require(fram::ram::memory[ringArrayStartAddress + 0] == 15_b);
        Require(fram::ram::memory[ringArrayStartAddress + 1] == 12_b);
        Require(fram::ram::memory[ringArrayStartAddress + 2] == 13_b);
        Require(fram::ram::memory[ringArrayStartAddress + 3] == 14_b);

        // Set() writes to memory
        charRingArray.Set(0, 21);
        charRingArray.Set(1, 22);
        charRingArray.Set(2, 23);
        Require(charRingArray.Get(0) == 21);
        Require(charRingArray.Get(1) == 22);
        Require(charRingArray.Get(2) == 23);
        Require(fram::ram::memory[ringArrayStartAddress + 0] == 23_b);
        Require(fram::ram::memory[ringArrayStartAddress + 1] == 12_b);
        Require(fram::ram::memory[ringArrayStartAddress + 2] == 21_b);
        Require(fram::ram::memory[ringArrayStartAddress + 3] == 22_b);

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

        // PushBack does not write to memory
        Require(fram::ram::memory[ringArrayStartAddress + 0] == 0_b);
        Require(fram::ram::memory[ringArrayStartAddress + 1] == 0_b);

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
        Require(fram::ram::memory[ringArrayStartAddress + 0] == 0_b);
        Require(fram::ram::memory[ringArrayStartAddress + 1] == 0_b);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        Require(charRingArray.Get(17) == 22);
        // Set() with out-of-bounds index prints a debug message and does not set anything
        charRingArray.Set(17, 0);
        Require(charRingArray.Get(0) == 21);
        Require(charRingArray.Get(1) == 22);
    }

    // TODO: Add tests with custom types
}

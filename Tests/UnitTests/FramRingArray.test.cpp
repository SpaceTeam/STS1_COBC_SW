#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/FramSections/RingArray.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

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

inline constexpr auto programStatusHistorySection = sts1cobcsw::Section<
    fram::Address(3 * 2 * sizeof(std::size_t) + 4),
    fram::Size(3 * 2 * sizeof(std::size_t)
               + sts1cobcsw::serialSize<sts1cobcsw::edu::ProgramStatusHistoryEntry> * 4)>{};

inline constexpr auto charRingArray = sts1cobcsw::RingArray<char, section, 2>{};
inline constexpr auto programStatusHistory = sts1cobcsw::
    RingArray<sts1cobcsw::edu::ProgramStatusHistoryEntry, programStatusHistorySection, 2>{};

static_assert(std::is_same_v<decltype(charRingArray)::ValueType, char>);
static_assert(charRingArray.FramCapacity() == 3);
static_assert(charRingArray.CacheCapacity() == 2);
static_assert(charRingArray.section.begin == fram::Address(0));
static_assert(charRingArray.section.end == fram::Address(28));
static_assert(charRingArray.section.size == fram::Size(28));

static_assert(std::is_same_v<decltype(programStatusHistory)::ValueType,
                             sts1cobcsw::edu::ProgramStatusHistoryEntry>);
static_assert(programStatusHistory.FramCapacity() == 3);
static_assert(programStatusHistory.CacheCapacity() == 2);
static_assert(programStatusHistory.section.begin == fram::Address(28));
static_assert(
    programStatusHistory.section.end
    == fram::Address(28 + 3 * 2 * sizeof(size_t)
                     + 4 * sts1cobcsw::serialSize<sts1cobcsw::edu::ProgramStatusHistoryEntry>));
static_assert(
    programStatusHistory.section.size
    == fram::Size(3 * 2 * sizeof(size_t)
                  + 4 * sts1cobcsw::serialSize<sts1cobcsw::edu::ProgramStatusHistoryEntry>));


auto RunUnitTest() -> void
{
    using fram::ram::memory;

    fram::ram::SetAllDoFunctions();
    fram::Initialize();
    static constexpr auto ringArrayStartAddress = 3 * 2 * sizeof(std::size_t);
    static constexpr auto programStatusHistoryStartAddress =
        3 * 2 * sizeof(std::size_t) + 4 + 3 * 2 * sizeof(std::size_t);


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
    // We need to provide serialization functions

    // SECTION("ProgramStatusHistoryEntry test")
    {
        using sts1cobcsw::serialSize;
        using sts1cobcsw::edu::ProgramStatusHistoryEntry;

        using sts1cobcsw::ProgramId;
        using sts1cobcsw::RealTime;
        using sts1cobcsw::edu::ProgramStatus;


        memory.fill(0x00_b);
        fram::framIsWorking.Store(true);

        Require(programStatusHistory.Size() == 0);

        // NOTE:
        //      ProgramId     : 2 bytes
        //      RealTime      : 4 bytes
        //      ProgramStatus : 1 byte
        auto entry1 = ProgramStatusHistoryEntry{.programId = ProgramId(1),
                                                .startTime = RealTime(11),
                                                .status = ProgramStatus::programCouldNotBeStarted};

        auto entry2 = ProgramStatusHistoryEntry{.programId = ProgramId(2),
                                                .startTime = RealTime(12),
                                                .status = ProgramStatus::programRunning};

        auto entry3 = ProgramStatusHistoryEntry{.programId = ProgramId(3),
                                                .startTime = RealTime(13),
                                                .status = ProgramStatus::programRunning};

        auto entry4 = ProgramStatusHistoryEntry{.programId = ProgramId(4),
                                                .startTime = RealTime(10),
                                                .status = ProgramStatus::programRunning};

        auto entry5 = ProgramStatusHistoryEntry{.programId = ProgramId(5),
                                                .startTime = RealTime(10),
                                                .status = ProgramStatus::programRunning};

        auto entry6 = ProgramStatusHistoryEntry{.programId = ProgramId(6),
                                                .startTime = RealTime(10),
                                                .status = ProgramStatus::programRunning};

        auto entry7 = ProgramStatusHistoryEntry{.programId = ProgramId(7),
                                                .startTime = RealTime(10),
                                                .status = ProgramStatus::programRunning};

        auto entry8 = ProgramStatusHistoryEntry{.programId = ProgramId(8),
                                                .startTime = RealTime(10),
                                                .status = ProgramStatus::programRunning};


        Require(entry1.programId == sts1cobcsw::ProgramId{1});

        Require(programStatusHistory.Size() == 0);

        // Trying to set an element in an empty ring prints a debug message and does not set
        // anything
        programStatusHistory.Set(0, entry1);
        Require(std::all_of(memory.begin(), memory.end(), [](auto x) { return x == 0_b; }));

        programStatusHistory.PushBack(entry1);
        Require(programStatusHistory.Size() == 1);
        Require(programStatusHistory.Front().programId == sts1cobcsw::ProgramId(1));
        Require(programStatusHistory.Back().programId == sts1cobcsw::ProgramId(1));

        programStatusHistory.PushBack(entry2);
        Require(programStatusHistory.Size() == 2);
        Require(programStatusHistory.Front().programId.value_of() == 1);
        Require(programStatusHistory.Front().startTime.value_of() == 11);
        Require(programStatusHistory.Front().status == ProgramStatus::programCouldNotBeStarted);
        Require(programStatusHistory.Back().programId.value_of() == 2);
        Require(programStatusHistory.Back().startTime.value_of() == 12);
        Require(programStatusHistory.Back().status == ProgramStatus::programRunning);

        programStatusHistory.PushBack(entry3);
        Require(programStatusHistory.Size() == 3);
        Require(programStatusHistory.Front().programId.value_of() == 1);
        Require(programStatusHistory.Back().programId.value_of() == 3);

        Require(programStatusHistory.Get(0).startTime.value_of() == 11);
        Require(programStatusHistory.Get(1).startTime.value_of() == 12);
        Require(programStatusHistory.Get(2).startTime.value_of() == 13);

        // PushBack writes to memory
        Require(fram::ram::memory[programStatusHistoryStartAddress + 0] == 1_b);
        Require(fram::ram::memory[programStatusHistoryStartAddress + sizeof(sts1cobcsw::ProgramId)]
                == 11_b);
        Require(fram::ram::memory[programStatusHistoryStartAddress + 6] == 1_b);

        // When pushing to a full ring, the size stays the same and the oldest element is lost
        programStatusHistory.PushBack(entry4);
        Require(programStatusHistory.Size() == 3);
        Require(programStatusHistory.Front().programId.value_of() == 2);
        Require(programStatusHistory.Back().programId.value_of() == 4);

        // Only the (size + 2)th element overwrites the first one in memory because we keep a gap
        // of one between begin and end indexes
        programStatusHistory.PushBack(entry5);
        Require(programStatusHistory.Size() == 3);
        Require(programStatusHistory.Front().programId.value_of() == 3);
        Require(programStatusHistory.Back().programId.value_of() == 5);

        // Set() writes to memory
        programStatusHistory.Set(0, entry6);
        programStatusHistory.Set(1, entry7);
        programStatusHistory.Set(2, entry8);
        Require(programStatusHistory.Get(0).programId.value_of() == 6);
        Require(programStatusHistory.Get(1).programId.value_of() == 7);
        Require(programStatusHistory.Get(2).programId.value_of() == 8);

        Require(fram::ram::memory[programStatusHistoryStartAddress + 0] == 8_b);
        Require(fram::ram::memory[programStatusHistoryStartAddress
                                  + serialSize<ProgramStatusHistoryEntry>]
                == 2_b);
        Require(fram::ram::memory[programStatusHistoryStartAddress
                                  + 2 * serialSize<ProgramStatusHistoryEntry>]
                == 6_b);
        Require(fram::ram::memory[programStatusHistoryStartAddress
                                  + 3 * serialSize<ProgramStatusHistoryEntry>]
                == 7_b);

        // Get() with out-of-bounds index prints a debug message and returns the last element
        Require(programStatusHistory.Get(17).programId.value_of() == 8);
        //// Set() with out-of-bounds index prints a debug message and does not set anything
        programStatusHistory.Set(17, entry1);
        Require(programStatusHistory.Get(0).programId.value_of() == 6);
        Require(programStatusHistory.Get(1).programId.value_of() == 7);
        Require(programStatusHistory.Get(2).programId.value_of() == 8);

        // UpdateProgramStatusHistory()
        for(std::size_t i = 0; i < programStatusHistory.Size(); ++i)
        {
            auto entry = programStatusHistory.Get(i);
            if(entry.startTime == sts1cobcsw::RealTime(10)
               and entry.programId == sts1cobcsw::ProgramId(8))
            {
                entry.status = ProgramStatus::programExecutionSucceeded;
                programStatusHistory.Set(i, entry);
            }
        }

        Require(programStatusHistory.Get(2).status == ProgramStatus::programExecutionSucceeded);
    }
}

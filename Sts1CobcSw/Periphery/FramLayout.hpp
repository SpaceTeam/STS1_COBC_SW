#pragma once


#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>


namespace sts1cobcsw::fram
{
// ---- Persistent Variables ----

// Maximum size of a single copy of the persistent variables
inline constexpr auto persistentVariablesSegmentSize = 100;
inline constexpr auto persistentVariablesSize = persistentVariablesSegmentSize * 3;
inline constexpr auto persistentVariablesStartAddress = 0;
inline constexpr auto persistentVariablesEndAddress =
    persistentVariablesStartAddress + persistentVariablesSize;

// The start addresses of the 3 segments for the persistent variables
inline constexpr auto persistentVariablesStartAddress0 = persistentVariablesStartAddress;
inline constexpr auto persistentVariablesStartAddress1 =
    persistentVariablesStartAddress0 + persistentVariablesSegmentSize;
inline constexpr auto persistentVariablesStartAddress2 =
    persistentVariablesStartAddress1 + persistentVariablesSegmentSize;

static_assert(persistentVariablesEndAddress <= framSize, "Persistent variables do not fit in FRAM");
static_assert(persistentVariablesStartAddress2 + persistentVariablesSegmentSize
                  == persistentVariablesEndAddress,
              "Persistent variables segments have the wrong size");


// ---- Test Memory ----

inline constexpr auto testMemorySize = 1000;
inline constexpr auto testMemoryStartAddress = persistentVariablesEndAddress;
inline constexpr auto testMemoryEndAddress = testMemoryStartAddress + testMemorySize;


// ---- EDU Program Queue ----

// Maximum number of programs in the edu program queue
inline constexpr auto nEduProgramQueueEntries = edu::programQueueSize;
inline constexpr auto eduProgramQueueEntrySize = totalSerialSize<edu::QueueEntry>;

inline constexpr auto eduProgramQueueSize = nEduProgramQueueEntries * eduProgramQueueEntrySize;
inline constexpr auto eduProgramQueueStartAddress = testMemoryEndAddress;
inline constexpr auto eduProgramQueueEndAddress = eduProgramQueueStartAddress + eduProgramQueueSize;


// ---- EDU Program Status History ----

inline constexpr auto nEduProgramStatusHistoryEntries = 50;
inline constexpr auto eduProgramStatusHistoryEntrySize =
    totalSerialSize<edu::ProgramStatusHistoryEntry>;

inline constexpr auto eduProgramStatusHistorySize =
    nEduProgramStatusHistoryEntries * eduProgramStatusHistoryEntrySize;
inline constexpr auto eduProgramStatusHistoryStartAddress = eduProgramQueueEndAddress;
inline constexpr auto eduProgramStatusHistoryEndAddress =
    eduProgramStatusHistoryStartAddress + eduProgramStatusHistorySize;


// ---- Telemetry ----

inline constexpr auto telemetryRecordSize = 10;  // TODO: Set correct telemetry entry size
inline constexpr auto telemetryStartAddress = eduProgramStatusHistoryEndAddress;
inline constexpr auto telemetrySize =
    (framSize - telemetryStartAddress) / telemetryRecordSize * telemetryRecordSize;
inline constexpr auto telemetryEndAddress = telemetryStartAddress + telemetrySize;
}

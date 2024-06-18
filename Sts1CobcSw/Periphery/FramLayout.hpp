#pragma once


#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

namespace sts1cobcsw::fram
{

// ---------- General ---------- //

/// The total size of the F-Ram in bytes
inline constexpr auto framSize = 1048576;

// ---------- Persistent Variables ---------- //

/// The maximal size of a single copy of the persistent variables
inline constexpr auto persistentVariablesSegmentSize = 100;

/// The total size of the persisent variable section
inline constexpr auto persistentVariablesSize = persistentVariablesSegmentSize * 3;
/// The start address of the persistent variable section
inline constexpr auto persistentVariablesStart = 0;
/// The exclusive end address of the persistent variable section
inline constexpr auto persistentVariablesEnd = persistentVariablesStart + persistentVariablesSize;

/// The baseaddress of the fist persistent variables copy
inline constexpr auto persistentVariablesAddress0 = persistentVariablesStart;
/// The baseaddress of the second persistent variables copy
inline constexpr auto persistentVariablesAddress1 =
    persistentVariablesAddress0 + persistentVariablesSegmentSize;
/// The baseaddress of the third persistent variables copy
inline constexpr auto persistentVariablesAddress2 =
    persistentVariablesAddress1 + persistentVariablesSegmentSize;

// ---------- Test Memory ---------- //

/// The size of the test memory section
inline constexpr auto testMemorySize = 1000;
/// The start address of the test memory section
inline constexpr auto testMemoryStart = persistentVariablesEnd;
/// The exclusive end address of the test memory section
inline constexpr auto testMemoryEnd = testMemoryStart + testMemorySize;

// ---------- EDU Program Queue ---------- //

/// Maximum number of programs in the edu program queue
inline constexpr auto programQueueCapacity = 20;
/// The size of an entry in the edu program queue
inline constexpr auto programQueueEntrySize = edu::programQueueSize;

/// The size of the edu program queue section
inline constexpr auto programQueueSize = programQueueCapacity * programQueueEntrySize;
/// The start address of the edu program queue section
inline constexpr auto programQueueStart = testMemoryEnd;
/// The exclusive end address of the edu program queue section
inline constexpr auto programQueueEnd = programQueueStart + programQueueSize;

// ---------- EDU Program Status History ---------- //

/// The maximum number of entries in the edu program status history
inline constexpr auto programStatusCapacity = 50;
/// The size of an entry in the edu program status history
inline constexpr auto programStatusEntrySize = serialSize<edu::ProgramStatusHistoryEntry>;

/// The size of the edu program status history section
inline constexpr auto programStatusSize = programStatusCapacity * programStatusEntrySize;
/// The start address of the edu program status history section
inline constexpr auto programStatusStart = programQueueEnd;
/// The exclusive end address of the edu program status history section
inline constexpr auto programStatusEnd = programStatusStart + programStatusSize;

// ---------- Telemetry ---------- //

/// The size of an entry in the telemetry ringbuffer
inline constexpr auto telemetryEntrySize = 10;  // TODO: Set correct telemetry entry size
/// The start address of the telemetry ringbuffer section
inline constexpr auto telemetryStart = programStatusEnd;
/// The maximum number of entries in the telemetry ringbuffer
inline constexpr auto telemetryCapacity = (framSize - telemetryStart) / telemetryEntrySize;
/// The size of an entry in the telemetry ringbuffer
inline constexpr auto telemetrySize = telemetryCapacity * telemetryEntrySize;
/// The exclusive end address of the telemetry ringbuffer section
inline constexpr auto telemetryEnd = telemetryStart + telemetrySize;

}
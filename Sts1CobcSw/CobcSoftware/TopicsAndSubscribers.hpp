#pragma once


#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/TimeTypes.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
extern RODOS::Topic<bool> eduIsAliveTopic;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForListener;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForPowerManagement;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForCommunicationError;
extern RODOS::CommBuffer<bool> eduIsAliveBufferForTelemetry;

extern RODOS::Topic<Duration> nextProgramStartDelayTopic;
extern RODOS::CommBuffer<Duration> nextProgramStartDelayBuffer;

extern RODOS::Topic<ProgramId> programIdOfCurrentEduProgramQueueEntryTopic;
extern RODOS::CommBuffer<ProgramId> programIdOfCurrentEduProgramQueueEntryBuffer;

extern RODOS::Topic<std::int32_t> rxBaudRateTopic;
extern RODOS::CommBuffer<std::int32_t> rxBaudRateBuffer;

extern RODOS::Topic<std::int32_t> txBaudRateTopic;
extern RODOS::CommBuffer<std::int32_t> txBaudRateBuffer;

extern RODOS::Topic<TelemetryRecord> telemetryTopic;
// TODO: Look for a less memory-intensive solution than a CommBuffer
extern RODOS::CommBuffer<TelemetryRecord> telemetryBuffer;
}

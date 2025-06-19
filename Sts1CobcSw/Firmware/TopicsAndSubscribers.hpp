#pragma once


#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>


// TODO: If more mailboxes are added, think about renaming this module to InterThreadCommunication,
// InterprocessCommunication, or something similar.
namespace sts1cobcsw
{
// Topics and subscribers must be defined in the same file to prevent a static initialization order
// fiasco
inline auto eduIsAliveTopic = RODOS::Topic<bool>(-1, "eduIsAliveTopic");
inline auto eduIsAliveBufferForListener = RODOS::CommBuffer<bool>{};
inline auto eduIsAliveBufferForPowerManagement = RODOS::CommBuffer<bool>{};
inline auto eduIsAliveBufferForCommunicationError = RODOS::CommBuffer<bool>{};
inline auto eduIsAliveBufferForTelemetry = RODOS::CommBuffer<bool>{};

inline auto nextProgramStartDelayTopic = RODOS::Topic<Duration>(-1, "nextProgramStartDelayTopic");
inline auto nextProgramStartDelayBuffer = RODOS::CommBuffer<Duration>{};

inline auto programIdOfCurrentEduProgramQueueEntryTopic =
    RODOS::Topic<ProgramId>(-1, "programIdOfCurrentEduProgramQueueEntryTopic");
inline auto programIdOfCurrentEduProgramQueueEntryBuffer = RODOS::CommBuffer<ProgramId>{};

inline auto rxDataRateTopic = RODOS::Topic<std::int32_t>(-1, "rxDataRateTopic");
inline auto rxDataRateBuffer = RODOS::CommBuffer<std::int32_t>{};

inline auto txDataRateTopic = RODOS::Topic<std::int32_t>(-1, "txDataRateTopic");
inline auto txDataRateBuffer = RODOS::CommBuffer<std::int32_t>{};

// We only send the telemetry records from the telemetry thread to the RF communication thread, so
// we don't need the whole publisher/subscriber mechanism here. A simple mailbox is enough.
inline auto telemetryRecordMailbox = Mailbox<TelemetryRecord>{};
inline auto encodedCfdpFrameMailbox = Mailbox<std::array<Byte, blockLength>>{};
}

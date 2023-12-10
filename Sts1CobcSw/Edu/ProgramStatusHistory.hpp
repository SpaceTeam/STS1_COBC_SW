#pragma once


// clang-format off
#include <cstdint>
// ringbuffer.h does not include <cstdint> even though it requires it
#include <rodos/support/support-libs/ringbuffer.h>
// clang-format on


namespace sts1cobcsw::edu
{
enum class ProgramStatus : std::uint8_t
{
    programRunning,
    programCouldNotBeStarted,
    programExecutionFailed,
    programExecutionSucceeded,
    resultFileTransfered,
    resultFileSentToRf,
    ackFromGround,
    resultFileDeleted
};


struct ProgramStatusHistoryEntry
{
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
    ProgramStatus status = ProgramStatus::programRunning;
};


inline constexpr auto programStatusHistorySize = 20;


extern RODOS::RingBuffer<ProgramStatusHistoryEntry, programStatusHistorySize> programStatusHistory;


auto UpdateProgramStatusHistory(std::uint16_t programId,
                                std::int32_t startTime,
                                ProgramStatus newStatus) -> void;
}

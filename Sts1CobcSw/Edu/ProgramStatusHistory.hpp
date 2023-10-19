#pragma once


#include <type_safe/types.hpp>

#include <rodos/support/support-libs/ringbuffer.h>

#include <cstdint>


namespace sts1cobcsw::edu
{
namespace ts = type_safe;
using ts::operator""_u16;


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
    ts::uint16_t programId = 0_u16;
    ts::uint16_t queueId = 0_u16;
    ProgramStatus status = ProgramStatus::programRunning;
};


inline constexpr auto programStatusHistorySize = 20;


extern RODOS::RingBuffer<ProgramStatusHistoryEntry, programStatusHistorySize> programStatusHistory;


auto FindProgramStatusHistoryEntry(std::uint16_t programId, std::uint16_t queueId)
    -> ProgramStatusHistoryEntry;
}

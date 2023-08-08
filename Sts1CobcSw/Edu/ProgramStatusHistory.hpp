#pragma once

#include <type_safe/types.hpp>

#include <ringbuffer.h>

#include <cstdint>


namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_u16;


enum class ProgramStatus : uint8_t
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


struct EduProgramStatusHistoryEntry
{
    ts::uint16_t programId = 0_u16;
    ts::uint16_t queueId = 0_u16;
    ProgramStatus status = ProgramStatus::programRunning;
};


inline constexpr auto programStatusHistorySize = 20;
extern RODOS::RingBuffer<EduProgramStatusHistoryEntry, programStatusHistorySize>
    eduProgramStatusHistory;


auto FindEduProgramStatusHistoryEntry(std::uint16_t programId, std::uint16_t queueId)
    -> EduProgramStatusHistoryEntry;
}

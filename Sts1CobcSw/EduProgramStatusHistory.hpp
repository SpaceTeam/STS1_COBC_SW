#pragma once

#include <type_safe/types.hpp>

#include <ringbuffer.h>

#include <cstdint>


namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_u16;
using ts::operator""_u32;


enum class EduProgramStatus : uint8_t
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
    ts::uint32_t queueId = 0_u32;
    EduProgramStatus status = EduProgramStatus::programRunning;
};


inline constexpr auto eduProgramStatusHistorySize = 20;
extern RODOS::RingBuffer<EduProgramStatusHistoryEntry, eduProgramStatusHistorySize>
    eduProgramStatusHistory;


auto FindEduProgramStatusHistoryEntry(std::uint16_t programId, std::uint32_t queueId)
    -> EduProgramStatusHistoryEntry;
}

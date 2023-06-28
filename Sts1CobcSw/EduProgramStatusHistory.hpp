#pragma once

#include <type_safe/types.hpp>

#include <ringbuffer.h>

#include <cstdint>

namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_u16;

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
    ts::uint16_t queueId = 0_u16;
    EduProgramStatus status = EduProgramStatus::programRunning;
};

// TODO: Think about the name. Maybe something like program/queueStatusAndHistory is better?
inline constexpr auto eduProgramStatusHistorySize = 20;

RODOS::RingBuffer<EduProgramStatusHistoryEntry, eduProgramStatusHistorySize>
    eduProgramStatusHistory;

auto FindEduProgramStatusHistoryEntry(std::uint16_t programId, std::uint16_t queueId)
    -> EduProgramStatusHistoryEntry;
}
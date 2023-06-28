#pragma once

#include <cstdint>

#include <ringbuffer.h>

#include <type_safe/types.hpp>

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


struct StatusHistoryEntry
{
    ts::uint16_t programId = 0_u16;
    ts::uint16_t queueId = 0_u16;
    ProgramStatus status = ProgramStatus::programRunning;
};

// TODO: Think about the name. Maybe something like program/queueStatusAndHistory is better?
inline constexpr auto statusHistorySize = 20;

RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory;

auto FindStatusAndHistoryEntry(std::uint16_t programId, std::uint16_t queueId)
    -> StatusHistoryEntry;
}
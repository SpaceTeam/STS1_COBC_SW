#pragma once

// clang-format off
#include <cstdint>
// TODO: Change install rules of rodos such that this must be included as
// rodos/support-libs/ringbuffer.h or something.
//
// ringbuffer.h does not include <cstdint> even though it requires it
#include <ringbuffer.h>
// clang-format on

#include <type_safe/types.hpp>

#include <etl/string.h>
#include <etl/vector.h>


namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_u8;
using ts::operator""_i16;
using ts::operator""_u16;
using ts::operator""_i32;


struct EduQueueEntry
{
    ts::uint16_t programId = 0_u16;
    ts::uint16_t queueId = 0_u16;
    ts::int32_t startTime = 0_i32;
    ts::int16_t timeout = 0_i16;
};

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

inline constexpr auto eduProgramQueueSize = 20;
// TODO: Think about the name. Maybe something like program/queueStatusAndHistory is better?
inline constexpr auto statusHistorySize = 20;

// TODO: Why is this defined in EduProgramQueueThread.cpp and not EduProgramQueue.cpp?
extern uint16_t queueIndex;
extern etl::vector<EduQueueEntry, eduProgramQueueSize> eduProgramQueue;
// TODO: Maybe move that to its own file? Together with the definition of StatusHistoryEntry of
// course.
extern RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory;
}

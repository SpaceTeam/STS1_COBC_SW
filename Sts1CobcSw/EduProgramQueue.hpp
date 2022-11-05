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
    ts::uint16_t programId;
    ts::uint16_t queueId;
    ts::int32_t startTime;
    ts::int16_t timeout;

    explicit EduQueueEntry(ts::uint16_t programId_ = 0_u16,  // NOLINT
                           ts::uint16_t queueId_ = 0_u16,    // NOLINT
                           ts::int32_t startTime_ = 0_i32,   // NOLINT
                           ts::int16_t timeout_ = 0_i16)     // NOLINT
        : programId(programId_),
          queueId(queueId_),
          startTime(startTime_),
          timeout(timeout_)  // NOLINT
    {
    }
};


// TODO: Move the status and history related stuff to its own StatusAndHistory.hpp
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
    ts::uint16_t programId;
    ts::uint16_t queueId;
    ProgramStatus status{};

    // TODO: would'nt it be better to create an 'unitialized' status ?
    StatusHistoryEntry() : programId(0_u16), queueId(0_u16), status{ProgramStatus::programRunning}
    {
    }


    StatusHistoryEntry(ts::uint16_t programIdArg,
                       ts::uint16_t queueIdArg,
                       ProgramStatus statusArg)                            // NOLINT
        : programId(programIdArg), queueId(queueIdArg), status(statusArg)  // NOLINT
    {
    }
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

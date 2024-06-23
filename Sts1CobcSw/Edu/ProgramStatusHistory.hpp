#pragma once


#include <Sts1CobcSw/Periphery/FramLayout.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

// clang-format off
#include <cstdint>
// ringbuffer.h does not include <cstdint> even though it requires it
#include <rodos/support/support-libs/ringbuffer.h>
// clang-format on

#include <cstddef>


namespace sts1cobcsw
{
namespace edu
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
    ProgramId programId = ProgramId(0);
    std::int32_t startTime = 0;
    ProgramStatus status = ProgramStatus::programRunning;
};
}


template<>
inline constexpr std::size_t serialSize<edu::ProgramStatusHistoryEntry> =
    totalSerialSize<decltype(edu::ProgramStatusHistoryEntry::programId),
                    decltype(edu::ProgramStatusHistoryEntry::startTime),
                    decltype(edu::ProgramStatusHistoryEntry::status)>;


namespace edu
{
inline constexpr auto programStatusHistorySize = 20;
static_assert(programStatusHistorySize * totalSerialSize<ProgramStatusHistoryEntry>
                  <= fram::EduProgramStatusHistory::size,
              "Size of EDU program status history exceeds size of FRAM section");

extern RODOS::RingBuffer<ProgramStatusHistoryEntry, programStatusHistorySize> programStatusHistory;


auto UpdateProgramStatusHistory(ProgramId programId,
                                std::int32_t startTime,
                                ProgramStatus newStatus) -> void;
}
}

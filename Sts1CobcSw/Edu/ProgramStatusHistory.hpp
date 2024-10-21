#pragma once


#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/RingArray.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>


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
    RealTime startTime = RealTime(0);
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
inline constexpr auto nCachedProgramStatusHistoryEntries = 10;

extern RingArray<ProgramStatusHistoryEntry,
                 framSections.template Get<"eduProgramStatusHistory">(),
                 nCachedProgramStatusHistoryEntries>
    programStatusHistory;

auto UpdateProgramStatusHistory(ProgramId programId, RealTime startTime, ProgramStatus newStatus)
    -> void;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramStatusHistoryEntry * data)
    -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ProgramStatusHistoryEntry const & data)
    -> void *;
}
}

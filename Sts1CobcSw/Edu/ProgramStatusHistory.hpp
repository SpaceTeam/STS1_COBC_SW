#pragma once


#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/TimeTypes.hpp>


namespace sts1cobcsw::edu
{
inline constexpr auto nCachedProgramStatusHistoryEntries = 10;
inline constexpr auto programStatusHistory =
    FramRingArray<ProgramStatusHistoryEntry,
                  framSections.template Get<"eduProgramStatusHistory">(),
                  nCachedProgramStatusHistoryEntries>{};


auto UpdateProgramStatusHistory(ProgramId programId, RealTime startTime, ProgramStatus newStatus)
    -> void;
}

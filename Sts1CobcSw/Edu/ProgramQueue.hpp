#pragma once


#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>


namespace sts1cobcsw::edu
{
inline constexpr auto nCachedProgramQueueEntries = 10;
inline constexpr auto programQueue = FramVector<ProgramQueueEntry,
                                                framSections.template Get<"eduProgramQueue">(),
                                                nCachedProgramQueueEntries>{};
}

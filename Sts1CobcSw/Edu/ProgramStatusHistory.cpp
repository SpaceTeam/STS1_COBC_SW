#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>

#include <strong_type/equality.hpp>

#include <etl/circular_buffer.h>


namespace sts1cobcsw::edu
{
auto UpdateProgramStatusHistory(ProgramId programId, RealTime startTime, ProgramStatus newStatus)
    -> void
{
    programStatusHistory.FindAndReplace(
        [programId, startTime](auto const & entry) -> bool
        { return entry.programId == programId and entry.startTime == startTime; },
        ProgramStatusHistoryEntry{programId, startTime, newStatus});
}
}

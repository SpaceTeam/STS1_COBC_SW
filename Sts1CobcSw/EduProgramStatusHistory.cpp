#include <Sts1CobcSw/EduProgramStatusHistory.hpp>

#include <rodos.h>


namespace sts1cobcsw
{
RODOS::RingBuffer<EduProgramStatusHistoryEntry, eduProgramStatusHistorySize>
    eduProgramStatusHistory;


auto FindEduProgramStatusHistoryEntry(std::uint16_t programId, std::uint32_t queueId)
    -> EduProgramStatusHistoryEntry
{
    auto eduProgramStatusHistoryEntry = EduProgramStatusHistoryEntry{};
    do
    {
        eduProgramStatusHistory.get(eduProgramStatusHistoryEntry);
        // RODOS::PRINTF("%d,%d vs %d,%d\n", eduProgramStatusHistoryEntry.programId,
        // eduProgramStatusHistoryEntry.queueId, programId, queueId);
    } while(eduProgramStatusHistoryEntry.queueId != queueId
            or eduProgramStatusHistoryEntry.programId != programId);

    return eduProgramStatusHistoryEntry;
}
}

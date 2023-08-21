#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>

// #include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
RODOS::RingBuffer<EduProgramStatusHistoryEntry, programStatusHistorySize> eduProgramStatusHistory;


auto FindEduProgramStatusHistoryEntry(std::uint16_t programId, std::uint16_t queueId)
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
    int a = 3;
}
}

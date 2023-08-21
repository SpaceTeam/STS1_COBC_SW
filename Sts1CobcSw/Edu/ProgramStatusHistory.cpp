#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>

// #include <rodos_no_using_namespace.h>


namespace sts1cobcsw::edu
{
RODOS::RingBuffer<ProgramStatusHistoryEntry, programStatusHistorySize> programStatusHistory;


// TODO: This should probably not return a copy but a reference or pointer so that the user code can
// modify the entry.
auto FindProgramStatusHistoryEntry(std::uint16_t programId, std::uint16_t queueId)
    -> ProgramStatusHistoryEntry
{
    auto programStatusHistoryEntry = ProgramStatusHistoryEntry{};
    do
    {
        programStatusHistory.get(programStatusHistoryEntry);
        // RODOS::PRINTF("%d,%d vs %d,%d\n", eduProgramStatusHistoryEntry.programId,
        // eduProgramStatusHistoryEntry.queueId, programId, queueId);
    } while(programStatusHistoryEntry.queueId != queueId
            or programStatusHistoryEntry.programId != programId);

    return programStatusHistoryEntry;
}
}

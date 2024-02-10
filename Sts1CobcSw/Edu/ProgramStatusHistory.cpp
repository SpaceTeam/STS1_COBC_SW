#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>


namespace sts1cobcsw::edu
{
RODOS::RingBuffer<ProgramStatusHistoryEntry, programStatusHistorySize> programStatusHistory;


auto UpdateProgramStatusHistory(std::uint16_t programId,
                                std::int32_t startTime,
                                ProgramStatus newStatus) -> void
{
    // TODO: Check that there is only one entry matching program/queue ID, or should it be the case
    // by construction ?
    for(std::uint32_t i = 0; i < programStatusHistory.occupiedCnt; ++i)
    {
        if(programStatusHistory.vals[i].startTime == startTime
           and programStatusHistory.vals[i].programId == programId)
        {
            programStatusHistory.vals[i].status = newStatus;
        }
    }
}
}

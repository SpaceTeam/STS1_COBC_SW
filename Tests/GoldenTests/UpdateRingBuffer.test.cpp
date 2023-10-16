#include <Sts1CobcSw/Dummy.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>

#include <ringbuffer.h>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cstdint>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{

auto StatusToString(edu::ProgramStatus status) -> std::string
{
    switch(status)
    {
        case edu::ProgramStatus::programRunning:
            return {"programRunning"};
        case edu::ProgramStatus::programExecutionFailed:
            return {"programExecutionFailed"};
        case edu::ProgramStatus::programExecutionSucceeded:
            return {"programExecutionSucceeded"};
        default:
            break;
    }
}


// Helper function for edu::programStatusHistory
void PrintBuffer()
{
    for(int i = 0; i < edu::programStatusHistory.occupiedCnt; ++i)
    {
        RODOS::PRINTF("Vals[%d] = .id(%d), .status(%s)\n",
                      i,
                      edu::programStatusHistory.vals[i].programId.get(),
                      StatusToString(edu::programStatusHistory.vals[i].status).c_str());
    }
}


class HelloDummy : public RODOS::StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        edu::programStatusHistory.put(
            edu::ProgramStatusHistoryEntry{.programId = static_cast<std::uint16_t>(1),
                                           .queueId = static_cast<std::uint16_t>(1),
                                           .status = edu::ProgramStatus::programExecutionFailed});

        edu::programStatusHistory.put(
            edu::ProgramStatusHistoryEntry{.programId = static_cast<std::uint16_t>(2),
                                           .queueId = static_cast<std::uint16_t>(1),
                                           .status = edu::ProgramStatus::programRunning});

        edu::programStatusHistory.put(
            edu::ProgramStatusHistoryEntry{.programId = static_cast<std::uint16_t>(3),
                                           .queueId = static_cast<std::uint16_t>(1),
                                           .status = edu::ProgramStatus::programRunning});

        edu::programStatusHistory.put(
            edu::ProgramStatusHistoryEntry{.programId = static_cast<std::uint16_t>(4),
                                           .queueId = static_cast<std::uint16_t>(1),
                                           .status = edu::ProgramStatus::programRunning});


        auto readCnt = edu::programStatusHistory.readCnt;
        auto writeCnt = edu::programStatusHistory.writeCnt;
        auto occupiedCnt = edu::programStatusHistory.occupiedCnt;


        // Print RingBuffer
        PrintBuffer();


        edu::UpdateProgramStatusHistory(2, 1, edu::ProgramStatus::programExecutionSucceeded);
        edu::UpdateProgramStatusHistory(4, 1, edu::ProgramStatus::programExecutionFailed);

        edu::programStatusHistory.put(
            edu::ProgramStatusHistoryEntry{.programId = static_cast<std::uint16_t>(5),
                                           .queueId = static_cast<std::uint16_t>(1),
                                           .status = edu::ProgramStatus::programRunning});

        edu::UpdateProgramStatusHistory(5, 1, edu::ProgramStatus::programExecutionSucceeded);

        // 1, we did not read anything
        RODOS::PRINTF("readCnt unchanged     : %d\n",
                      static_cast<int>(edu::programStatusHistory.readCnt == readCnt));
        // 0, bc we did write
        RODOS::PRINTF("writeCnt unchanged    : %d\n",
                      static_cast<int>(edu::programStatusHistory.readCnt == writeCnt));
        // 0
        RODOS::PRINTF("OccupiedCnt unchanged : %d\n",
                      static_cast<int>(edu::programStatusHistory.occupiedCnt == occupiedCnt));

        // Print RingBuffer
        PrintBuffer();

        RODOS::hwResetAndReboot();
    }
} helloDummy;
}

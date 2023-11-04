#include <Sts1CobcSw/Dummy.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>

#include <type_safe/types.hpp>

#include <rodos/support/support-libs/ringbuffer.h>
#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <string_view>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{
auto ToString(edu::ProgramStatus status) -> std::string_view
{
    switch(status)
    {
        case edu::ProgramStatus::programRunning:
            return "programRunning";
        case edu::ProgramStatus::programExecutionFailed:
            return "programExecutionFailed";
        case edu::ProgramStatus::programExecutionSucceeded:
            return "programExecutionSucceeded";
        default:
            return "";
    }
}


// Helper function for edu::programStatusHistory
void PrintBuffer()
{
    for(std::uint32_t i = 0; i < edu::programStatusHistory.occupiedCnt; ++i)
    {
        RODOS::PRINTF("Vals[%d] = .id(%d), .status(%s)\n",
                      i,
                      edu::programStatusHistory.vals[i].programId.get(),
                      ToString(edu::programStatusHistory.vals[i].status).data());
    }
}


class UpdateRingBufferTest : public RODOS::StaticThread<>
{
    void run() override
    {
        using type_safe::operator""_u16;

        printfMask = 1;

        edu::programStatusHistory.put(
            edu::ProgramStatusHistoryEntry{.programId = 1_u16,
                                           .queueId = 1_u16,
                                           .status = edu::ProgramStatus::programExecutionFailed});
        edu::programStatusHistory.put(edu::ProgramStatusHistoryEntry{
            .programId = 2_u16, .queueId = 1_u16, .status = edu::ProgramStatus::programRunning});
        edu::programStatusHistory.put(edu::ProgramStatusHistoryEntry{
            .programId = 3_u16, .queueId = 1_u16, .status = edu::ProgramStatus::programRunning});
        edu::programStatusHistory.put(edu::ProgramStatusHistoryEntry{
            .programId = 4_u16, .queueId = 1_u16, .status = edu::ProgramStatus::programRunning});


        auto readCnt = edu::programStatusHistory.readCnt;
        auto writeCnt = edu::programStatusHistory.writeCnt;
        auto occupiedCnt = edu::programStatusHistory.occupiedCnt;

        PrintBuffer();

        edu::UpdateProgramStatusHistory(2, 1, edu::ProgramStatus::programExecutionSucceeded);
        edu::UpdateProgramStatusHistory(4, 1, edu::ProgramStatus::programExecutionFailed);
        edu::programStatusHistory.put(edu::ProgramStatusHistoryEntry{
            .programId = 5_u16, .queueId = 1_u16, .status = edu::ProgramStatus::programRunning});
        edu::UpdateProgramStatusHistory(5, 1, edu::ProgramStatus::programExecutionSucceeded);

        // 1, because we did not read anything
        RODOS::PRINTF("readCnt unchanged     : %d\n",
                      static_cast<int>(edu::programStatusHistory.readCnt == readCnt));
        // 0, because we did write
        RODOS::PRINTF("writeCnt unchanged    : %d\n",
                      static_cast<int>(edu::programStatusHistory.readCnt == writeCnt));
        // 0
        RODOS::PRINTF("OccupiedCnt unchanged : %d\n",
                      static_cast<int>(edu::programStatusHistory.occupiedCnt == occupiedCnt));

        PrintBuffer();

        RODOS::hwResetAndReboot();
    }
} updateRingBufferTest;
}

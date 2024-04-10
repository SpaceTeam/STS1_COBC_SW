#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <cinttypes>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using utility::PrintFormattedSystemUtc;


// TODO: Move this to EduProgramQueueThreadMock.cpp or something
auto ResumeEduProgramQueueThread() -> void
{
    PRINTF("Call to ResumeEduProgramQueueThread()\n");
}


// TODO: This file is not used at all right now. Think about the mocking later.
namespace edu
{
auto Initialize() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to Initialize()\n");
}


auto TurnOn() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to TurnOn()\n");
}


auto TurnOff() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to TurnOff()\n");
}


auto StoreProgram(StoreProgramData const & data) -> Result<void>
{
    PrintFormattedSystemUtc();
    PRINTF("Call to StoreProgram(programId = %d)\n", data.programId);
    return outcome_v2::success();
}


auto ExecuteProgram(ExecuteProgramData const & data) -> Result<void>
{
    PrintFormattedSystemUtc();
    PRINTF("Call to ExecuteProgram(programId = %d, startTime = %" PRIi32 ", timeout = %d)\n",
           data.programId,
           data.startTime,
           data.timeout);
    return outcome_v2::success();
}


auto StopProgram() -> Result<void>
{
    PrintFormattedSystemUtc();
    PRINTF("Call to StopProgram()\n");
    return outcome_v2::success();
}


auto GetStatus() -> Result<Status>
{
    PrintFormattedSystemUtc();
    PRINTF("Call to GetStatus()\n");
    return Status{.statusType = StatusType::invalid, .programId = 0, .startTime = 0, .exitCode = 0};
}


auto UpdateTime(UpdateTimeData const & data) -> Result<void>
{
    PrintFormattedSystemUtc();
    PRINTF("Call to UpdateTime(currentTime = %" PRIi32 ")\n", data.currentTime);
    return outcome_v2::success();
}
}
}

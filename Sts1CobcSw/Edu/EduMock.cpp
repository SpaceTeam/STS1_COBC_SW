#include <Sts1CobcSw/Edu/Edu.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <cinttypes>  // IWYU pragma: keep


namespace sts1cobcsw
{
using utility::PrintFormattedSystemUtc;


// TODO: Move this to EduProgramQueueThreadMock.cpp or something
auto ResumeEduProgramQueueThread() -> void
{
    DEBUG_PRINT("Call to ResumeEduProgramQueueThread()\n");
}


// TODO: This file is not used at all right now. Think about the mocking later.
namespace edu
{
auto Initialize() -> void
{
    PrintFormattedSystemUtc();
    DEBUG_PRINT("Call to Initialize()\n");
}


auto TurnOn() -> void
{
    PrintFormattedSystemUtc();
    DEBUG_PRINT("Call to TurnOn()\n");
}


auto TurnOff() -> void
{
    PrintFormattedSystemUtc();
    DEBUG_PRINT("Call to TurnOff()\n");
}


auto StoreProgram(StoreProgramData const & data) -> Result<void>
{
    PrintFormattedSystemUtc();
    DEBUG_PRINT("Call to StoreProgram(programId = %" PRIu16 ")\n", value_of(data.programId));
    return outcome_v2::success();
}


auto ExecuteProgram(ExecuteProgramData const & data) -> Result<void>
{
    PrintFormattedSystemUtc();
    DEBUG_PRINT("Call to ExecuteProgram(programId = %" PRIu16 ", startTime = %" PRIi32
                ", timeout = %d)\n",
                value_of(data.programId),
                data.startTime,
                data.timeout);
    return outcome_v2::success();
}


auto StopProgram() -> Result<void>
{
    PrintFormattedSystemUtc();
    DEBUG_PRINT("Call to StopProgram()\n");
    return outcome_v2::success();
}


auto GetStatus() -> Result<Status>
{
    PrintFormattedSystemUtc();
    DEBUG_PRINT("Call to GetStatus()\n");
    return Status();
}


auto UpdateTime(UpdateTimeData const & data) -> Result<void>
{
    PrintFormattedSystemUtc();
    DEBUG_PRINT("Call to UpdateTime(currentTime = %" PRIi32 ")\n", data.currentTime);
    return outcome_v2::success();
}
}
}

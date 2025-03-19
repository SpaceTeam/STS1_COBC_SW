#include <Sts1CobcSw/Edu/Edu.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>

#include <strong_type/type.hpp>

#include <cinttypes>  // IWYU pragma: keep


namespace sts1cobcsw
{
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
    DEBUG_PRINT_REAL_TIME();
    DEBUG_PRINT("Call to Initialize()\n");
}


auto TurnOn() -> void
{
    DEBUG_PRINT_REAL_TIME();
    DEBUG_PRINT("Call to TurnOn()\n");
}


auto TurnOff() -> void
{
    DEBUG_PRINT_REAL_TIME();
    DEBUG_PRINT("Call to TurnOff()\n");
}


auto StoreProgram([[maybe_unused]] StoreProgramData const & data) -> Result<void>
{
    DEBUG_PRINT_REAL_TIME();
    DEBUG_PRINT("Call to StoreProgram(programId = %" PRIu16 ")\n", value_of(data.programId));
    return outcome_v2::success();
}


auto ExecuteProgram([[maybe_unused]] ExecuteProgramData const & data) -> Result<void>
{
    DEBUG_PRINT_REAL_TIME();
    DEBUG_PRINT("Call to ExecuteProgram(programId = %" PRIu16 ", startTime = %" PRIi32
                ", timeout = %d)\n",
                value_of(data.programId),
                value_of(data.startTime),
                data.timeout);
    return outcome_v2::success();
}


auto StopProgram() -> Result<void>
{
    DEBUG_PRINT_REAL_TIME();
    DEBUG_PRINT("Call to StopProgram()\n");
    return outcome_v2::success();
}


auto GetStatus() -> Result<Status>
{
    DEBUG_PRINT_REAL_TIME();
    DEBUG_PRINT("Call to GetStatus()\n");
    return Status();
}


auto UpdateTime([[maybe_unused]] UpdateTimeData const & data) -> Result<void>
{
    DEBUG_PRINT_REAL_TIME();
    DEBUG_PRINT("Call to UpdateTime(currentTime = %" PRIi32 ")\n", value_of(data.currentTime));
    return outcome_v2::success();
}
}
}

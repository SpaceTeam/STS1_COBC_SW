#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <cinttypes>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using utility::PrintFormattedSystemUtc;


// TODO: Move this to the proper file
auto ResumeEduErrorCommunicationThread() -> void
{
    PRINTF("\nCall to ResumeEduErrorCommunicationThread()\n");
    PrintFormattedSystemUtc();
}


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


auto StoreArchive(StoreArchiveData const & data) -> Result<std::int32_t>
{
    PrintFormattedSystemUtc();
    PRINTF("Call to StoreArchive(programId = %d)\n", data.programId);
    return 0;
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


auto SendCommand(Byte commandId) -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to SendCommand(commandId = 0x%02x)\n", static_cast<unsigned int>(commandId));
}


auto SendData(std::span<Byte> data) -> Result<void>
{
    PrintFormattedSystemUtc();
    PRINTF("Call to SendData(size(data) = %d)\n", size(data));
    return outcome_v2::success();
}


auto UartReceive([[maybe_unused]] std::span<Byte> destination) -> Result<void>
{
    PrintFormattedSystemUtc();
    PRINTF("Call to UartReceive(size(destination) = %d)\n", size(destination));
    return outcome_v2::success();
}


void FlushUartBuffer()
{
    PrintFormattedSystemUtc();
    PRINTF("Call to FlushUartBuffer()\n");
}
}
}

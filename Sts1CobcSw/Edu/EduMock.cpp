#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>


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
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Initialize() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to Initialize()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto TurnOn() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to TurnOn()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto TurnOff() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to TurnOff()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto StoreArchive(StoreArchiveData const & data) -> std::int32_t
{
    PrintFormattedSystemUtc();
    PRINTF("Call to StoreArchive(programId = %d)\n", data.programId);
    return 0;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto ExecuteProgram(ExecuteProgramData const & data) -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to ExecuteProgram(programId = %d, queueId = %d, timeout = %d)\n",
           data.programId,
           data.queueId,
           data.timeout);
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto StopProgram() -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to StopProgram()\n");
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto GetStatus() -> Status
{
    PrintFormattedSystemUtc();
    PRINTF("Call to GetStatus()\n");
    return {.statusType = StatusType::invalid,
            .programId = 0,
            .queueId = 0,
            .exitCode = 0,
            .errorCode = ErrorCode::success};
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto UpdateTime(UpdateTimeData const & data) -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to UpdateTime(timestamp = %d)\n", data.timestamp);
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto SendCommand(Byte commandId) -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to SendCommand(commandId = 0x%02x)\n", static_cast<unsigned int>(commandId));
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto SendData(std::span<Byte> data) -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to SendData(size(data) = %d)\n", size(data));
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto UartReceive([[maybe_unused]] std::span<Byte> destination) -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to UartReceive(size(destination) = %d)\n", size(destination));
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void FlushUartBuffer()
{
    PrintFormattedSystemUtc();
    PRINTF("Call to FlushUartBuffer()\n");
}
}
}

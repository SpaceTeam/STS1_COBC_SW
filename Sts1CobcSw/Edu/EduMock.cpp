#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using utility::PrintFormattedSystemUtc;


edu::Edu eduUnit;


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
auto Edu::Initialize() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to Initialize()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::TurnOn() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to TurnOn()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::TurnOff() -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to TurnOff()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::StoreArchive(StoreArchiveData const & data) -> std::int32_t
{
    PrintFormattedSystemUtc();
    PRINTF("Call to StoreArchive(programId = %d)\n", data.programId.get());
    return 0;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::ExecuteProgram(ExecuteProgramData const & data) -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to ExecuteProgram(programId = %d, queueId = %d, timeout = %d)\n",
           data.programId.get(),
           data.queueId.get(),
           data.timeout.get());
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::StopProgram() -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to StopProgram()\n");
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::GetStatus() -> Status
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
auto Edu::UpdateTime(UpdateTimeData const & data) -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to UpdateTime(timestamp = %d)\n", data.timestamp.get());
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::SendCommand(Byte commandId) -> void
{
    PrintFormattedSystemUtc();
    PRINTF("Call to SendCommand(commandId = 0x%02x)\n", static_cast<unsigned int>(commandId));
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::SendData(std::span<Byte> data) -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to SendData(size(data) = %d)\n", size(data));
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::UartReceive([[maybe_unused]] std::span<Byte> destination) -> ErrorCode
{
    PrintFormattedSystemUtc();
    PRINTF("Call to UartReceive(size(destination) = %d)\n", size(destination));
    return ErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Edu::FlushUartBuffer()
{
    PrintFormattedSystemUtc();
    PRINTF("Call to FlushUartBuffer()\n");
}
}
}
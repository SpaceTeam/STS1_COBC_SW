#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>
#include <Sts1CobcSw/Utility/UtilityNames.hpp>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using utility::PrintTime;


// TODO: Move this in the proper file
auto ResumeEduErrorCommunicationThread() -> void
{
    PRINTF("\nCall to ResumeEduErrorCommunicationThread()\n");
    PrintTime();
}


// TODO: This file is not used at all right now. Think about the mocking later.
namespace periphery
{
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::Initialize() -> void
{
    PrintTime();
    PRINTF("Call to Initialize()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::TurnOn() -> void
{
    PrintTime();
    PRINTF("Call to TurnOn()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::TurnOff() -> void
{
    PrintTime();
    PRINTF("Call to TurnOff()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::StoreArchive(StoreArchiveData const & data) -> std::int32_t
{
    PrintTime();
    PRINTF("Call to StoreArchive(programId = %d)\n", data.programId.get());
    return 0;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::ExecuteProgram(ExecuteProgramData const & data) -> EduErrorCode
{
    PrintTime();
    PRINTF("Call to ExecuteProgram(programId = %d, queueId = %d, timeout = %d)\n",
           data.programId.get(),
           data.queueId.get(),
           data.timeout.get());
    return EduErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::StopProgram() -> EduErrorCode
{
    PrintTime();
    PRINTF("Call to StopProgram()\n");
    return EduErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::GetStatus() -> EduStatus
{
    PrintTime();
    PRINTF("Call to GetStatus()\n");
    return {.statusType = EduStatusType::invalid,
            .programId = 0,
            .queueId = 0,
            .exitCode = 0,
            .errorCode = EduErrorCode::success};
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::UpdateTime(UpdateTimeData const & data) -> EduErrorCode
{
    PrintTime();
    PRINTF("Call to UpdateTime(timestamp = %d)\n", data.timestamp.get());
    return EduErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::SendCommand(Byte commandId) -> void
{
    PrintTime();
    PRINTF("Call to SendCommand(commandId = 0x%02x)\n", static_cast<unsigned int>(commandId));
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::SendData(std::span<Byte> data) -> EduErrorCode
{
    PrintTime();
    PRINTF("Call to SendData(size(data) = %d)\n", size(data));
    return EduErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::UartReceive([[maybe_unused]] std::span<Byte> destination) -> EduErrorCode
{
    PrintTime();
    PRINTF("Call to UartReceive(size(destination) = %d)\n", size(destination));
    return EduErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Edu::FlushUartBuffer()
{
    PrintTime();
    PRINTF("Call to FlushUartBuffer()\n");
}
}
}

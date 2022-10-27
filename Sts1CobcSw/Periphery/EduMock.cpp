#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/UtilityNames.hpp>


namespace sts1cobcsw::periphery
{
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::ExecuteProgram([[maybe_unused]] ExecuteProgramData const & data)
    -> EduErrorCode
{
    RODOS::PRINTF("Call to ExecuteProgram()\n");
    return EduErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::GetStatus() -> EduStatus
{
    RODOS::PRINTF("Call to GetStatus()\n");

    return {.statusType = EduStatusType::invalid,
            .programId = 0,
            .queueId = 0,
            .exitCode = 0,
            .errorCode = EduErrorCode::success};
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::UpdateTime([[maybe_unused]] UpdateTimeData const & data) -> EduErrorCode
{
    RODOS::PRINTF("Call to UpdateTime()\n");
    return EduErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Edu::SendCommand([[maybe_unused]] Byte commandId) -> void
{
    RODOS::PRINTF("Call to SendCommand()\n");
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::SendData([[maybe_unused]] std::span<Byte> data) -> EduErrorCode
{
    RODOS::PRINTF("Call to SendData()\n");
    return EduErrorCode::success;
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] auto Edu::UartReceive([[maybe_unused]] std::span<Byte> destination) -> EduErrorCode
{
    RODOS::PRINTF("Call to UartReceive()\n");
    return EduErrorCode::success;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Edu::FlushUartBuffer()
{
    RODOS::PRINTF("Call to FlushUartBuffer()\n");
}
}

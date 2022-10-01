#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Util/Util.hpp>
#include <Sts1CobcSw/Util/UtilNames.hpp>

#include <array>
#include <iostream>
#include <vector>

namespace sts1cobcsw::periphery
{
EduUartInterface::EduUartInterface() = default;

// NOLINTNEXTLINE
void EduUartInterface::FlushUartBuffer()
{
    RODOS::PRINTF("Call to FlushUartBuffer()\n");
}

// NOLINTNEXTLINE
auto EduUartInterface::UartReceive(std::vector<uint8_t> & recvVec, size_t nBytes) -> EduErrorCode
{
    RODOS::PRINTF("Call to UartReceive()\n");
    return EduErrorCode::success;
}

// NOLINTNEXTLINE
void EduUartInterface::SendCommand(uint8_t cmd)
{
    RODOS::PRINTF("Call to SendCommand()\n");
}

// NOLINTNEXTLINE
auto EduUartInterface::SendData(std::span<uint8_t> data) -> EduErrorCode
{
    RODOS::PRINTF("Call to SendData()\n");
    return EduErrorCode::success;
}

// NOLINTNEXTLINE
auto EduUartInterface::ExecuteProgram(uint16_t programId, uint16_t queueId, uint16_t timeout)
    -> EduErrorCode
{
    RODOS::PRINTF("Call to ExecuteProgram()\n");
    return EduErrorCode::success;
}

// NOLINTNEXTLINE
auto EduUartInterface::GetStatus()
    -> std::tuple<EduStatusType, uint16_t, uint16_t, uint8_t, EduErrorCode>
{
    RODOS::PRINTF("Call to GetStatus()\n");

    uint16_t programId = 0;
    uint16_t queueId = 0;
    uint8_t exitCode = 0;
    return {EduStatusType::invalid, programId, queueId, exitCode, EduErrorCode::success};
}
}

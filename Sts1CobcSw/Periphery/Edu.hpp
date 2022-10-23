#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>

#include <span>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::Byte;


class Edu
{
  public:
    Edu();

    auto ExecuteProgram(uint16_t programId, uint16_t queueId, uint16_t timeout) -> EduErrorCode;
    auto GetStatus() -> EduStatus;
    auto UpdateTime(int32_t timestamp) -> EduErrorCode;
    auto StopProgram() -> EduErrorCode;
    auto ReturnResult(std::array<uint8_t, maxDataLength> & dest) -> ResultInfo;
    auto StoreArchive() -> int32_t;

  private:
    [[nodiscard]] auto SendData(std::span<Byte> data) -> EduErrorCode;
    auto SendCommand(uint8_t cmd) -> void;
    [[nodiscard]] auto UartReceive(std::span<Byte> destination) -> EduErrorCode;
    auto FlushUartBuffer() -> void;
    // RODOS::HAL_UART mEduUart_ = HAL_UART(hal::eduUartIndex, hal::eduUartTxPin,
    // hal::eduUartRxPin);
    RODOS::HAL_UART mEduUart_ =
        RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
};
}
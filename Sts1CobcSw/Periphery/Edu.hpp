#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>
#include <span>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::Byte;


class Edu
{
public:
    Edu();

    [[nodiscard]] auto StoreArchive(StoreArchiveData const & data) -> int32_t;
    [[nodiscard]] auto ExecuteProgram(ExecuteProgramData const & data) -> EduErrorCode;
    [[nodiscard]] auto StopProgram() -> EduErrorCode;
    // TODD: Find better name (or maybe even mechanism) for GetStatus
    [[nodiscard]] auto GetStatus() -> EduStatus;
    [[nodiscard]] auto ReturnResult() -> ResultInfo;
    [[nodiscard]] auto UpdateTime(UpdateTimeData const & data) -> EduErrorCode;

private:
    // TODO: Rework -> Send(EduBasicCommand command) -> void;
    auto SendCommand(uint8_t cmd) -> void;
    [[nodiscard]] auto SendData(std::span<Byte> data) -> EduErrorCode;
    [[nodiscard]] auto UartReceive(std::span<Byte> destination) -> EduErrorCode;
    auto FlushUartBuffer() -> void;

    // RODOS::HAL_UART mEduUart_ = HAL_UART(hal::eduUartIndex, hal::eduUartTxPin,
    // hal::eduUartRxPin);
    RODOS::HAL_UART mEduUart_ =
        RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
};
}
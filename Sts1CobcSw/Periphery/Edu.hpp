#pragma once


#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
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


// TODO: Think about const-correctness and whether to make uart_ mutable or not
//
// TODO: There is no reason for this to be a class (there is no class invariant), so this being a
// class just unnecessarily exposes the private members and functions to the user which makes
// mocking harder.
class Edu
{
public:
    auto Initialize() -> void;
    auto TurnOn() -> void;
    auto TurnOff() -> void;

    // TODO: Why does this return a std::int32_t?
    [[nodiscard]] auto StoreArchive(StoreArchiveData const & data) -> std::int32_t;
    [[nodiscard]] auto ExecuteProgram(ExecuteProgramData const & data) -> EduErrorCode;
    [[nodiscard]] auto StopProgram() -> EduErrorCode;
    // TODD: Find better name (or maybe even mechanism) for GetStatus
    [[nodiscard]] auto GetStatus() -> EduStatus;
    [[nodiscard]] auto ReturnResult() -> ResultInfo;
    [[nodiscard]] auto UpdateTime(UpdateTimeData const & data) -> EduErrorCode;

private:
    // TODO: Rework -> Send(EduBasicCommand command) -> void;
    auto SendCommand(Byte commandId) -> void;
    [[nodiscard]] auto SendData(std::span<Byte> data) -> EduErrorCode;
    // TODO: Make this read and return a Type instead of having to provide a destination. Use
    // Deserialize<>() internally.
    [[nodiscard]] auto UartReceive(std::span<Byte> destination) -> EduErrorCode;
    auto FlushUartBuffer() -> void;

    hal::GpioPin eduEnabledGpioPin_ = hal::GpioPin(hal::eduEnabledPin);
    // RODOS::HAL_UART uart_ = HAL_UART(hal::eduUartIndex, hal::eduUartTxPin,
    // hal::eduUartRxPin);
    RODOS::HAL_UART uart_ =
        RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
};
}

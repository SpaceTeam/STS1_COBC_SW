#pragma once


#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/EduEnums.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>
#include <span>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::Byte;


// Max. length of a single data packet
inline constexpr auto maxDataLength = 32768;
// Data buffer for potentially large data sizes (ReturnResult and StoreArchive)
extern std::array<Byte, maxDataLength> cepDataBuffer;


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
    [[nodiscard]] auto ReturnResult(ReturnResultData const & data) -> ResultInfo;
    [[nodiscard]] auto UpdateTime(UpdateTimeData const & data) -> EduErrorCode;

private:
    // TODO: Rework -> Send(EduBasicCommand command) -> void;
    auto SendCommand(Byte commandId) -> void;
    [[nodiscard]] auto SendData(std::span<Byte> data) -> EduErrorCode;
    // TODO: Make this read and return a Type instead of having to provide a destination. Use
    // Deserialize<>() internally.
    [[nodiscard]] auto UartReceive(std::span<Byte> destination) -> EduErrorCode;
    [[nodiscard]] auto UartReceive(void * destination) -> EduErrorCode;
    auto FlushUartBuffer() -> void;
    [[nodiscard]] auto CheckCrc32(std::span<Byte> data) -> EduErrorCode;
    [[nodiscard]] auto GetStatusCommunication() -> EduStatus;
    [[nodiscard]] auto ReturnResultCommunication() -> ResultInfo;
    [[nodiscard]] auto ReturnResultRetry() -> ResultInfo;
    void MockWriteToFile(std::span<Byte> data);

    hal::GpioPin eduEnableGpioPin_ = hal::GpioPin(hal::eduEnablePin);
    RODOS::HAL_UART uart_ =
        RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
    // RODOS::HAL_UART uart_ =
    //     RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
};
}

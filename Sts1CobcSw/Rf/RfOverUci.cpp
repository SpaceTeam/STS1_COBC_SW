#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>  // IWYU pragma: associated

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <utility>


namespace sts1cobcsw::rf
{
namespace
{
constexpr auto defaultDataRate = 115'200U;

auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
auto txIsEnabled = true;
auto rxDataRate = defaultDataRate;
auto txDataRate = defaultDataRate;
}


auto Initialize([[maybe_unused]] TxType txType) -> void
{
    hal::Initialize(&uciUart, defaultDataRate);
}


auto EnableTx() -> void
{
    txIsEnabled = true;
}


auto DisableTx() -> void
{
    txIsEnabled = false;
}


auto ReadPartNumber() -> std::uint16_t
{
    return rf::correctPartNumber;
}


auto EnterStandbyMode() -> void
{}  // Not required for UART


auto SetTxType([[maybe_unused]] TxType txType) -> void
{}  // Not required for UART


auto SetTxDataLength([[maybe_unused]] std::uint16_t length) -> void
{}  // Not required for UART


auto SetTxDataRate(std::uint32_t dataRate) -> void
{
    txDataRate = dataRate;
}


auto SetRxDataRate(std::uint32_t dataRate) -> void
{
    rxDataRate = dataRate;
}


auto GetTxDataRate() -> std::uint32_t
{
    return txDataRate;
}


auto GetRxDataRate() -> std::uint32_t
{
    return rxDataRate;
}


auto SendAndWait(std::span<Byte const> data) -> Result<void>
{
    if(not txIsEnabled)
    {
        // TODO: Think about what to do if TX is disabled
        return outcome_v2::success();
    }
    hal::Initialize(&uciUart, txDataRate);
    auto const timeout = static_cast<int>(data.size()) * s / txDataRate + 20 * ms;
    return hal::WriteTo(&uciUart, data, timeout);
}


// UART does not support send-and-continue semantics. It either busy waits, or suspends until the
// data is sent, so we just call SendAndWait().
auto SendAndContinue(std::span<Byte const> data) -> Result<void>
{
    return SendAndWait(data);
}


// UART does not support send-and-continue semantics. Every send either busy waits, or suspends
// until the data is sent, so this always returns success.
auto SuspendUntilDataSent([[maybe_unused]] Duration timeout) -> Result<void>
{
    return outcome_v2::success();
}


auto Receive(std::span<Byte> data, Duration timeout) -> Result<void>
{
    hal::Initialize(&uciUart, rxDataRate);
    return hal::ReadFrom(&uciUart, data, timeout);
}
}

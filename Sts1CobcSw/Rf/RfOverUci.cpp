#include <Sts1CobcSw/ChannelCoding/External/ConvolutionalCoding.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>  // IWYU pragma: associated
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <utility>


namespace sts1cobcsw::rf
{
namespace
{
// For more convenience during tests, we will not change the baud rate of the UCI UART with
// SetTx/RxDataRate(). We'll just store the RX and TX data rates and put them in the telemetry
// record.
constexpr auto uartBaudRate = 115'200U;
// To be able to distinguish between sending an RF frame and a string from RODOS::PRINTF() over the
// UART, we add the ASCII control character 0x02 = STX (Start of Text) at the start of each frame.
// We also add a linebreak at the end of each frame, for nicer formatting in HTerm.
constexpr auto startOfFrame = 0x02_b;
constexpr auto endOfFrame = std::array{0x0D_b, 0x0A_b};  // = \r\n
constexpr auto frameDelimiterTimeout = 10 * ms;  // Timeout for writing the start or end of frame

auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
auto rxDataRate = uartBaudRate;
auto txDataRate = uartBaudRate;
}


auto Initialize([[maybe_unused]] TxType txType) -> Result<void>
{
    hal::Initialize(&uciUart, uartBaudRate);
    // The real RF module takes > 100 ms to initialize so we simulate this with a busy wait. Without
    // this delay, the SPI supervisor test crashes for some reason when printing stack usage is
    // enabled. I suspect that RODOS::PRINTF() is non-blocking and with all the printing we overflow
    // the UART buffer.
    BusyWaitFor(100 * ms);  // NOLINT(*magic-numbers)
    return outcome_v2::success();
}


auto EnableTx() -> void
{
    persistentVariables.Store<"txIsOn">(true);
}


auto DisableTx() -> void
{
    persistentVariables.Store<"txIsOn">(false);
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


auto SendAndWait(std::span<Byte const> data) -> void
{
    if(not persistentVariables.Load<"txIsOn">())
    {
        DEBUG_PRINT("TX is off, not sending data\n");
        return;
    }
    auto result = [&]() -> Result<void>
    {
        auto convolutionalCoder = sts1cobcsw::cc::ViterbiCodec{};
        auto encodedData = convolutionalCoder.Encode(data, /*flush=*/true);
        auto const txByteRate = txDataRate / 10;
        static constexpr auto safetyFactor = 2;
        static constexpr auto safetyMargin = 10 * ms;
        auto const timeout =
            static_cast<int>(encodedData.size()) * s / txByteRate * safetyFactor + safetyMargin;
        OUTCOME_TRY(hal::WriteTo(&uciUart, Span(startOfFrame), frameDelimiterTimeout));
        OUTCOME_TRY(hal::WriteTo(&uciUart, Span(encodedData), timeout));
        return hal::WriteTo(&uciUart, Span(endOfFrame), frameDelimiterTimeout);
    }();
    if(result.has_error())
    {
        DEBUG_PRINT("Failed to send data: %s\n", ToCZString(result.error()));
        return;
    }
}


// UART does not support send-and-continue semantics. It either busy waits, or suspends until the
// data is sent, so we just call SendAndWait().
auto SendAndContinue(std::span<Byte const> data) -> void
{
    SendAndWait(data);
}


// UART does not support send-and-continue semantics. Every send either busy waits, or suspends
// until the data is sent, so this always returns success.
auto SuspendUntilDataSent([[maybe_unused]] Duration timeout) -> void
{}


auto Receive(std::span<Byte> data, Duration timeout) -> std::size_t
{
    auto result = hal::ReadFrom(&uciUart, data, timeout);
    // Add a line break after receiving the data, for nicer formatting in HTerm.
    (void)hal::WriteTo(&uciUart, Span(endOfFrame), frameDelimiterTimeout);
    if(result.has_error())
    {
        return 0;
    }
    return data.size();
}
}

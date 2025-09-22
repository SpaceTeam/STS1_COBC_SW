//! @file
//! @brief  Driver for the RF module Si4463.
//!
//! See "AN625: Si446x API Descriptions" for more information.


#include <Sts1CobcSw/Rf/Rf.hpp>

#include <Sts1CobcSw/ChannelCoding/External/ConvolutionalCoding.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>
#include <Sts1CobcSw/Rf/RfDataRate.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>  // IWYU pragma: keep
#include <Sts1CobcSw/Utility/FlatArray.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/WatchdogTimers/WatchdogTimers.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <bit>
#include <compare>
#include <concepts>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>


namespace sts1cobcsw::rf
{
namespace
{
// --- Private globals ---

constexpr auto endianness = std::endian::big;

// Commands
[[maybe_unused]] constexpr auto cmdPartInfo = 0x01_b;
[[maybe_unused]] constexpr auto cmdPowerUp = 0x02_b;
[[maybe_unused]] constexpr auto cmdFuncInfo = 0x10_b;
[[maybe_unused]] constexpr auto cmdSetProperty = 0x11_b;
[[maybe_unused]] constexpr auto cmdGetProperty = 0x12_b;
[[maybe_unused]] constexpr auto cmdGpioPinCfg = 0x13_b;
[[maybe_unused]] constexpr auto cmdFifoInfo = 0x15_b;
[[maybe_unused]] constexpr auto cmdGetIntStatus = 0x20_b;
[[maybe_unused]] constexpr auto cmdGetModemStatus = 0x22_b;
[[maybe_unused]] constexpr auto cmdStartTx = 0x31_b;
[[maybe_unused]] constexpr auto cmdStartRx = 0x32_b;
[[maybe_unused]] constexpr auto cmdRequestDeviceState = 0x33_b;
[[maybe_unused]] constexpr auto cmdChangeState = 0x34_b;
[[maybe_unused]] constexpr auto cmdReadCmdBuff = 0x44_b;
[[maybe_unused]] constexpr auto cmdWriteTxFifo = 0x66_b;
[[maybe_unused]] constexpr auto cmdReadRxFifo = 0x77_b;

// Command answer lengths
//
// TODO: We should do it similarly to SimpleInstruction and SendInstruction() in Flash.cpp. Unless
// this remains the only command with an answer. Then we should probably get rid of SendCommand<>()
// instead.
[[maybe_unused]] constexpr auto partInfoAnswerLength = 8U;
[[maybe_unused]] constexpr auto fifoInfoAnswerLength = 2U;
[[maybe_unused]] constexpr auto interruptStatusAnswerLength = 8U;
[[maybe_unused]] constexpr auto modemStatusAnswerLength = 8U;

// Property indexes and sizes
[[maybe_unused]] constexpr auto iModemDataRate = 0x03_b;
[[maybe_unused]] constexpr auto modemDataRateSize = 3U;

// Packet handler interrupt flags
[[maybe_unused]] constexpr auto noInterrupts = 0x00_b;
[[maybe_unused]] constexpr auto rxFifoAlmostFullInterrupt = Byte{1U << 0U};
[[maybe_unused]] constexpr auto txFifoAlmostEmptyInterrupt = Byte{1U << 1U};
[[maybe_unused]] constexpr auto crcErrorInterrupt = Byte{1U << 3U};
[[maybe_unused]] constexpr auto packetRxInterrupt = Byte{1U << 4U};
[[maybe_unused]] constexpr auto packetSentInterrupt = Byte{1U << 5U};
[[maybe_unused]] constexpr auto filterMissInterrupt = Byte{1U << 6U};
[[maybe_unused]] constexpr auto filterMatchInterrupt = Byte{1U << 7U};

// Modem interrupt flags
[[maybe_unused]] constexpr auto syncDetectInterrupt = Byte{1U << 0U};
[[maybe_unused]] constexpr auto preambleDetectInterrupt = Byte{1U << 1U};
[[maybe_unused]] constexpr auto invalidPreambleInterrupt = Byte{1U << 2U};
[[maybe_unused]] constexpr auto rssiInterrupt = Byte{1U << 3U};
[[maybe_unused]] constexpr auto rssiJumpInterrupt = Byte{1U << 4U};
[[maybe_unused]] constexpr auto invalidSyncInterrupt = Byte{1U << 5U};

[[maybe_unused]] constexpr auto txFifoSize = 64U;
[[maybe_unused]] constexpr auto rxFifoSize = 64U;
constexpr auto txFifoThreshold = 48U;  // Free space that triggers TX FIFO almost empty interrupt
constexpr auto rxFifoThreshold = 32U;  // Stored bytes trigger RX FIFO almost full interrupt

// TODO: Split into fifoAlmostEmptyTimeout and dataSentTimeout and use shorter timeouts for both
constexpr auto interruptTimeout = 1 * s;
constexpr auto spiTimeout = 1 * ms;
constexpr auto ctsTimeout = 100 * ms;
constexpr auto pollingInterval = 10 * us;

// Minimum time the SDN pin must be pulled high to reset the RF module (double the value from the
// datasheet to be safe)
constexpr auto minShutdownDuration = 20 * us;
constexpr auto porCircuitSettleDelay = 100 * ms;  // Time for PoR circuit to settle after power up
constexpr auto porRunningDelay = 20 * ms;         // Time for power on reset to finish

constexpr auto postTxDelay = 100 * ms;  // Time to wait after sending data to prevent RX issues

constexpr auto errorHandlingRetryDelay = 1 * ms;
constexpr auto errorHandlingCobcResetDelay = 1 * s;

auto csGpioPin = hal::GpioPin(hal::rfCsPin);
auto nirqGpioPin = hal::GpioPin(hal::rfNirqPin);
auto sdnGpioPin = hal::GpioPin(hal::rfSdnPin);
auto gpio0GpioPin = hal::GpioPin(hal::rfGpio0Pin);
auto gpio1GpioPin = hal::GpioPin(hal::rfGpio1Pin);
auto paEnablePin = hal::GpioPin(hal::rfPaEnablePin);
#if 27 <= HW_VERSION and HW_VERSION < 30
auto rfLatchupDisableGpioPin = hal::GpioPin(hal::rfLatchupDisablePin);
#endif
#if 30 <= HW_VERSION
auto rfLatchupDisableGpioPin1 = hal::GpioPin(hal::rfLatchupDisablePin1);
auto rfLatchupDisableGpioPin2 = hal::GpioPin(hal::rfLatchupDisablePin2);
#endif

auto isInTxMode = false;
// TODO: Use 9600 by default
auto rxDataRateConfig = dataRateConfig1200;
auto txDataRateConfig = dataRateConfig1200;
auto currentDataRate = dataRateConfig1200.dataRate;


using ModemStatus = std::array<Byte, modemStatusAnswerLength>;


// --- Private function declarations ---

template<auto doFunction, typename... Args>
    requires std::invocable<decltype(doFunction), Args...>
auto ExecuteWithRecovery(Args... args)
    -> std::invoke_result_t<decltype(doFunction), Args...>::value_type;


[[nodiscard]] auto DoInitialize() -> Result<void>;
[[nodiscard]] auto DoReadPartNumber() -> Result<std::uint16_t>;
[[nodiscard]] auto DoEnterStandbyMode() -> Result<void>;
[[nodiscard]] auto DoSetTxDataLength(std::uint16_t length) -> Result<void>;
[[nodiscard]] auto DoSendAndWait(std::span<Byte const> data) -> Result<void>;
[[nodiscard]] auto DoSendAndContinue(std::span<Byte const> data) -> Result<void>;
[[nodiscard]] auto DoSuspendUntilDataSent(Duration timeout) -> Result<void>;
// Return the number of received bytes
[[nodiscard]] auto DoReceive(std::span<Byte> data, Duration timeout) -> Result<std::size_t>;

auto Reset() -> void;
auto InitializeGpiosAndSpi() -> void;
[[nodiscard]] auto ApplyPatch() -> Result<void>;
[[nodiscard]] auto PowerUp() -> Result<void>;
[[nodiscard]] auto Configure() -> Result<void>;
[[nodiscard]] auto SetConstantModemProperties() -> Result<void>;

[[nodiscard]] auto GetDataRateConfig(std::uint32_t dataRate) -> DataRateConfig;
[[nodiscard]] auto SetDataRate(DataRateConfig const & dataRateConfig) -> Result<void>;

auto EnableRfLatchupProtection() -> void;
auto DisableRfLatchupProtection() -> void;

[[nodiscard]] auto SetRxFifoThreshold(Byte threshold) -> Result<void>;
[[nodiscard]] auto SetPacketHandlerInterrupts(Byte interruptFlags) -> Result<void>;
// auto SetModemInterrupts(Byte interruptFlags) -> void;
auto ReadAndClearInterruptStatus() -> Result<std::array<Byte, interruptStatusAnswerLength>>;
[[nodiscard]] auto SuspendUntilInterrupt(RodosTime reactivationTime) -> Result<void>;
[[nodiscard]] auto SuspendUntilInterrupt(Duration timeout) -> Result<void>;

[[nodiscard]] auto StartTx() -> Result<void>;
[[nodiscard]] auto StartRx() -> Result<void>;

[[nodiscard]] auto ResetFifos() -> Result<void>;
[[nodiscard]] auto WriteToFifo(std::span<Byte const> data) -> Result<void>;
auto ReadFromFifo(std::span<Byte> data) -> void;
[[nodiscard]] auto ReadFreeTxFifoSpace() -> Result<std::uint8_t>;
[[nodiscard]] auto ReadRxFifoFillLevel() -> Result<std::uint8_t>;

auto DebugPrintModemStatus() -> void;
[[nodiscard]] auto ReadModemStatus() -> Result<ModemStatus>;

[[nodiscard]] auto SendCommand(std::span<Byte const> data) -> Result<void>;
template<std::size_t answerLength>
[[nodiscard]] auto SendCommand(std::span<Byte const> data)
    -> Result<std::array<Byte, answerLength>>;
auto SelectChip() -> void;
auto DeselectChip() -> void;
[[nodiscard]] auto BusyWaitForCts(Duration timeout) -> Result<void>;
template<std::size_t answerLength>
[[nodiscard]] auto BusyWaitForAnswer(Duration timeout) -> Result<std::array<Byte, answerLength>>;

template<std::size_t extent>
    requires(extent <= maxNProperties)
[[nodiscard]] auto SetProperties(PropertyGroup propertyGroup,
                                 Byte startIndex,
                                 std::span<Byte const, extent> propertyValues) -> Result<void>;
template<std::size_t nProperties>
    requires(nProperties <= maxNProperties)
[[nodiscard]] auto SetProperties(PropertyGroup propertyGroup,
                                 Byte propertyStartIndex,
                                 std::array<Byte, nProperties> const & properties) -> Result<void>;
template<PropertyGroup propertyGroup, sts1cobcsw::Byte propertyStartIndex, std::size_t nProperties>
[[nodiscard]] auto SetProperties(
    Properties<propertyGroup, propertyStartIndex, nProperties> property) -> Result<void>;
template<std::size_t size>
    requires(size <= maxNProperties)
[[nodiscard]] auto GetProperties(PropertyGroup propertyGroup, Byte startIndex)
    -> Result<std::array<Byte, size>>;
}


// --- Public function definitions ---

auto Initialize() -> Result<void>
{
    return DoInitialize();
}


auto EnableTx() -> void
{
    persistentVariables.Store<"txIsOn">(true);
    paEnablePin.Set();
}


auto DisableTx() -> void
{
    persistentVariables.Store<"txIsOn">(false);
    paEnablePin.Reset();
}


auto ReadPartNumber() -> std::uint16_t
{
    return ExecuteWithRecovery<DoReadPartNumber>();
}


auto EnterStandbyMode() -> void
{
    ExecuteWithRecovery<DoEnterStandbyMode>();
}


// Must be called before SendAndContinue()
auto SetTxDataLength(std::uint16_t length) -> void
{
    ExecuteWithRecovery<DoSetTxDataLength>(length);
}


auto SetTxDataRate(std::uint32_t dataRate) -> void
{
    txDataRateConfig = GetDataRateConfig(dataRate);
}


auto SetRxDataRate(std::uint32_t dataRate) -> void
{
    rxDataRateConfig = GetDataRateConfig(dataRate);
}


auto GetTxDataRate() -> std::uint32_t
{
    return txDataRateConfig.dataRate;
}


auto GetRxDataRate() -> std::uint32_t
{
    return rxDataRateConfig.dataRate;
}


auto SendAndWait(std::span<Byte const> data) -> void
{
    ExecuteWithRecovery<DoSendAndWait>(data);
}


// Send the data to the RF module and return as soon as the last chunk was written to the FIFO. This
// allows sending multiple data packets without interruption.
auto SendAndContinue(std::span<Byte const> data) -> void
{
    ExecuteWithRecovery<DoSendAndContinue>(data);
}


auto SuspendUntilDataSent(Duration timeout) -> void
{
    ExecuteWithRecovery<DoSuspendUntilDataSent>(timeout);
}


auto Receive(std::span<Byte> data, Duration timeout) -> std::size_t
{
    return ExecuteWithRecovery<DoReceive>(data, timeout);
}


// --- Private function definitions ---

namespace
{
template<auto doFunction, typename... Args>
    requires std::invocable<decltype(doFunction), Args...>
auto ExecuteWithRecovery(Args... args)
    -> std::invoke_result_t<decltype(doFunction), Args...>::value_type
{
    // First try
    auto result = doFunction(args...);
    if(result.has_value())
    {
        return result.value();
    }
    SuspendFor(errorHandlingRetryDelay);
    // Second try
    result = doFunction(args...);
    if(result.has_value())
    {
        return result.value();
    }
    Reset();
    auto reinitResult = DoInitialize();
    if(reinitResult.has_value())
    {
        // Third try
        result = doFunction(args...);
        if(result.has_value())
        {
            return result.value();
        }
    }
    // Reset COBC
    SuspendFor(errorHandlingCobcResetDelay);
    RODOS::hwResetAndReboot();
    __builtin_unreachable();  // Tell the compiler that hwResetAndReboot() never returns
}


auto DoInitialize() -> Result<void>
{
    InitializeGpiosAndSpi();
    OUTCOME_TRY(ApplyPatch());
    OUTCOME_TRY(PowerUp());
    OUTCOME_TRY(Configure());
    OUTCOME_TRY(SetConstantModemProperties());
    OUTCOME_TRY(SetDataRate(txDataRateConfig));
    persistentVariables.Load<"txIsOn">() ? EnableTx() : DisableTx();
    return outcome_v2::success();
}


auto DoReadPartNumber() -> Result<std::uint16_t>
{
    OUTCOME_TRY(auto answer, SendCommand<partInfoAnswerLength>(Span(cmdPartInfo)));
    return Deserialize<endianness, std::uint16_t>(Span(answer).subspan<1, 2>());
}


auto DoEnterStandbyMode() -> Result<void>
{
    static constexpr auto standbyMode = 0x01_b;
    OUTCOME_TRY(SendCommand(Span({cmdChangeState, standbyMode})));
    isInTxMode = false;
    EnableRfLatchupProtection();
    return outcome_v2::success();
}


auto DoSetTxDataLength(std::uint16_t length) -> Result<void>
{
    static constexpr auto iPktField1Length = 0x0D_b;
    auto encodedLength = static_cast<uint16_t>(cc::ViterbiCodec::EncodedSize(length, true));
    return SetProperties(
        PropertyGroup::pkt, iPktField1Length, Span(Serialize<endianness>(encodedLength)));
}


auto DoSendAndWait(std::span<Byte const> data) -> Result<void>
{
    if(not persistentVariables.Load<"txIsOn">())
    {
        return outcome_v2::success();
    }
    OUTCOME_TRY(DoSetTxDataLength(static_cast<std::uint16_t>(data.size())));
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(DoSendAndContinue(data));
        auto suspendUntilDataSentResult = DoSuspendUntilDataSent(interruptTimeout);
        return suspendUntilDataSentResult;
    }();
    OUTCOME_TRY(DoEnterStandbyMode());
    return result;
}


// The high cognitive complexity comes from the OUTCOME_TRY macros which expand to extra if
// statements. However, you don't see them when reading the code so they shouldn't really count.
// Also, even extracting the interrupt driven part of the sending into a separate function doesn't
// help, since that function alone would have too high of a cognitive complexity.
//
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto DoSendAndContinue(std::span<Byte const> data) -> Result<void>
{
    if(not persistentVariables.Load<"txIsOn">())
    {
        return outcome_v2::success();
    }
    if(currentDataRate != txDataRateConfig.dataRate)
    {
        OUTCOME_TRY(SetDataRate(txDataRateConfig));
        currentDataRate = txDataRateConfig.dataRate;
    }
    if(not isInTxMode)
    {
        OUTCOME_TRY(ResetFifos());
        DisableRfLatchupProtection();
    }
    auto dataIndex = 0U;
    static auto convolutionalCoder = cc::ViterbiCodec{};
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(SetPacketHandlerInterrupts(txFifoAlmostEmptyInterrupt));
        OUTCOME_TRY(auto freeSpace, ReadFreeTxFifoSpace());
        auto chunkSize = cc::ViterbiCodec::UnencodedSize(freeSpace, true);
        while(dataIndex + chunkSize < static_cast<unsigned int>(data.size()))
        {
            auto encodedChunk =
                convolutionalCoder.Encode(data.subspan(dataIndex, chunkSize), /*flush=*/false);
            OUTCOME_TRY(WriteToFifo(encodedChunk));
            OUTCOME_TRY(ReadAndClearInterruptStatus());
            if(not isInTxMode)
            {
                OUTCOME_TRY(StartTx());
            }
            dataIndex += chunkSize;
            OUTCOME_TRY(SuspendUntilInterrupt(interruptTimeout));
            OUTCOME_TRY(freeSpace, ReadFreeTxFifoSpace());
            chunkSize = cc::ViterbiCodec::UnencodedSize(freeSpace, true);
        }
        return outcome_v2::success();
    }();
    OUTCOME_TRY(SetPacketHandlerInterrupts(noInterrupts));
    if(result.has_error())
    {
        return result;
    }
    auto encodedData = convolutionalCoder.Encode(data.subspan(dataIndex), /*flush=*/true);
    OUTCOME_TRY(WriteToFifo(encodedData));
    if(not isInTxMode)
    {
        OUTCOME_TRY(StartTx());
    }
    return outcome_v2::success();
}


auto DoSuspendUntilDataSent(Duration timeout) -> Result<void>
{
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(SetPacketHandlerInterrupts(packetSentInterrupt));
        static constexpr auto iPacketHandlerStatus = 3;
        OUTCOME_TRY(auto interruptStatus, ReadAndClearInterruptStatus());
        auto packetHandlerStatus = interruptStatus[iPacketHandlerStatus];
        if((packetHandlerStatus & packetSentInterrupt) == 0x00_b)
        {
            OUTCOME_TRY(SuspendUntilInterrupt(timeout));
        }
        return outcome_v2::success();
    }();
    // We won't stay in TX mode, no matter if the transmission was completed successfully or not
    isInTxMode = false;
    EnableRfLatchupProtection();
    // If we don't wait here, receiving does not work. According to Jakob it might be related to the
    // power amplifier somehow.
    SuspendFor(postTxDelay);
    OUTCOME_TRY(SetPacketHandlerInterrupts(noInterrupts));
    return result;
}


// I don't care too much about the high cognitive complexity here for the same reason as in
// DoSendAndContinue().
//
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto DoReceive(std::span<Byte> data, Duration timeout) -> Result<std::size_t>
{
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    auto result = [&]() -> Result<std::size_t>
    {
        if(currentDataRate != rxDataRateConfig.dataRate)
        {
            OUTCOME_TRY(SetDataRate(rxDataRateConfig));
            currentDataRate = rxDataRateConfig.dataRate;
        }
        OUTCOME_TRY(ResetFifos());
        OUTCOME_TRY(SetPacketHandlerInterrupts(rxFifoAlmostFullInterrupt));
        OUTCOME_TRY(ReadAndClearInterruptStatus());
        DisableRfLatchupProtection();
        auto reactivationTime = CurrentRodosTime() + timeout;
        OUTCOME_TRY(StartRx());
        DebugPrintModemStatus();
        auto dataIndex = 0U;
        while(dataIndex + rxFifoThreshold < static_cast<unsigned int>(data.size()))
        {
            auto suspendUntilInterruptResult = SuspendUntilInterrupt(reactivationTime);
            DebugPrintModemStatus();
            if(suspendUntilInterruptResult.has_error())
            {
                return dataIndex;
            }
            OUTCOME_TRY(ReadAndClearInterruptStatus());
            ReadFromFifo(data.subspan(dataIndex, rxFifoThreshold));
            dataIndex += rxFifoThreshold;
        }
        auto remainingData = data.subspan(dataIndex);
        OUTCOME_TRY(SetRxFifoThreshold(static_cast<Byte>(remainingData.size())));
        OUTCOME_TRY(auto fillLevel, ReadRxFifoFillLevel());
        if(fillLevel < remainingData.size())
        {
            auto suspendUntilInterruptResult = SuspendUntilInterrupt(reactivationTime);
            if(suspendUntilInterruptResult.has_error())
            {
                return dataIndex;
            }
        }
        ReadFromFifo(remainingData);
        OUTCOME_TRY(SetRxFifoThreshold(static_cast<Byte>(rxFifoThreshold)));
        dataIndex += remainingData.size();
        return dataIndex;
    }();
    auto setInterruptsResult = SetPacketHandlerInterrupts(noInterrupts);
    OUTCOME_TRY(DoEnterStandbyMode());
    if(setInterruptsResult.has_error())
    {
        return setInterruptsResult.error();
    }
    OUTCOME_TRY(ReadAndClearInterruptStatus());
    return result;
}


auto Reset() -> void
{
    sdnGpioPin.Set();
    SuspendFor(minShutdownDuration);
    sdnGpioPin.Reset();
}


auto InitializeGpiosAndSpi() -> void
{
    csGpioPin.SetDirection(hal::PinDirection::out);
#if HW_VERSION >= 30
    csGpioPin.SetOutputType(hal::PinOutputType::openDrain);
#else
    csGpioPin.SetOutputType(hal::PinOutputType::pushPull);
#endif
    csGpioPin.Set();
    nirqGpioPin.SetDirection(hal::PinDirection::in);
    nirqGpioPin.SetInterruptSensitivity(hal::InterruptSensitivity::fallingEdge);
    nirqGpioPin.DisableInterrupts();
    sdnGpioPin.SetDirection(hal::PinDirection::out);
    sdnGpioPin.Set();
    gpio0GpioPin.SetDirection(hal::PinDirection::out);
    gpio0GpioPin.Reset();
    paEnablePin.SetDirection(hal::PinDirection::out);
    paEnablePin.Reset();
#if 27 <= HW_VERSION and HW_VERSION < 30
    rfLatchupDisableGpioPin.SetDirection(hal::PinDirection::out);
#endif
#if 30 <= HW_VERSION
    rfLatchupDisableGpioPin1.SetDirection(hal::PinDirection::out);
    rfLatchupDisableGpioPin2.SetDirection(hal::PinDirection::out);
#endif
    EnableRfLatchupProtection();
    wdt::Initialize();
    // The watchdog must be fed regularly for the TX to work. Even without the watchdog timer on
    // the PCB it needs to be triggered at least once after boot to enable the TX.
    wdt::Feed();

    constexpr auto baudrate = 6'000'000;
#if HW_VERSION >= 30
    Initialize(&rfSpi, baudrate, /*useOpenDrainOutputs=*/true);
#else
    Initialize(&rfSpi, baudrate, /*useOpenDrainOutputs=*/false);
#endif

    // Enable Si4463 and wait for PoR to finish
    SuspendFor(porCircuitSettleDelay);
    sdnGpioPin.Reset();
    SuspendFor(porRunningDelay);
}


// This must be called after reset of the RF module but before rf::PowerUp()
auto ApplyPatch() -> Result<void>
{
    // Template argument deduction for std::array doesn't work because the array is too large. Lol.
    // Every line of the patch array starts with a one-byte length and then as many data bytes. We
    // got the patch data from some configuration tool that Andriy found.
    //
    // clang-format off
    static constexpr auto patch = std::to_array<Byte>({
        0x08_b, 0x04_b, 0x21_b, 0x71_b, 0x4B_b, 0x00_b, 0x00_b, 0xDC_b, 0x95_b,
        0x08_b, 0x05_b, 0xA6_b, 0x22_b, 0x21_b, 0xF0_b, 0x41_b, 0x5B_b, 0x26_b,
        0x08_b, 0xE2_b, 0x2F_b, 0x1C_b, 0xBB_b, 0x0A_b, 0xA8_b, 0x94_b, 0x28_b,
        0x08_b, 0x05_b, 0x87_b, 0x67_b, 0xE2_b, 0x58_b, 0x1A_b, 0x07_b, 0x5B_b,
        0x08_b, 0xE1_b, 0xD0_b, 0x72_b, 0xD8_b, 0x8A_b, 0xB8_b, 0x5B_b, 0x7D_b,
        0x08_b, 0x05_b, 0x11_b, 0xEC_b, 0x9E_b, 0x28_b, 0x23_b, 0x1B_b, 0x6D_b,
        0x08_b, 0xE2_b, 0x4F_b, 0x8A_b, 0xB2_b, 0xA9_b, 0x29_b, 0x14_b, 0x13_b,
        0x08_b, 0x05_b, 0xD1_b, 0x2E_b, 0x71_b, 0x6A_b, 0x51_b, 0x4C_b, 0x2C_b,
        0x08_b, 0xE5_b, 0x80_b, 0x27_b, 0x42_b, 0xA4_b, 0x69_b, 0xB0_b, 0x7F_b,
        0x08_b, 0x05_b, 0xAA_b, 0x81_b, 0x2A_b, 0xBD_b, 0x45_b, 0xE8_b, 0xA8_b,
        0x08_b, 0xEA_b, 0xE4_b, 0xF0_b, 0x24_b, 0xC9_b, 0x9F_b, 0xCC_b, 0x3C_b,
        0x08_b, 0x05_b, 0x08_b, 0xF5_b, 0x05_b, 0x04_b, 0x27_b, 0x62_b, 0x98_b,
        0x08_b, 0xEA_b, 0x6B_b, 0x62_b, 0x84_b, 0xA1_b, 0xF9_b, 0x4A_b, 0xE2_b,
        0x08_b, 0x05_b, 0xE9_b, 0x77_b, 0x05_b, 0x4F_b, 0x84_b, 0xEE_b, 0x35_b,
        0x08_b, 0xE2_b, 0x43_b, 0xC3_b, 0x8D_b, 0xFB_b, 0xAD_b, 0x54_b, 0x25_b,
        0x08_b, 0x05_b, 0x14_b, 0x06_b, 0x5E_b, 0x39_b, 0x36_b, 0x2F_b, 0x45_b,
        0x08_b, 0xEA_b, 0x0C_b, 0x1C_b, 0x74_b, 0xD0_b, 0x11_b, 0xFC_b, 0x32_b,
        0x08_b, 0x05_b, 0xDA_b, 0x38_b, 0xBA_b, 0x0E_b, 0x3C_b, 0xE7_b, 0x8B_b,
        0x08_b, 0xEA_b, 0xB0_b, 0x09_b, 0xE6_b, 0xFF_b, 0x94_b, 0xBB_b, 0xA9_b,
        0x08_b, 0x05_b, 0xD7_b, 0x11_b, 0x29_b, 0xFE_b, 0xDC_b, 0x71_b, 0xD5_b,
        0x08_b, 0xEA_b, 0x7F_b, 0x83_b, 0xA7_b, 0x60_b, 0x90_b, 0x62_b, 0x18_b,
        0x08_b, 0x05_b, 0x84_b, 0x7F_b, 0x6A_b, 0xD1_b, 0x91_b, 0xC6_b, 0x52_b,
        0x08_b, 0xEA_b, 0x2A_b, 0xD8_b, 0x7B_b, 0x8E_b, 0x4A_b, 0x9F_b, 0x91_b,
        0x08_b, 0x05_b, 0xBD_b, 0xAA_b, 0x9D_b, 0x16_b, 0x18_b, 0x06_b, 0x15_b,
        0x08_b, 0xE2_b, 0x55_b, 0xAD_b, 0x2D_b, 0x0A_b, 0x14_b, 0x1F_b, 0x5D_b,
        0x08_b, 0x05_b, 0xD3_b, 0xE0_b, 0x7C_b, 0x39_b, 0xCF_b, 0x01_b, 0xF0_b,
        0x08_b, 0xEF_b, 0x3A_b, 0x91_b, 0x72_b, 0x6A_b, 0x03_b, 0xBB_b, 0x96_b,
        0x08_b, 0xE7_b, 0x83_b, 0x6D_b, 0xA4_b, 0x92_b, 0xFC_b, 0x13_b, 0xA7_b,
        0x08_b, 0xEF_b, 0xF8_b, 0xFD_b, 0xCF_b, 0x62_b, 0x07_b, 0x6F_b, 0x1E_b,
        0x08_b, 0xE7_b, 0x4C_b, 0xEA_b, 0x4A_b, 0x75_b, 0x4F_b, 0xD6_b, 0xCF_b,
        0x08_b, 0xE2_b, 0xF6_b, 0x11_b, 0xE4_b, 0x26_b, 0x0D_b, 0x4D_b, 0xC6_b,
        0x08_b, 0x05_b, 0xFB_b, 0xBF_b, 0xE8_b, 0x07_b, 0x89_b, 0xC3_b, 0x51_b,
        0x08_b, 0xEF_b, 0x82_b, 0x27_b, 0x04_b, 0x3F_b, 0x96_b, 0xA8_b, 0x58_b,
        0x08_b, 0xE7_b, 0x41_b, 0x29_b, 0x3C_b, 0x75_b, 0x2A_b, 0x03_b, 0x1C_b,
        0x08_b, 0xEF_b, 0xAF_b, 0x59_b, 0x98_b, 0x36_b, 0xAA_b, 0x0F_b, 0x06_b,
        0x08_b, 0xE6_b, 0xF6_b, 0x93_b, 0x41_b, 0x2D_b, 0xEC_b, 0x0E_b, 0x99_b,
        0x08_b, 0x05_b, 0x29_b, 0x19_b, 0x90_b, 0xE5_b, 0xAA_b, 0x36_b, 0x40_b,
        0x08_b, 0xE7_b, 0xFB_b, 0x68_b, 0x10_b, 0x7D_b, 0x77_b, 0x5D_b, 0xC0_b,
        0x08_b, 0xE7_b, 0xCB_b, 0xB4_b, 0xDD_b, 0xCE_b, 0x90_b, 0x54_b, 0xBE_b,
        0x08_b, 0xE7_b, 0x72_b, 0x8A_b, 0xD6_b, 0x02_b, 0xF4_b, 0xDD_b, 0xCC_b,
        0x08_b, 0xE7_b, 0x6A_b, 0x21_b, 0x0B_b, 0x02_b, 0x86_b, 0xEC_b, 0x15_b,
        0x08_b, 0xE7_b, 0x7B_b, 0x7C_b, 0x3D_b, 0x6B_b, 0x81_b, 0x03_b, 0xD0_b,
        0x08_b, 0xEF_b, 0x7D_b, 0x61_b, 0x36_b, 0x94_b, 0x7C_b, 0xA0_b, 0xDF_b,
        0x08_b, 0xEF_b, 0xCC_b, 0x85_b, 0x3B_b, 0xDA_b, 0xE0_b, 0x5C_b, 0x1C_b,
        0x08_b, 0xE7_b, 0xE3_b, 0x75_b, 0xBB_b, 0x39_b, 0x22_b, 0x4B_b, 0xA8_b,
        0x08_b, 0xEF_b, 0xF9_b, 0xCE_b, 0xE0_b, 0x5E_b, 0xEB_b, 0x1D_b, 0xCB_b,
        0x08_b, 0xE7_b, 0xBD_b, 0xE2_b, 0x70_b, 0xD5_b, 0xAB_b, 0x4E_b, 0x3F_b,
        0x08_b, 0xE7_b, 0xB7_b, 0x8D_b, 0x20_b, 0x68_b, 0x6B_b, 0x09_b, 0x52_b,
        0x08_b, 0xEF_b, 0xA1_b, 0x1B_b, 0x90_b, 0xCD_b, 0x98_b, 0x00_b, 0x63_b,
        0x08_b, 0xEF_b, 0x54_b, 0x67_b, 0x5D_b, 0x9C_b, 0x11_b, 0xFC_b, 0x45_b,
        0x08_b, 0xE7_b, 0xD4_b, 0x9B_b, 0xC8_b, 0x97_b, 0xBE_b, 0x8A_b, 0x07_b,
        0x08_b, 0xEF_b, 0x52_b, 0x8D_b, 0x90_b, 0x63_b, 0x73_b, 0xD5_b, 0x2A_b,
        0x08_b, 0xEF_b, 0x03_b, 0xBC_b, 0x6E_b, 0x1C_b, 0x76_b, 0xBE_b, 0x4A_b,
        0x08_b, 0xE7_b, 0xC2_b, 0xED_b, 0x67_b, 0xBA_b, 0x5E_b, 0x66_b, 0x21_b,
        0x08_b, 0xEF_b, 0xE7_b, 0x3F_b, 0x87_b, 0xBE_b, 0xE0_b, 0x7A_b, 0x6D_b,
        0x08_b, 0xE7_b, 0xC9_b, 0x70_b, 0x93_b, 0x1D_b, 0x64_b, 0xF5_b, 0x6C_b,
        0x08_b, 0xEF_b, 0xF5_b, 0x28_b, 0x08_b, 0x34_b, 0xB3_b, 0xB6_b, 0x2C_b,
        0x08_b, 0xEF_b, 0x3A_b, 0x0A_b, 0xEC_b, 0x0F_b, 0xDB_b, 0x56_b, 0xCA_b,
        0x08_b, 0xEF_b, 0x39_b, 0xA0_b, 0x6E_b, 0xED_b, 0x79_b, 0xD0_b, 0x24_b,
        0x08_b, 0xE7_b, 0x6C_b, 0x0B_b, 0xAF_b, 0xA9_b, 0x4E_b, 0x40_b, 0xB5_b,
        0x08_b, 0xE9_b, 0xB9_b, 0xAF_b, 0xBF_b, 0x25_b, 0x50_b, 0xD1_b, 0x37_b,
        0x08_b, 0x05_b, 0x9E_b, 0xDB_b, 0xDE_b, 0x3F_b, 0x94_b, 0xE9_b, 0x6B_b,
        0x08_b, 0xEC_b, 0xC5_b, 0x05_b, 0xAA_b, 0x57_b, 0xDC_b, 0x8A_b, 0x5E_b,
        0x08_b, 0x05_b, 0x70_b, 0xDA_b, 0x84_b, 0x84_b, 0xDD_b, 0xCA_b, 0x90_b
    });
    // clang-format on

    // We assume that the data length is the same for every line.
    static constexpr auto dataLength = static_cast<std::uint8_t>(patch[0]);
    static_assert(patch.size() % (dataLength + 1U) == 0);
    for(auto i = 1U; i < patch.size(); i += dataLength + 1U)
    {
        OUTCOME_TRY(SendCommand(Span(patch).subspan(i, dataLength)));
    }
    return outcome_v2::success();
}


auto PowerUp() -> Result<void>
{
    static constexpr auto bootOptions = 0x81_b;
    static constexpr auto xtalOptions = 0x01_b;          // Use external oscillator
    static constexpr std::uint32_t xoFreq = 26'000'000;  // MHz
    return SendCommand(FlatArray(
        cmdPowerUp, bootOptions, xtalOptions, Serialize<endianness, std::uint32_t>(xoFreq)));
}


// TODO: values starting with modem* will now be set in SetConstantModemProperties & SetDataRate
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto Configure() -> Result<void>
{
    // Configure GPIO pins, NIRQ, and SDO
    // Weak pull-up enabled, no function (tristate)
    static constexpr auto gpio0Config = 0x41_b;
    // Weak pull-up enabled, no function (tristate)
    static constexpr auto gpio1Config = 0x41_b;
    // GPIO2 active in RX state
    static constexpr auto gpio2Config = 0x21_b;
    // GPIO3 active in TX state
    static constexpr auto gpio3Config = 0x20_b;
    // NIRQ is still used as NIRQ but enable internal pull-up
    static constexpr auto nirqConfig = 0x67_b;
    // SDO is still used as SDO but enable internal pull-up
    static constexpr auto sdoConfig = 0x4B_b;
    // GPIOs configured as outputs will have highest drive strength
    static constexpr auto genConfig = 0x00_b;
    OUTCOME_TRY(SendCommand(Span({cmdGpioPinCfg,
                                  gpio0Config,
                                  gpio1Config,
                                  gpio2Config,
                                  gpio3Config,
                                  nirqConfig,
                                  sdoConfig,
                                  genConfig})));

    // Crystal oscillator frequency and clock
    static constexpr auto iGlobalXoTune = 0x00_b;
    static constexpr auto globalXoTune = 0x52_b;
    static constexpr auto globalClkCfg = 0x00_b;
    OUTCOME_TRY(
        SetProperties(PropertyGroup::global, iGlobalXoTune, Span({globalXoTune, globalClkCfg})));

    // Global config
    static constexpr auto iGlobalConfig = 0x03_b;
    // High performance mode, generic packet format, split FIFO mode, fast sequencer mode
    static constexpr auto globalConfig = 0x60_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::global, iGlobalConfig, Span(globalConfig)));

    // Interrupt
    static constexpr auto iIntCtlEnable = 0x00_b;
    // Enable all three general interrupt sources (chip, modem, packet handler)
    static constexpr auto intCtlEnable = 0x07_b;
    // Disable all interrupts
    // TODO: We could enable the chip ready interrupt to potentially remove some hardcoded delays?
    static constexpr auto intPhEnable = 0x00_b;
    static constexpr auto intModemEnable = 0x00_b;
    static constexpr auto intChipEnable = 0x00_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::intCtl,
                              iIntCtlEnable,
                              Span({intCtlEnable, intPhEnable, intModemEnable, intChipEnable})));

    // Preamble
    static constexpr auto iPreambleTxLength = 0x00_b;
    // Send 0 bytes preamble
    static constexpr auto preambleTxLength = 0x00_b;
    // Normal sync timeout, 20-bit preamble RX threshold
    static constexpr auto preambleConfigStd1 = 0x14_b;
    // No non-standard preamble pattern
    static constexpr auto preambleConfigNstd = 0x00_b;
    // No extended RX preamble timeout, 0x0F nibbles timeout until detected preamble is discarded as
    // invalid
    static constexpr auto preambleConfigStd2 = 0x0F_b;
    // RX Standard preamble, first received preamble bit is 0, unit of preamble TX length is in
    // bytes, use standard preamble 0101 pattern
    static constexpr auto preambleConfig = 0b0001'0010_b;
    // Non-standard pattern
    static constexpr auto preamblePattern = std::array<Byte, 4>{};
    OUTCOME_TRY(SetProperties(PropertyGroup::preamble,
                              iPreambleTxLength,
                              Span(FlatArray(preambleTxLength,
                                             preambleConfigStd1,
                                             preambleConfigNstd,
                                             preambleConfigStd2,
                                             preambleConfig,
                                             preamblePattern))));

    // Sync word
    static constexpr auto iSyncConfig = 0x00_b;
    // Do not transmit sync word, allow 4-bit sync word errors on receive, 4-byte sync word length
    static constexpr auto syncConfig = 0b1100'0011_b;
    // Valid CCSDS TM sync word for Reed-Solomon or convolutional coding. Be careful: Send order is
    // MSB-first but little endian so the lowest bit of the highest byte is transmitted first, which
    // is different to how the CCSDS spec annotates those bit patterns!
    //
    // This sync word is in theory not correct for uplink - but as is Reed Solomon. We are using the
    // same configuration for up- and downlink just without convolutional coding for uplink.
    //
    // TODO: Check that pattern!
    //
    // TODO: The application note says the exact opposite: LSB first, big endian. Also, instead of
    // specifying the 4 bytes manually we should, serialize a 32-bit number.
    static constexpr auto syncBits =
        std::array{0b0101'1000_b, 0b1111'0011_b, 0b0011'1111_b, 0b1011'1000_b};
    OUTCOME_TRY(
        SetProperties(PropertyGroup::sync, iSyncConfig, Span(FlatArray(syncConfig, syncBits))));

    // CRC
    static constexpr auto iPktCrcConfig = 0x00_b;
    // No CRC
    static constexpr auto pktCrcConfig = 0x00_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::pkt, iPktCrcConfig, Span({pktCrcConfig})));

    // Packet Whitening and Config
    static constexpr auto iPktWhiteConfig = 0x05_b;
    // No Whitening
    static constexpr auto pktWhiteConfig = 0x00_b;
    // Don't split RX and TX field information (length, ...), enable RX packet handler, use normal
    // (2)FSK, no Manchester coding, no CRC, data transmission with MSB first.
    static constexpr auto pktConfig1 = 0x00_b;
    OUTCOME_TRY(
        SetProperties(PropertyGroup::pkt, iPktWhiteConfig, Span({pktWhiteConfig, pktConfig1})));

    // Packet length
    static constexpr auto iPktLen = 0x08_b;
    // Infinite receive, big endian (MSB first)
    static constexpr auto pktLen = 0x60_b;
    static constexpr auto pktLenFieldSource = 0x00_b;
    static constexpr auto pktLenAdjust = 0x00_b;
    static constexpr auto pktTxThreshold = static_cast<Byte>(txFifoThreshold);
    static constexpr auto pktRxThreshold = static_cast<Byte>(rxFifoThreshold);
    static constexpr auto pktField1Length = std::array{0x00_b, 0x01_b};
    static constexpr auto pktField1Config = 0x00_b;
    static constexpr auto pktField1CrcConfig = 0x00_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::pkt,
                              iPktLen,
                              Span(FlatArray(pktLen,
                                             pktLenFieldSource,
                                             pktLenAdjust,
                                             pktTxThreshold,
                                             pktRxThreshold,
                                             pktField1Length,
                                             pktField1Config,
                                             pktField1CrcConfig))));

    OUTCOME_TRY(SetConstantModemProperties());

    // RX filter coefficients
    // Block 1
    static constexpr auto iRxFilterCoefficientsBlock1 = 0x00_b;
    static constexpr auto rxFilterCoefficientsBlock1 = std::array{
        0xFF_b,  // RX1_CHFLT_COE13[7:0]
        0xC4_b,  // RX1_CHFLT_COE12[7:0]
        0x30_b,  // RX1_CHFLT_COE11[7:0]
        0x7F_b,  // RX1_CHFLT_COE10[7:0]
        0xF5_b,  // RX1_CHFLT_COE9[7:0]
        0xB5_b,  // RX1_CHFLT_COE8[7:0]
        0xB8_b,  // RX1_CHFLT_COE7[7:0]
        0xDE_b,  // RX1_CHFLT_COE6[7:0]
        0x05_b,  // RX1_CHFLT_COE5[7:0]
        0x17_b,  // RX1_CHFLT_COE4[7:0]
        0x16_b,  // RX1_CHFLT_COE3[7:0]
        0x0C_b   // RX1_CHFLT_COE2[7:0]
    };
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modemChflt, iRxFilterCoefficientsBlock1, Span(rxFilterCoefficientsBlock1)));

    // Block 2
    static constexpr auto iRxFilterCoefficientsBlock2 = 0x0C_b;
    static constexpr auto rxFilterCoefficientsBlock2 = std::array{
        0x03_b,  // RX1_CHFLT_COE1[7:0]
        0x00_b,  // RX1_CHFLT_COE0[7:0]
        0x15_b,  // RX1_CHFLT_COE10[9:8]  | RX1_CHFLT_COE11[9:8]  | RX1_CHFLT_COE12[9:8]  |
                 // RX1_CHFLT_COE13[9:8]
        0xFF_b,  // RX1_CHFLT_COE6[9:8]   | RX1_CHFLT_COE7[9:8]   | RX1_CHFLT_COE8[9:8]   |
                 // RX1_CHFLT_COE9[9:8]
        0x00_b,  // RX1_CHFLT_COE2[9:8]   | RX1_CHFLT_COE3[9:8]   | RX1_CHFLT_COE4[9:8]   |
                 // RX1_CHFLT_COE5[9:8]
        0x00_b,  // 0 | 0 | 0 | 0         | RX1_CHFLT_COE0[9:8]   | RX1_CHFLT_COE1[9:8]
        0xFF_b,  // RX2_CHFLT_COE13[7:0]
        0xC4_b,  // RX2_CHFLT_COE12[7:0]
        0x30_b,  // RX2_CHFLT_COE11[7:0]
        0x7F_b,  // RX2_CHFLT_COE10[7:0]
        0xF5_b,  // RX2_CHFLT_COE9[7:0]
        0xB5_b   // RX2_CHFLT_COE8[7:0]
    };
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modemChflt, iRxFilterCoefficientsBlock2, Span(rxFilterCoefficientsBlock2)));

    // Block 3
    static constexpr auto iRxFilterCoefficientsBlock3 = 0x18_b;
    static constexpr auto rxFilterCoefficientsBlock3 = std::array{
        0xB8_b,  // RX2_CHFLT_COE7[7:0]
        0xDE_b,  // RX2_CHFLT_COE6[7:0]
        0x05_b,  // RX2_CHFLT_COE5[7:0]
        0x17_b,  // RX2_CHFLT_COE4[7:0]
        0x16_b,  // RX2_CHFLT_COE3[7:0]
        0x0C_b,  // RX2_CHFLT_COE2[7:0]
        0x03_b,  // RX2_CHFLT_COE1[7:0]
        0x00_b,  // RX2_CHFLT_COE0[7:0]
        0x15_b,  // RX2_CHFLT_COE10[9:8]  | RX2_CHFLT_COE11[9:8]  | RX2_CHFLT_COE12[9:8] |
                 // RX2_CHFLT_COE13[9:8]
        0xFF_b,  // RX2_CHFLT_COE6[9:8]   | RX2_CHFLT_COE7[9:8]   | RX2_CHFLT_COE8[9:8]  |
                 // RX2_CHFLT_COE9[9:8]
        0x00_b,  // RX2_CHFLT_COE2[9:8]   | RX2_CHFLT_COE3[9:8]   | RX2_CHFLT_COE4[9:8]  |
                 // RX2_CHFLT_COE5[9:8]
        0x00_b   // 0 | 0 | 0 | 0         | RX2_CHFLT_COE0[9:8]   | RX2_CHFLT_COE1[9:8]  |
    };
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modemChflt, iRxFilterCoefficientsBlock3, Span(rxFilterCoefficientsBlock3)));

    // RF PA mode
    static constexpr auto iPaMode = 0x00_b;
    // PA switching amp mode, PA_SEL = HP_COARSE, disable power sequencing, disable external TX ramp
    // signal
    static constexpr auto paMode = 0x08_b;
    // Enabled PA fingers (sets output power but not linearly; 10 µA bias current per enabled
    // finger, complementary drive signal with 50 % duty cycle)
    static constexpr auto paPwrLvl = 0x2f_b;
    static constexpr auto paBiasClkduty = 0x00_b;
    // Ramping time constant = 0x1F (~56 µs to full - 0.5 dB), FSK modulation delay 30 µs
    static constexpr auto paTc = 0xFF_b;
    OUTCOME_TRY(
        SetProperties(PropertyGroup::pa, iPaMode, Span({paMode, paPwrLvl, paBiasClkduty, paTc})));

    // RF synth feed forward charge pump current, integrated charge pump current, VCO gain scaling
    // factor, FF loop filter values
    static constexpr auto iSynthPfdcpCpff = 0x00_b;
    // FF charge pump current = 60 µA
    static constexpr auto synthPfdcpCpff = 0x2C_b;
    // SYNTH_PFDCP_CPINT: Int charge pump current = 30 µA
    static constexpr auto synthPfdcpCpint = 0x0E_b;
    // SYNTH_VCO_KV: set VCO scaling factor to maximum value, set tuning varactor gain to maximum
    // value
    static constexpr auto synthVcoKv = 0x0B_b;
    // SYNTH_LPFILT3: R2 value 90 kOhm
    static constexpr auto synthLpfilt3 = 0x04_b;
    // SYNTH_LPFILT2: C2 value 11.25 pF
    static constexpr auto synthLpfilt2 = 0x0C_b;
    // SYNTH_LPFILT1: C3 value 12 pF, C1 offset 0 pF, C1 value 7.21 pF
    static constexpr auto synthLpfilt1 = 0x73_b;
    // SYNTH_LPFILT0: FF amp bias current 100 µA
    static constexpr auto synthLpfilt0 = 0x03_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::synth,
                              iSynthPfdcpCpff,
                              Span({synthPfdcpCpff,
                                    synthPfdcpCpint,
                                    synthVcoKv,
                                    synthLpfilt3,
                                    synthLpfilt2,
                                    synthLpfilt1,
                                    synthLpfilt0})));

    // RF match mask
    static constexpr auto iMatchValue1 = 0x00_b;
    static constexpr auto matchValue1 = 0x00_b;
    static constexpr auto matchMask1 = 0x00_b;
    static constexpr auto matchCtrl1 = 0x00_b;
    static constexpr auto matchValue2 = 0x00_b;
    static constexpr auto matchMask2 = 0x00_b;
    static constexpr auto matchCtrl2 = 0x00_b;
    static constexpr auto matchValue3 = 0x00_b;
    static constexpr auto matchMask3 = 0x00_b;
    static constexpr auto matchCtrl3 = 0x00_b;
    static constexpr auto matchValue4 = 0x00_b;
    static constexpr auto matchMask4 = 0x00_b;
    static constexpr auto matchCtrl4 = 0x00_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::match,
                              iMatchValue1,
                              Span({matchValue1,
                                    matchMask1,
                                    matchCtrl1,
                                    matchValue2,
                                    matchMask2,
                                    matchCtrl2,
                                    matchValue3,
                                    matchMask3,
                                    matchCtrl3,
                                    matchValue4,
                                    matchMask4,
                                    matchCtrl4})));

    // Frequency control
    static constexpr auto iFreqControlInte = 0x00_b;
    // FC_inte 0x41 = 533.5, 0x41 = 434.5, 0x42 = 437.395
    static constexpr auto freqControlInte = 0x42_b;
    // FC_frac. 0xD89D9 = 433.5, 0xEC4EC = 434.5, 0xA5512 = 437.395, N_presc = 2, outdiv = 8, F_xo =
    // 26 MHz, RF_channel_Hz = (FC_inte + FC_frac / 2^19) * ((N_presc * F_xo) / outdiv)
    static constexpr auto freqControlFrac = std::array{0x0A_b, 0x55_b, 0x12_b};
    // Channel step size = 0x4EC5
    static constexpr auto freqControlChannelStepSize = std::array{0x4E_b, 0xC5_b};
    // Window gating period (in number of crystal clock cycles) = 32
    static constexpr auto freqControlWSize = 0x20_b;
    // Adjust target mode for VCO calibration in RX mode = 0xFE int8_t
    static constexpr auto freqControlVcontRxAdj = 0xFE_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::freqControl,
                              iFreqControlInte,
                              Span(FlatArray(freqControlInte,
                                             freqControlFrac,
                                             freqControlChannelStepSize,
                                             freqControlWSize,
                                             freqControlVcontRxAdj))));

    // Frequency adjust (stolen from Arduino demo code)
    // static constexpr auto globalXoTuneUpdated = 0x62_b;
    // SetProperties(PropertyGroup::global, iGlobalXoTune, Span({globalXoTuneUpdated}));

    // Change sequencer mode to guaranteed
    //
    // TODO: Why?
    //
    // Split FIFO and guaranteed sequencer mode
    // static constexpr auto newGlobalConfig = 0x40_b;
    // SetProperties(PropertyGroup::global, iGlobalConfig, Span({newGlobalConfig}));
    return outcome_v2::success();
}


// Set modem properties that don't change for different data rates
// NOLINTNEXTLINE(*cognitive-complexity)
auto SetConstantModemProperties() -> Result<void>
{
    // Values acquired by comparing 9 WDS data rate configurations. For some properties only parts
    // are here. The changing parts will be set per data rate.
    //
    // NOLINTBEGIN(*magic-numbers)
    //
    // MODEM_MOD_TYPE, MODEM_MAP_CONTROL, MODEM_DSM_CTRL
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x00_b, std::array{0x03_b, 0x00_b, 0x07_b}));
    // MODEM_TX_NCO_MODE, MODEM_FREQ_DEV (only LSB - other two bytes are non-constant)
    OUTCOME_TRY(
        SetProperties(PropertyGroup::modem, 0x07_b, std::array{0x8C_b, 0xBA_b, 0x80_b, 0x00_b}));
    // MODEM_TX_RAMP_DELAY, MODEM_MDM_CTRL, MODEM_IF_CONTROL, MODEM_IF_FREQ
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modem, 0x18_b, std::array{0x01_b, 0x00_b, 0x08_b, 0x03_b, 0x80_b, 0x00_b}));
    // MODEM_IFPKD_THRESHOLDS, MODEM_BCR_OSR (only LSB - other byte is non-constant)
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x21_b, std::array{0xE8_b, 0x00_b}));
    // MODEM_BCR_GEAR, MODEM_BCR_MISC1, MODEM_BCR_MISC0, MODEM_AFC_GEAR
    OUTCOME_TRY(
        SetProperties(PropertyGroup::modem, 0x29_b, std::array{0x02_b, 0x00_b, 0x00_b, 0x00_b}));
    // MODEM_AFC_MISC
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x32_b, std::array{0xA0_b}));
    // MODEM_AGC_CONTROL
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x35_b, std::array{0xE0_b}));
    // MODEM_AGC_WINDOW_SIZE
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x38_b, std::array{0x11_b}));
    // MODEM_FSK4_GAIN1, MODEM_FSK4_GAIN0, MODEM_FSK4_TH, MODEM_FSK4_MAP
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modem, 0x3b_b, std::array{0x80_b, 0x1A_b, 0x40_b, 0x00_b, 0x00_b}));
    // MODEM_OOK_BLOPK, MODEM_OOK_CNT1, MODEM_OOK_MISC
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x41_b, std::array{0x0C_b, 0xA4_b, 0x23_b}));
    // MODEM_RAW_CONTROL
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x45_b, std::array{0x03_b}));
    // MODEM_ANT_DIV_MODE, MODEM_ANT_DIV_CONTROL, MODEM_RSSI_THRESH, MODEM_RSSI_JUMP_THRESH,
    // MODEM_RSSI_CONTROL, MODEM_RSSI_CONTROL2, MODEM_RSSI_COMP
    OUTCOME_TRY(SetProperties(PropertyGroup::modem,
                              0x48_b,
                              std::array{0x01_b, 0x00_b, 0xFF_b, 0x06_b, 0x00_b, 0x18_b, 0x40_b}));
    // MODEM_RAW_SEARCH2
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x50_b, std::array{0x84_b, 0x0A_b}));
    // MODEM_ONE_SHOT_AFC
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x55_b, std::array{0x07_b}));
    // MODEM_RSSI_MUTE
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x57_b, std::array{0x00_b}));
    // MODEM_DSA_CTRL1, MODEM_DSA_CTRL2
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x5b_b, std::array{0x40_b, 0x04_b}));
    // MODEM_DSA_RSSI, MODEM_DSA_MISC
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, 0x5e_b, std::array{0x78_b, 0x20_b}));
    // MODEM_CHFLT_RX1_CHFLT_COE
    OUTCOME_TRY(SetProperties(PropertyGroup::modemChflt, 0x0e_b, std::array{0x15_b}));
    // MODEM_CHFLT_RX2_CHFLT_COE
    return SetProperties(PropertyGroup::modemChflt, 0x20_b, std::array{0x15_b});
    // NOLINTEND(*magic-numbers)
}


auto GetDataRateConfig(std::uint32_t dataRate) -> DataRateConfig
{
    if(dataRate > ((dataRateConfig115200.dataRate + dataRateConfig76800.dataRate) / 2))
    {
        return dataRateConfig115200;
    }
    if(dataRate > ((dataRateConfig76800.dataRate + dataRateConfig57600.dataRate) / 2))
    {
        return dataRateConfig76800;
    }
    if(dataRate > ((dataRateConfig57600.dataRate + dataRateConfig38400.dataRate) / 2))
    {
        return dataRateConfig57600;
    }
    if(dataRate > ((dataRateConfig38400.dataRate + dataRateConfig19200.dataRate) / 2))
    {
        return dataRateConfig38400;
    }
    if(dataRate > ((dataRateConfig19200.dataRate + dataRateConfig9600.dataRate) / 2))
    {
        return dataRateConfig19200;
    }
    if(dataRate > ((dataRateConfig9600.dataRate + dataRateConfig4800.dataRate) / 2))
    {
        return dataRateConfig9600;
    }
    if(dataRate > ((dataRateConfig4800.dataRate + dataRateConfig2400.dataRate) / 2))
    {
        return dataRateConfig4800;
    }
    if(dataRate > ((dataRateConfig2400.dataRate + dataRateConfig1200.dataRate) / 2))
    {
        return dataRateConfig2400;
    }
    return dataRateConfig1200;
}


// Properties for RX & TX are the same for a given DataRate
// NOLINTNEXTLINE(*cognitive-complexity)
auto SetDataRate(DataRateConfig const & dataRateConfig) -> Result<void>
{
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_DATA_RATE));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_FREQ_DEV));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_DECIMATION_CFG1));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_BCR_OSR));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_AFC_WAIT));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_AGC_RFPD_DECAY));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_OOK_PDTC));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_RAW_EYE));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_SPIKE_DET));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_DSA_QUAL));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_CHFLT_RX1_CHFLT_COE));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_CHFLT_RX1_CHFLT_COE_2));
    OUTCOME_TRY(SetProperties(dataRateConfig.MODEM_CHFLT_RX2_CHFLT_COE));
    return SetProperties(dataRateConfig.PREAMBLE_TX_LENGTH);
}


auto EnableRfLatchupProtection() -> void
{
#if 27 <= HW_VERSION and HW_VERSION < 30
    rfLatchupDisableGpioPin.Reset();
#endif
#if 30 <= HW_VERSION
    rfLatchupDisableGpioPin1.Reset();
    rfLatchupDisableGpioPin2.Reset();
#endif
}


auto DisableRfLatchupProtection() -> void
{
#if 27 <= HW_VERSION and HW_VERSION < 30
    rfLatchupDisableGpioPin.Set();
#endif
#if 30 <= HW_VERSION
    rfLatchupDisableGpioPin1.Set();
    rfLatchupDisableGpioPin2.Set();
#endif
}


auto SetRxFifoThreshold(Byte threshold) -> Result<void>
{
    static constexpr auto iFifoThreshold = 0x0c_b;
    return SetProperties(PropertyGroup::pkt, iFifoThreshold, Span({threshold}));
}


auto SetPacketHandlerInterrupts(Byte interruptFlags) -> Result<void>
{
    static constexpr auto iIntCtlPhEnable = 0x01_b;
    return SetProperties(PropertyGroup::intCtl, iIntCtlPhEnable, Span(interruptFlags));
}


// auto SetModemInterrupts(Byte interruptFlags) -> void
// {
//     static constexpr auto iIntCtlModemEnable = 0x02_b;
//     SetProperties(PropertyGroup::intCtl, iIntCtlModemEnable, Span(interruptFlags));
// }


auto ReadAndClearInterruptStatus() -> Result<std::array<Byte, interruptStatusAnswerLength>>
{
    return SendCommand<interruptStatusAnswerLength>(
        Span({cmdGetIntStatus, 0x00_b, 0x00_b, 0x00_b}));
}


// Do not return until the NIRQ pin is set or the reactivationTime is reached
auto SuspendUntilInterrupt(RodosTime reactivationTime) -> Result<void>
{
    nirqGpioPin.EnableInterrupts();
    nirqGpioPin.ResetInterruptStatus();
    auto result = [&]() -> Result<void>
    {
        while(nirqGpioPin.Read() == hal::PinState::set)
        {
            OUTCOME_TRY(nirqGpioPin.SuspendUntilInterrupt(reactivationTime));
        }
        return outcome_v2::success();
    }();
    nirqGpioPin.DisableInterrupts();
    return result;
}


// Do not return until the NIRQ pin is set or the timeout is reached
auto SuspendUntilInterrupt(Duration timeout) -> Result<void>
{
    auto reactivationTime = CurrentRodosTime() + timeout;
    return SuspendUntilInterrupt(reactivationTime);
}


auto StartTx() -> Result<void>
{
    static constexpr auto channel = 0x00_b;
    // [7:4]: TXCOMPLETE_STATE = 0b0011 -> READY state
    // [3]: UPDATE = 0b0 -> Use TX parameters to enter TX mode
    // [2]: RETRANSMIT = 0b0 -> Send from TX FIFO, not last packet
    // [1:0]: START = 0b0 -> Start TX immediately
    static constexpr auto condition = 0x30_b;
    // Length is set in SetTxDataLength(), not in StartTx()
    static constexpr auto txLen = std::array{0x00_b, 0x00_b};
    static constexpr auto txDelay = 0x00_b;
    static constexpr auto numRepeat = 0x00_b;
    OUTCOME_TRY(
        SendCommand(Span(FlatArray(cmdStartTx, channel, condition, txLen, txDelay, numRepeat))));
    isInTxMode = true;
    return outcome_v2::success();
}


auto StartRx() -> Result<void>
{
    static constexpr auto channel = 0x00_b;
    // [0]: START: 0b0 -> Start RX immediately
    static constexpr auto condition = 0x00_b;
    // RX_LEN = 0 -> Configuration of the packet handler fields is used
    static constexpr auto rxLen = std::array{0x00_b, 0x00_b};
    // [3:0] RXTIMEOUT_STATE = 0 -> Remain in RX state on preamble detection timeout
    static constexpr auto nextState1 = 0x00_b;
    // [3:0] RXVALID_STATE = 0 -> Remain in RX state if CRC check passes (we don't check CRC)
    static constexpr auto nextState2 = 0x00_b;
    // [3:0] RXINVALID_STATE = 0 -> Remain in RX state if CRC check fails (we don't check CRC)
    static constexpr auto nextState3 = 0x00_b;
    OUTCOME_TRY(SendCommand(Span(
        FlatArray(cmdStartRx, channel, condition, rxLen, nextState1, nextState2, nextState3))));
    isInTxMode = false;
    return outcome_v2::success();
}


auto ResetFifos() -> Result<void>
{
    static constexpr auto resetBothFifos = 0b11_b;
    return SendCommand(Span({cmdFifoInfo, resetBothFifos}));
}


auto WriteToFifo(std::span<Byte const> data) -> Result<void>
{
    if(data.empty())
    {
        return outcome_v2::success();
    }
    OUTCOME_TRY(BusyWaitForCts(ctsTimeout));
    SelectChip();
    WriteTo(&rfSpi, Span(cmdWriteTxFifo), spiTimeout);
    WriteTo(&rfSpi, data, spiTimeout);
    DeselectChip();
    return outcome_v2::success();
}


auto ReadFromFifo(std::span<Byte> data) -> void
{
    SelectChip();
    // auto buf = std::to_array<std::uint8_t>({0x77});
    // spi.write(std::data(buf), std::size(buf));
    WriteTo(&rfSpi, Span(cmdReadRxFifo), spiTimeout);
    // spi.read(data, length);
    ReadFrom(&rfSpi, data, spiTimeout);
    DeselectChip();
}


auto ReadFreeTxFifoSpace() -> Result<std::uint8_t>
{
    OUTCOME_TRY(auto fifoInfo, SendCommand<fifoInfoAnswerLength>(Span({cmdFifoInfo, 0x00_b})));
    return static_cast<std::uint8_t>(fifoInfo[1]);
}


auto ReadRxFifoFillLevel() -> Result<std::uint8_t>
{
    OUTCOME_TRY(auto fifoInfo, SendCommand<fifoInfoAnswerLength>(Span({cmdFifoInfo, 0x00_b})));
    return static_cast<std::uint8_t>(fifoInfo[0]);
}


auto DebugPrintModemStatus() -> void
{
#ifdef ENABLE_DEBUG_PRINT
    auto modemStatusResult = ReadModemStatus();
    if(modemStatusResult.has_error())
    {
        DEBUG_PRINT("Reading modem status failed: %s\n", ToCZString(modemStatusResult.error()));
    }
    else
    {
        auto modemStatus = modemStatusResult.value();
        // NOLINTBEGIN(*magic-numbers)
        DEBUG_PRINT(
            "Current RSSI: "
            "%4.1fdBm, Latched RSSI: %4.1fdBm, AFC Frequency Offset: %6d\n",
            (static_cast<double>(static_cast<int>(modemStatus[2])) / 2.0) - 70,  // Current RSSI
            (static_cast<double>(static_cast<int>(modemStatus[3])) / 2.0) - 70,  // Latched RSSI
            (static_cast<unsigned>(modemStatus[6]) << 8U)
                + static_cast<unsigned>(modemStatus[7]));  // AFC Offset
        // NOLINTEND(*magic-numbers)
    }
#endif
}


[[maybe_unused]] auto ReadModemStatus() -> Result<ModemStatus>
{
    return SendCommand<modemStatusAnswerLength>(Span(cmdGetModemStatus));
}


auto SendCommand(std::span<Byte const> data) -> Result<void>
{
    OUTCOME_TRY(SendCommand<0>(data));
    return outcome_v2::success();
}


template<std::size_t answerLength>
auto SendCommand(std::span<Byte const> data) -> Result<std::array<Byte, answerLength>>
{
    SelectChip();
    hal::WriteTo(&rfSpi, data, spiTimeout);
    DeselectChip();
    return BusyWaitForAnswer<answerLength>(ctsTimeout);
}


auto SelectChip() -> void
{
    static constexpr auto postChipSelectionDelay = 20 * ns;
    csGpioPin.Reset();
    BusyWaitFor(postChipSelectionDelay);
}


auto DeselectChip() -> void
{
    static constexpr auto preChipDeselectionDelay = 50 * ns;
    BusyWaitFor(preChipDeselectionDelay);
    csGpioPin.Set();
}


auto BusyWaitForCts(Duration timeout) -> Result<void>
{
    OUTCOME_TRY(BusyWaitForAnswer<0>(timeout));
    return outcome_v2::success();
}


template<std::size_t answerLength>
auto BusyWaitForAnswer(Duration timeout) -> Result<std::array<Byte, answerLength>>
{
    static constexpr auto dataIsReadyValue = 0xFF_b;
    auto nextPollingTime = CurrentRodosTime();
    auto deadline = nextPollingTime + timeout;
    while(true)
    {
        SelectChip();
        hal::WriteTo(&rfSpi, Span(cmdReadCmdBuff), spiTimeout);
        auto cts = 0x00_b;
        hal::ReadFrom(&rfSpi, Span(&cts), spiTimeout);
        if(cts == dataIsReadyValue)
        {
            break;
        }
        DeselectChip();
        nextPollingTime += pollingInterval;
        BusyWaitUntil(nextPollingTime);
        if(CurrentRodosTime() > deadline)
        {
            return ErrorCode::timeout;
        }
    }
    auto answer = std::array<Byte, answerLength>{};
    if constexpr(answerLength > 0)
    {
        hal::ReadFrom(&rfSpi, Span(&answer), spiTimeout);
    }
    DeselectChip();
    return answer;
}


template<std::size_t extent>
    requires(extent <= maxNProperties)
inline auto SetProperties(PropertyGroup propertyGroup,
                          Byte startIndex,
                          std::span<Byte const, extent> propertyValues) -> Result<void>
{
    return SendCommand(FlatArray(cmdSetProperty,
                                 static_cast<Byte>(propertyGroup),
                                 static_cast<Byte>(extent),
                                 startIndex,
                                 propertyValues));
}


template<std::size_t nProperties>
    requires(nProperties <= maxNProperties)
[[nodiscard]] auto SetProperties(PropertyGroup propertyGroup,
                                 Byte propertyStartIndex,
                                 std::array<Byte, nProperties> const & properties) -> Result<void>
{
    return SetProperties(propertyGroup, propertyStartIndex, Span(properties));
}


template<PropertyGroup propertyGroup, sts1cobcsw::Byte propertyStartIndex, std::size_t nProperties>
[[nodiscard]] auto SetProperties(
    Properties<propertyGroup, propertyStartIndex, nProperties> property) -> Result<void>
{
    return SetProperties(property.group, property.startIndex, Span(property.GetValues()));
}


template<std::size_t size>
    requires(size <= maxNProperties)
inline auto GetProperties(PropertyGroup propertyGroup, Byte startIndex)
    -> Result<std::array<Byte, size>>
{
    return SendCommand<size>(Span(
        {cmdGetProperty, static_cast<Byte>(propertyGroup), static_cast<Byte>(size), startIndex}));
}
}
}

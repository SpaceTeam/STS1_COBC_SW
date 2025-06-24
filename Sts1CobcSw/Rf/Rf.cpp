//! @file
//! @brief  Driver for the RF module Si4463.
//!
//! See "AN625: Si446x API Descriptions" for more information.


#include <Sts1CobcSw/Rf/Rf.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>
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
#include <cstdint>
#include <span>
#include <utility>


namespace sts1cobcsw::rf
{
namespace
{
enum class PropertyGroup : std::uint8_t
{
    global = 0x00,       //
    intCtl = 0x01,       // Interrupt control
    frrCtl = 0x02,       // Fast response register control
    preamble = 0x10,     //
    sync = 0x11,         // Sync word
    pkt = 0x12,          // Packet
    modem = 0x20,        //
    modemChflt = 0x21,   //
    pa = 0x22,           // Power amplifier
    synth = 0x23,        //
    match = 0x30,        //
    freqControl = 0x40,  //
    rxHop = 0x50,        //
    pti = 0xF0           // Packet trace interface
};


// --- Private globals ---

// Commands
[[maybe_unused]] constexpr auto cmdPartInfo = 0x01_b;
[[maybe_unused]] constexpr auto cmdPowerUp = 0x02_b;
[[maybe_unused]] constexpr auto cmdFuncInfo = 0x11_b;
[[maybe_unused]] constexpr auto cmdSetProperty = 0x11_b;
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
// Max. number of properties that can be set in a single command
constexpr auto maxNProperties = 12;

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

constexpr auto porCircuitSettleDelay = 100 * ms;  // Time for PoR circuit to settles after power up
constexpr auto porRunningDelay = 20 * ms;         // Time for power on reset to finish

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

constexpr auto errorHandlingRetryDelay = 1 * ms;
constexpr auto errorHandlingCobcResetDelay = 1 * s;
auto usedTxType = TxType{};


using ModemStatus = std::array<Byte, modemStatusAnswerLength>;


// --- Private function declarations ---
template<auto doFunction, typename... Args>
    requires std::invocable<decltype(doFunction), Args...>
auto ExecuteWithRecovery(Args &... args) -> decltype(auto);

[[nodiscard]] auto DoInitialize(TxType txType) -> Result<void>;
[[nodiscard]] auto DoReadPartNumber() -> Result<std::uint16_t>;
[[nodiscard]] auto DoEnterStandbyMode() -> Result<void>;
[[nodiscard]] auto DoSetTxType(TxType txType) -> Result<void>;
[[nodiscard]] auto DoSetTxDataLength(std::uint16_t length) -> Result<void>;
[[nodiscard]] auto DoSetTxDataRate(std::uint32_t dataRate) -> Result<void>;
[[nodiscard]] auto DoSetRxDataRate(std::uint32_t dataRate) -> Result<void>;
[[nodiscard]] auto DoGetTxDataRate() -> Result<std::uint32_t>;
[[nodiscard]] auto DoGetRxDataRate() -> Result<std::uint32_t>;
[[nodiscard]] auto DoSendAndWait(std::span<Byte const> data) -> Result<void>;
[[nodiscard]] auto DoSendAndContinue(std::span<Byte const> data) -> Result<void>;
[[nodiscard]] auto DoSuspendUntilDataSent(Duration timeout) -> Result<void>;
[[nodiscard]] auto DoReceive(std::span<Byte> data, Duration timeout) -> Result<void>;

auto InitializeGpiosAndSpi() -> void;
[[nodiscard]] auto ApplyPatch() -> Result<void>;
[[nodiscard]] auto PowerUp() -> Result<void>;
[[nodiscard]] auto Configure(TxType txType) -> Result<void>;

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


template<auto doFunction, typename... Args>
    requires std::invocable<decltype(doFunction), Args...>
auto ExecuteWithRecovery(Args &... args) -> decltype(auto)
{
    // First try
    auto result = doFunction(args...);
    if(not result.has_error())
    {
        return result;
    }
    SuspendFor(errorHandlingRetryDelay);
    // Second try
    result = doFunction(args...);
    if(not result.has_error())
    {
        return result;
    }
    auto reinitResult = DoInitialize(usedTxType);
    if(reinitResult.has_error())
    {
        // TODO: handle error
    }
    // Third try
    result = doFunction(args...);
    if(not result.has_error())
    {
        return result;
    }
    // Reset cobc
    SuspendFor(errorHandlingCobcResetDelay);
    RODOS::hwResetAndReboot();
    return result;
}
}


// --- Public function definitions ---

auto Initialize(TxType txType) -> void
{
    usedTxType = txType;
    (void)ExecuteWithRecovery<DoInitialize>(txType);
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
    auto answer = ExecuteWithRecovery<DoReadPartNumber>();
    if(answer.has_error())
    {
        return {};
    }
    return answer.value();
}


auto EnterStandbyMode() -> void
{
    (void)ExecuteWithRecovery<DoEnterStandbyMode>();
}


auto SetTxType(TxType txType) -> void
{
    (void)ExecuteWithRecovery<DoSetTxType>(txType);
}


// Must be called before SendAndContinue()
auto SetTxDataLength(std::uint16_t length) -> void
{
    (void)ExecuteWithRecovery<DoSetTxDataLength>(length);
}


auto SetTxDataRate(std::uint32_t dataRate) -> void
{
    (void)ExecuteWithRecovery<DoSetTxDataRate>(dataRate);
}


auto SetRxDataRate(std::uint32_t dataRate) -> void
{
    (void)ExecuteWithRecovery<DoSetRxDataRate>(dataRate);
}


auto GetTxDataRate() -> std::uint32_t
{
    auto result = ExecuteWithRecovery<DoGetTxDataRate>();
    if(result.has_error())
    {
        return 0;
    }
    return result.value();
}


auto GetRxDataRate() -> std::uint32_t
{
    auto result = ExecuteWithRecovery<DoGetRxDataRate>();
    if(result.has_error())
    {
        return 0;
    }
    return result.value();
}


auto SendAndWait(std::span<Byte const> data) -> Result<void>
{
    return ExecuteWithRecovery<DoSendAndWait>(data);
}


// Send the data to the RF module and return as soon as the last chunk was written to the FIFO. This
// allows sending multiple data packets without interruption.
auto SendAndContinue(std::span<Byte const> data) -> Result<void>
{
    return ExecuteWithRecovery<DoSendAndContinue>(data);
}


auto SuspendUntilDataSent(Duration timeout) -> Result<void>
{
    return ExecuteWithRecovery<DoSuspendUntilDataSent>(timeout);
}


auto Receive(std::span<Byte> data, Duration timeout) -> Result<void>
{
    return ExecuteWithRecovery<DoReceive>(data, timeout);
}


// --- Private function definitions ---

namespace
{
auto DoInitialize(TxType txType) -> Result<void>
{
    InitializeGpiosAndSpi();
    OUTCOME_TRY(ApplyPatch());
    OUTCOME_TRY(PowerUp());
    OUTCOME_TRY(Configure(txType));
    persistentVariables.Load<"txIsOn">() ? EnableTx() : DisableTx();
    return outcome_v2::success();
}


auto DoReadPartNumber() -> Result<std::uint16_t>
{
    OUTCOME_TRY(auto answer, SendCommand<partInfoAnswerLength>(Span(cmdPartInfo)));
    return Deserialize<std::endian::big, std::uint16_t>(Span(answer).subspan<1, 2>());
}


auto DoEnterStandbyMode() -> Result<void>
{
    static constexpr auto standbyMode = 0x01_b;
    OUTCOME_TRY(SendCommand(Span({cmdChangeState, standbyMode})));
    isInTxMode = false;
    EnableRfLatchupProtection();
    return outcome_v2::success();
}


auto DoSetTxType(TxType txType) -> Result<void>
{
    // Constants for setting the TX type (morse, 2GFSK)
    // MODEM_DATA_RATE: unused, 20 kBaud
    static constexpr std::uint32_t dataRateMorse = 20'000U * 40U;
    // MODEM_DATA_RATE: For 9k6 Baud: (TX_DATA_RATE * MODEM_TX_NCO_MODE * TXOSR) / F_XTAL_Hz = (9600
    // * 26'000'000 * 40) / 26'000'000 = 9600 * 40
    static constexpr std::uint32_t dataRate2Gfsk = 9600U * 40U;
    // MODEM_MODE_TYPE: TX data from GPIO0 pin, modulation OOK
    static constexpr auto modemModeTypeMorse = 0x09_b;
    // MODEM_MODE_TYPE: TX data from packet handler, modulation 2GFSK
    static constexpr auto modemModeType2Gfsk = 0x03_b;
    // Inconsistent naming pattern due to strict adherence to datasheet
    static constexpr auto modemMapControl = 0x00_b;
    static constexpr auto modemDsmCtrl = 0x07_b;
    static constexpr auto startIndex = 0x00_b;
    auto modemModeType = (txType == TxType::morse ? modemModeTypeMorse : modemModeType2Gfsk);
    auto dataRate = (txType == TxType::morse ? dataRateMorse : dataRate2Gfsk);
    return SetProperties(
        PropertyGroup::modem,
        startIndex,
        Span(FlatArray(modemModeType,
                       modemMapControl,
                       modemDsmCtrl,
                       // The data rate property is only 3 bytes wide, so drop the first byte
                       Span(Serialize<std::endian::big>(dataRate)).subspan<1>())));
}


auto DoSetTxDataLength(std::uint16_t length) -> Result<void>
{
    static constexpr auto iPktField1Length = 0x0D_b;
    return SetProperties(
        PropertyGroup::pkt, iPktField1Length, Span(Serialize<std::endian::big>(length)));
}


auto DoSetTxDataRate(std::uint32_t dataRate) -> Result<void>
{
    // TODO: Implement this
    (void)dataRate;
    return outcome_v2::success();
}


auto DoSetRxDataRate(std::uint32_t dataRate) -> Result<void>
{
    // TODO: Implement this
    (void)dataRate;
    return outcome_v2::success();
}


auto DoGetTxDataRate() -> Result<std::uint32_t>
{
    // TODO: Implement this
    return 0;
}


auto DoGetRxDataRate() -> Result<std::uint32_t>
{
    // TODO: Implement this
    return 0;
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
    if(not isInTxMode)
    {
        OUTCOME_TRY(ResetFifos());
        DisableRfLatchupProtection();
    }
    auto dataIndex = 0U;
    auto result = [&]() -> Result<void>
    {
        OUTCOME_TRY(SetPacketHandlerInterrupts(txFifoAlmostEmptyInterrupt));
        OUTCOME_TRY(auto chunkSize, ReadFreeTxFifoSpace());
        while(dataIndex + chunkSize < static_cast<unsigned int>(data.size()))
        {
            OUTCOME_TRY(WriteToFifo(data.subspan(dataIndex, chunkSize)));
            OUTCOME_TRY(ReadAndClearInterruptStatus());
            if(not isInTxMode)
            {
                OUTCOME_TRY(StartTx());
            }
            dataIndex += chunkSize;
            OUTCOME_TRY(SuspendUntilInterrupt(interruptTimeout));
            OUTCOME_TRY(chunkSize, ReadFreeTxFifoSpace());
        }
        return outcome_v2::success();
    }();
    OUTCOME_TRY(SetPacketHandlerInterrupts(noInterrupts));
    if(result.has_error())
    {
        return result;
    }
    OUTCOME_TRY(WriteToFifo(data.subspan(dataIndex)));
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
    OUTCOME_TRY(SetPacketHandlerInterrupts(noInterrupts));
    return result;
}


// I don't care too much about the high cognitive complexity here for the same reason as in
// DoSendAndContinue().
//
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto DoReceive(std::span<Byte> data, Duration timeout) -> Result<void>
{
    auto result = [&]() -> Result<void>
    {
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
            OUTCOME_TRY(SuspendUntilInterrupt(reactivationTime));
            DebugPrintModemStatus();
            ReadFromFifo(data.subspan(dataIndex, rxFifoThreshold));
            OUTCOME_TRY(ReadAndClearInterruptStatus());
            dataIndex += rxFifoThreshold;
        }
        auto remainingData = data.subspan(dataIndex);
        OUTCOME_TRY(SetRxFifoThreshold(static_cast<Byte>(remainingData.size())));
        OUTCOME_TRY(auto fillLevel, ReadRxFifoFillLevel());
        if(fillLevel < remainingData.size())
        {
            OUTCOME_TRY(SuspendUntilInterrupt(reactivationTime));
        }
        ReadFromFifo(remainingData);
        OUTCOME_TRY(SetRxFifoThreshold(static_cast<Byte>(rxFifoThreshold)));
        return outcome_v2::success();
    }();
    auto setInterruptsResult = SetPacketHandlerInterrupts(noInterrupts);
    OUTCOME_TRY(DoEnterStandbyMode());
    if(setInterruptsResult.has_error())
    {
        return setInterruptsResult;
    }
    OUTCOME_TRY(ReadAndClearInterruptStatus());
    return result;
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
    // The watchdog must be fed regularely for the TX to work. Even without the watchdog timer on
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
        cmdPowerUp, bootOptions, xtalOptions, Serialize<std::endian::big, std::uint32_t>(xoFreq)));
}


// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto Configure(TxType txType) -> Result<void>
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

    // RF modem mod type
    OUTCOME_TRY(DoSetTxType(txType));
    // SetTxType sets modem properties from 0x00 to 0x05
    static constexpr auto iModemTxNcoMode = 0x06_b;
    // TXOSR = x40 = 0, NCOMOD = F_XTAL = 26'000'000 = 0x018CBA80
    static constexpr auto modemTxNcoMode = std::array{0x05_b, 0x8C_b, 0xBA_b, 0x80_b};
    // We use minimum shift keying, i.e., a frequency deviation of baudrate / 4. The value we need
    // to write to the property is (2^19 * outdiv * deviation_Hz) / (N_presc * F_xo) = (2^19 * 8 *
    // (9600 / 4)) / (2 * 26000000) = 194 = 0x0000C2
    static constexpr auto modemFreqDeviation = std::array{0x00_b, 0x00_b, 0xC2_b};
    OUTCOME_TRY(SetProperties(PropertyGroup::modem,
                              iModemTxNcoMode,
                              Span(FlatArray(modemTxNcoMode, modemFreqDeviation))));

    // RF modem TX ramp delay, modem MDM control, modem IF control, modem IF frequency & modem
    // decimation
    static constexpr auto iModemTxRampDelay = 0x18_b;
    // TX amplifier ramp down delay after TX: 7 (max)
    static constexpr auto modemTxRampDelay = 0x07_b;
    // Slicer phase source from phase computer output
    static constexpr auto modemMdmCtrl = 0x00_b;
    // No ETSI mode, fixed IF mode, normal IF mode (nonzero IF)
    static constexpr auto modemIfControl = 0x08_b;
    // -(2^19 * 8 * (26000000/64))/(2*26000000) = -32768 = 0x038000 (two's complement!)
    static constexpr auto modemIfFreq = std::array{0x03_b, 0x80_b, 0x00_b};
    // Decimation NDEC0 = decimation by 1, NDEC1 = decimation by 8, NDEC2 = decimation by 1
    static constexpr auto modemDecimationCfg1 = 0x30_b;
    // Normal decimate-by-8 filter gain, don't bypass the decimate-by-2 polyphase filter, bypass the
    // decimate-by-3 polyphase filter, enable droop compensation, channel selection filter in normal
    // mode (27 tap filter)
    static constexpr auto modemDecimationCfg0 = 0x20_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::modem,
                              iModemTxRampDelay,
                              Span(FlatArray(modemTxRampDelay,
                                             modemMdmCtrl,
                                             modemIfControl,
                                             modemIfFreq,
                                             modemDecimationCfg1,
                                             modemDecimationCfg0))));

    // RF modem BCR oversampling rate, modem BCR NCO offset, modem BCR gain, modem BCR gear & modem
    // BCR misc
    //
    // TODO: What values to use here?
    static constexpr auto iModemBcrOsr = 0x22_b;
    // RX symbol oversampling rate of 0xA9 / 8 = 169 / 8 = 21.125 (According to the datasheet usual
    // values are in the range of 8 to 12 where this value seems to be odd?)
    static constexpr auto modemBcrOsr = std::array{0x00_b, 0xA9_b};
    // BCR NCO offset of 0x030655 / 64 = ~3097.33
    static constexpr auto modemBcrNcoOffset = std::array{0x03_b, 0x06_b, 0x55_b};
    // BCR gain 0x060F = 1551
    static constexpr auto modemBcrGain = std::array{0x06_b, 0x0F_b};
    // BCR loop gear control, CRSLOW=2, CRFAST=0
    static constexpr auto modemBcrGear = 0x02_b;
    static constexpr auto modemBcrMisc1 = 0x00_b;
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modem,
        iModemBcrOsr,
        Span(
            FlatArray(modemBcrOsr, modemBcrNcoOffset, modemBcrGain, modemBcrGear, modemBcrMisc1))));

    // RF modem AFC gear, modem AFC wait, modem AFC gain, modem AFC limiter & modem AFC misc
    //
    // TODO: What values to use here?
    static constexpr auto iModemAfcGear = 0x2C_b;
    // AFC_SLOW gain 0, AFC_FAST gain 0, switch gear after detection of preamble
    static constexpr auto modemAfcGear = 0x00_b;
    // LGWAIT = 2, SHWAIT = 1
    static constexpr auto modemAfcWait = 0x12_b;
    // AFC loop gain = 97, don't half the loop gain, disable adaptive RX bandwidth, enable
    // frequency error estimation
    static constexpr auto modemAfcGain = std::array{0x80_b, 0x61_b};
    static constexpr auto modemAfcLimiter = std::array{0x04_b, 0x11_b};
    // - Expected frequency error is less then 12 * symbol rate
    // - AFC correction of PLL will be frozen if a consecutive string of 1 s or 0 s that exceed the
    //   search period is encountered
    // - Don't switch clock source for frequency estimator
    // - Don't freeze AFC at preamble end
    // - AFC correction uses the frequency estimation developed by the 2*Tb estimator in the
    //   Synchronous Demodulator
    // - Disable AFC value feedback to PLL
    // - freeze AFC after gear switching
    static constexpr auto modemAfcMisc = 0xA0_b;
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modem,
        iModemAfcGear,
        Span(FlatArray(modemAfcGear, modemAfcWait, modemAfcGain, modemAfcLimiter, modemAfcMisc))));

    // RF modem AGC control
    //
    // TODO: What values to use here?
    static constexpr auto iModemAgcControl = 0x35_b;
    // - Reset peak detectors only on change of gain indicated by peak detector output
    // - Adjustment of the ADC input gain is disabled
    // - Normal AGC speed
    // - AGC gain increases during signal reductions are prevented.
    // - The RF programmable gain loop will always perform gain decreases in -3 dB steps.
    // - The IF programmable gain loop will always perform gain decreases in -3 dB steps.
    // - AGC function operates over the entire packet.
    static constexpr auto modemAgcControl = 0xE0_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::modem, iModemAgcControl, Span({modemAgcControl})));

    // RF modem AGC window size, AGC RF peak detector decay, AGC IF peak detector decay, 4FSK gain,
    // 4FSK slicer threshold, 4FSK symbol mapping code, OOK attack/decay times
    //
    // TODO: What values to use here?
    static constexpr auto iModemAgcWindowSize = 0x38_b;
    // AGC gain settling window size = 1, AGC signal level measurement window = 1
    static constexpr auto modemAgcWindowSize = 0x11_b;
    // RF peak detector decay time = 0x25
    static constexpr auto modemAgcRfpdDecay = 0x25_b;
    // IF peak detector decay time = 0x25
    static constexpr auto modemAgcIfpdDecay = 0x25_b;
    // 4FSK Gain1 = 0, Disable 4(G)FSK ISI-suppression
    static constexpr auto modemFsk4Gain1 = 0x80_b;
    // 4FSK Gain0 = 26, disable 2FSK phase compensation
    static constexpr auto modemFsk4Gain0 = 0x1A_b;
    // 4FSK slicer threshold = 0x2000
    static constexpr auto modemFsk4Th = std::array{0x20_b, 0x00_b};
    // 4FSK symbol map 0 (`00 `01 `11 `10)
    static constexpr auto modemFsk4Map = 0x00_b;
    // OOK decay = 9, OOK attack = 2
    static constexpr auto modemOokPdtc = 0x29_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::modem,
                              iModemAgcWindowSize,
                              Span(FlatArray(modemAgcWindowSize,
                                             modemAgcRfpdDecay,
                                             modemAgcIfpdDecay,
                                             modemFsk4Gain1,
                                             modemFsk4Gain0,
                                             modemFsk4Th,
                                             modemFsk4Map,
                                             modemOokPdtc))));

    // RF modem OOK control, OOK misc, RAW search, RAW control, RAW eye, Antenna diversity mode,
    // antenna diversity control, RSSI threshold
    static constexpr auto iModemOokCnt1 = 0x42_b;
    // - Squelch function is off.
    // - Discriminator's slicer output is de-glitched by sample clock to reduce turn-around time.
    // - Raw data output is not synchronized to bit clock.
    // - Estimated frequency from MA detector will not be truncated.
    // - AGC and OOK moving average detector's threshold output will be frozen after the preamble is
    //   detected.
    // - S2p_mapping 2.
    static constexpr auto modemOokCnt1 = 0xA4_b;
    // - The min-max detector is selected to establish the slicing threshold level as the mid-point
    //   between the measured extreme frequency deviation levels.
    // - Does not affect OOK decay rate specified in decay[3:0] in MODEM_OOK_PDTC.
    // - Disable OOK Squelch functionality.
    // - Peak detector discharge is disabled when the detected peak is lower than the input signal
    //   for low input levels.
    // - Normal MA filter window.
    static constexpr auto modemOokMisc = 0x23_b;
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modem, iModemOokCnt1, Span(FlatArray(modemOokCnt1, modemOokMisc))));

    static constexpr auto iModemRawControl = 0x45_b;
    // - Gain = 1.
    // - If preamble has '1010' pattern, modem is recommended to work on standard packet mode.
    // - Standard packet mode.
    static constexpr auto modemRawControl = 0x03_b;
    // RAW eye open detector threshold
    static constexpr auto modemRawEye = std::array{0x00_b, 0x3D_b};
    // Antenna diversity mode
    static constexpr auto modemAntDivMode = 0x01_b;
    // Antenna diversity control
    static constexpr auto modemAntDivControl = 0x00_b;
    // Threshold for clear channel assessment and RSSI interrupt generation
    static constexpr auto modemRssiThresh = 0xFF_b;
    static constexpr auto modemRssiJumpThresh = 0x06_b;
    // Disable RSSI latch, RSSI value is avg over last 4 * Tb bit periods, disable RSSI threshold
    // check after latch
    static constexpr auto modemRssiControl = 0x00_b;
    static constexpr auto modemRssiControl2 = 0x18_b;
    // Compensation/offset of measured RSSI value
    // TODO: Measure this
    static constexpr auto modemRssiComp = 0x40_b;
    OUTCOME_TRY(SetProperties(PropertyGroup::modem,
                              iModemRawControl,
                              Span(FlatArray(modemRawControl,
                                             modemRawEye,
                                             modemAntDivMode,
                                             modemAntDivControl,
                                             modemRssiThresh,
                                             modemRssiJumpThresh,
                                             modemRssiControl,
                                             modemRssiControl2,
                                             modemRssiComp))));

    // RF modem clock generation band
    static constexpr auto iModemRawSearch = 0x50_b;
    // - Search window period after gear switching = 8*TB
    // - Search window period before gear switching = 2*TB
    // - Disable raw data filter to use the 4-tap MA filter.
    // - Freeze the Moving Average or Min-Max slicing threshold search engine upon switching to low
    //   gear.
    static constexpr auto modemRawSearch = 0x84_b;
    // Band = FVCO_DIV_8, high performance mode fixed prescaler div2, force recalibration
    static constexpr auto modemClkgenBand = 0x0A_b;
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modem, iModemRawSearch, Span({modemRawSearch, modemClkgenBand})));

    // RF modem spike detection
    static constexpr auto iModemSpikeDet = 0x54_b;
    // - 0x03 spike threshold
    // - Disable (G)FSK Spike Removal Function.
    static constexpr auto modemSpikeDet = 0x03_b;
    // - 7 bit periods delay in one shot AFC
    // - Disable MA filter for frequency error estimator.
    // - Disable data rate error measurement and compensation upon signal arrival detection.
    // - Allow BCR tracking prior to signal arrival.
    // - Disable One shot AFC function.
    static constexpr auto modemOneShotAfc = 0x07_b;
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modem, iModemSpikeDet, Span({modemSpikeDet, modemOneShotAfc})));

    // RF modem DSA control
    static constexpr auto iModemDsaCtrl1 = 0x5B_b;
    static constexpr auto modemDsaCtrl1 = 0x40_b;
    static constexpr auto modemDsaCtrl2 = 0x04_b;
    static constexpr auto modemDsaQual = 0x04_b;
    static constexpr auto modemDsaRssi = 0x78_b;
    static constexpr auto modemDsaMisc = 0x20_b;
    OUTCOME_TRY(SetProperties(
        PropertyGroup::modem,
        iModemDsaCtrl1,
        Span({modemDsaCtrl1, modemDsaCtrl2, modemDsaQual, modemDsaRssi, modemDsaMisc})));

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
    SelectChip();
    WriteTo(&rfSpi, Span(cmdWriteTxFifo), spiTimeout);
    WriteTo(&rfSpi, data, spiTimeout);
    DeselectChip();
    // TODO: What do we do in case of a timeout?
    // TODO: Wouldn't it be more efficient to wait for CTS before writing instead of after it?
    return BusyWaitForCts(ctsTimeout);
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
            "Modem status: Pending Interrupt Flags: %02x, Interrupt Flags: %02x, Current RSSI: "
            "%4.1fdBm, Latched RSSI: %4.1fdBm, AFC Frequency Offset: %6d\n",
            static_cast<int>(modemStatus[0]),  // Pending Modem Interrupt Flags
            static_cast<int>(modemStatus[1]),  // Modem Interrupt Flags
            (static_cast<double>(static_cast<int>(modemStatus[2])) / 2.0) - 70,  // Current RSSI
            (static_cast<double>(static_cast<int>(modemStatus[3])) / 2.0) - 70,  // Latched RSSI
            // static_cast<int>(modemStatus[4]), // ANT1 RSSI
            // static_cast<int>(modemStatus[5]), // ANT2 RSSI
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
}
}

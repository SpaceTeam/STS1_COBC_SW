//! @file
//! @brief  Driver for the RF module Si4463.
//!
//! See "AN625: Si446x API Descriptions" for more information.


#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/FlatArray.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/difference.hpp>

#include <array>
#include <bit>
#include <cinttypes>
#include <cstddef>


namespace sts1cobcsw::rf
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


// --- Public globals ---

bool rfIsWorking = true;


// --- Private globals ---

// Si4463 commands
constexpr auto cmdPartInfo = 0x01_b;
constexpr auto cmdPowerUp = 0x02_b;
constexpr auto cmdFuncInfo = 0x11_b;
constexpr auto cmdSetProperty = 0x11_b;
constexpr auto cmdGpioPinCfg = 0x13_b;
constexpr auto cmdFifoInfo = 0x15_b;
constexpr auto cmdGetIntStatus = 0x20_b;
constexpr auto cmdGetModemStatus = 0x22_b;
constexpr auto cmdStartTx = 0x31_b;
constexpr auto cmdRequestDeviceState = 0x33_b;
constexpr auto cmdChangeState = 0x34_b;
constexpr auto cmdReadCmdBuff = 0x44_b;
constexpr auto cmdWriteTxFifo = 0x66_b;

// Si4463 property indexes
constexpr auto iIntCtlPhEnable = 0x01_b;

// Command answer lengths
//
// TODO: We should do it similarly to SimpleInstruction and SendInstruction() in Flash.cpp. Unless
// this remains the only command with an answer. Then we should probably get rid of SendCommand<>()
// instead.
constexpr auto partInfoAnswerLength = 8U;
constexpr auto interruptStatusAnswerLength = 8U;
// Max. number of properties that can be set in a single command
constexpr auto maxNProperties = 12;

// Delay to wait for power on reset to finish
constexpr auto porRunningDelay = 20 * ms;
// Time until PoR circuit settles after applying power
constexpr auto porCircuitSettleDelay = 100 * ms;
// Delay for the sequence reset -> pause -> set -> pause -> reset in initialization
constexpr auto watchDogResetPinDelay = 1 * ms;
// TODO: Check this and write a good comment
constexpr auto spiTimeout = 1 * ms;

// Trigger TX FIFO almost empty interrupt when 32/64 bytes are empty
constexpr auto txFifoThreshold = 32_b;
// Trigger RX FIFO almost full interrupt when 48/64 bytes are filled
constexpr auto rxFifoThreshold = 48_b;

// Trigger TX FIFO almost empty interrupt when 32/64 bytes are empty
constexpr auto txFifoThreshold = 32_b;
// Trigger RX FIFO almost full interrupt when 48/64 bytes are filled
constexpr auto rxFifoThreshold = 48_b;

auto csGpioPin = hal::GpioPin(hal::rfCsPin);
auto nirqGpioPin = hal::GpioPin(hal::rfNirqPin);
auto sdnGpioPin = hal::GpioPin(hal::rfSdnPin);
auto gpio0GpioPin = hal::GpioPin(hal::rfGpio0Pin);
auto gpio1GpioPin = hal::GpioPin(hal::rfGpio1Pin);
auto paEnablePin = hal::GpioPin(hal::rfPaEnablePin);

// TODO: This should probably be somewhere else as it is not directly related to the RF module
auto watchdogResetGpioPin = hal::GpioPin(hal::watchdogClearPin);


// --- Private function declarations ---

auto InitializeGpiosAndSpi() -> void;
auto ApplyPatch() -> void;
auto PowerUp() -> void;
auto Configure(TxType txType) -> void;
auto SendCommand(std::span<Byte const> data) -> void;
template<std::size_t answerLength>
auto SendCommand(std::span<Byte const> data) -> std::array<Byte, answerLength>;
auto WaitForCts() -> void;
template<std::size_t extent>
    requires(extent <= maxNProperties)
auto SetProperties(PropertyGroup propertyGroup,
                   Byte startIndex,
                   std::span<Byte const, extent> propertyValues) -> void;
auto ClearTxFifo() -> void;
auto ClearRxFifo() -> void;
auto ClearFifos() -> void;
auto ReadAndClearInterruptStatus() -> std::array<Byte, interruptStatusAnswerLength>;
auto EnterStandby() -> void;
auto WriteToFifo(std::span<Byte const> data) -> void;
auto ReadFromFifo(std::span<Byte> data) -> void;
auto StartTx() -> void;


// --- Public function definitions ---

auto Initialize(TxType txType) -> void
{
    InitializeGpiosAndSpi();
    ApplyPatch();
    PowerUp();
    Configure(txType);
    // Power amplifier should only be turned on after the configuration is done
    paEnablePin.Set();
}


auto ReadPartNumber() -> std::uint16_t
{
    auto answer = SendCommand<partInfoAnswerLength>(Span(cmdPartInfo));
    return Deserialize<std::endian::big, std::uint16_t>(Span(answer).subspan<1, 2>());
}


auto ReadPartInfo() -> std::array<Byte, 8>
{
    return SendCommand<partInfoAnswerLength>(Span(cmdPartInfo));
}


auto ReadFunctionInfo() -> std::array<Byte, 6>
{
    return SendCommand<6>(Span(cmdFuncInfo));
}


auto ReadModemStatus() -> std::array<Byte, 8>
{
    return SendCommand<8>(Span(cmdGetModemStatus));
}


auto ReadDeviceState() -> std::array<Byte, 2>
{
    return SendCommand<2>(Span(cmdRequestDeviceState));
}


auto SetTxType(TxType txType) -> void
{
    // Constants for setting the TX type (morse, 2GFSK)
    // MODEM_DATA_RATE: unused, 20 kBaud
    static constexpr std::uint32_t dataRateMorse = 20'000U * 40U;
    // MODEM_DATA_RATE: For 9k6 Baud: (TX_DATA_RATE * MODEM_TX_NCO_MODE * TXOSR) / F_XTAL_Hz = (9600 * 26000000 * 40)/26000000 = 9600 * 40
    static constexpr std::uint32_t dataRate2Gfsk = 9'600U * 40U;
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
    SetProperties(
        PropertyGroup::modem,
        startIndex,
        Span(FlatArray(modemModeType,
                       modemMapControl,
                       modemDsmCtrl,
                       // The data rate property is only 3 bytes wide, so drop the first byte
                       Span(Serialize<std::endian::big>(dataRate)).subspan<1>())));
}


// TODO: Do we need to clear all FIFOs here, should be just TX FIFO
// TODO: Refactor (issue #226)
// TODO: Pull rfLatchupDisableGpioPin high while sending
auto Send(void const * data, std::size_t nBytes) -> void
{
    // TODO: Acc. the datasheet "fifo hardware does not need to be reset prior to use".
    ClearFifos();

    // Set TX Data Length
    // TODO: Check if we can just set the length in START_TX
    // TODO: Use Deserialize(), but length should then be uint16_t
    static constexpr auto iPktField1Length = 0x0D_b;
    auto lengthUpperBits = static_cast<Byte>(nBytes >> CHAR_BIT);
    auto lengthLowerBits = static_cast<Byte>(nBytes);
    SetProperties(PropertyGroup::pkt, iPktField1Length, Span({lengthUpperBits, lengthLowerBits}));

    // Fill the TX FIFO with chunkSize bytes each round
    static constexpr auto chunkSize = static_cast<unsigned int>(txFifoThreshold);
    auto almostEmptyInterruptEnabled = false;

    // While the packet is longer than a single fill round, wait for the almost empty interrupt,
    // afterwards for the packet sent interrupt
    auto dataIndex = 0U;
    auto dataSpan = std::span(static_cast<Byte const *>(data), nBytes);
    while(dataIndex + chunkSize < nBytes)
    {
        // Enable the almost empty interrupt in the first round
        if(not almostEmptyInterruptEnabled)
        {
            // TODO: Setting interrupts could be put in a function
            static constexpr auto txFifoAlmostEmptyInterrupt = 0b0000'0010_b;
            SetProperties(PropertyGroup::intCtl, iIntCtlPhEnable, Span(txFifoAlmostEmptyInterrupt));
            almostEmptyInterruptEnabled = true;
        }

        // Write chunkSize bytes to the TX FIFO
        WriteToFifo(dataSpan.subspan(dataIndex, chunkSize));
        ReadAndClearInterruptStatus();
        if(dataIndex == 0)
        {
            StartTx();
        }
        dataIndex += chunkSize;
        // Wait for TX FIFO almost empty interrupt
        //
        // TODO: Wait more intelligently by computing the estimated time t_0 it takes to send
        // chunkSize bytes: t_0 = chunkSize * 10 / baudRate (maybe even chunkSize + 1). Then wait
        // for the time, t_1 = 10 / baudRate, it takes to send a single byte in a loop. Also add a
        // timeout to not wait indefinitely.
        while(nirqGpioPin.Read() == hal::PinState::set)
        {
            RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
        }
    }

    // Enable packet sent interrupt
    static constexpr auto packetSentInterrupt = 0b0010'0000_b;
    SetProperties(PropertyGroup::intCtl, iIntCtlPhEnable, Span(packetSentInterrupt));

    ReadAndClearInterruptStatus();

    // Write the rest of the data
    // NOLINTNEXTLINE(*pointer-arithmetic)
    WriteToFifo(dataSpan.subspan(dataIndex));

    auto startTime = RODOS::NOW();

    // Wait for Packet Sent Interrupt
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        // TODO: Set timeout constant
        if(RODOS::NOW() - startTime > 1 * RODOS::SECONDS)
        {
            break;
        }
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
    }
    EnterStandby();
}


auto ReceiveTestData() -> std::array<Byte, maxRxBytes>
{
    // auto sendBuffer = std::array<std::uint8_t, 32>{};

    ClearFifos();

    // Enable RX FIFO almost full interrupt as well as preamble and sync detect interrupts
    static constexpr auto rxFifoAlmostFullInterrupt = 0b0000'0001_b;
    static constexpr auto preambleAndSyncDetectInterrupt = 0b0000'0011_b;
    SetProperties(PropertyGroup::intCtl,
                  iIntCtlPhEnable,
                  Span(FlatArray(rxFifoAlmostFullInterrupt, preambleAndSyncDetectInterrupt)));

    ReadAndClearInterruptStatus();

    // Enter RX mode
    // sendBuffer[0] = 0x32;
    // sendBuffer[1] = 0x00;  // "Channel 0"
    // sendBuffer[2] = 0x00;  // Start RX now
    // sendBuffer[3] = 0x00;  // RX Length = 0
    // sendBuffer[4] = 0x00;
    // sendBuffer[5] = 0x00;  // Remain in RX state on preamble detection timeout
    // sendBuffer[6] = 0x00;  // Do nothing on RX packet valid (we'll never enter this state)
    // sendBuffer[7] = 0x00;  // Do nothing on RX packet invalid (we'll never enter this state)
    SendCommand(Span({0x32_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b}));

    // Wait for interrupts
    auto i = 0U;
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        // if(nirqGpioPin.Read() == hal::PinState::reset)
        // {
        //     auto interruptStatus = ReadAndClearInterrupts();
        //     auto preambleOrSyncWasDetected = (interruptStatus[4] & 0b11_b) != 0_b;
        //     if(preambleOrSyncWasDetected)
        //     {
        //         auto modemStatus = ReadModemStatus();
        //         DEBUG_PRINT("Modem status: %02x %02x %d %d %d %d %d\n",
        //                     static_cast<int>(modemStatus[0]),
        //                     static_cast<int>(modemStatus[1]),
        //                     static_cast<int>(modemStatus[2]),
        //                     static_cast<int>(modemStatus[3]),
        //                     static_cast<int>(modemStatus[4]),
        //                     static_cast<int>(modemStatus[5]),
        //                     (static_cast<unsigned>(modemStatus[6]) << 8U)
        //                         + static_cast<unsigned>(modemStatus[7]));
        //     }
        //     auto rxFifoIsAlmostFull = (interruptStatus[2] & 1_b) != 0_b;
        //     if(rxFifoIsAlmostFull)
        //     {
        //         break;
        //     }
        // }
        if(i % 200 == 0)
        {
            auto modemStatus = ReadModemStatus();
            DEBUG_PRINT("Modem status: %02x %02x %d %d %d %d %d\n",
                        static_cast<int>(modemStatus[0]),
                        static_cast<int>(modemStatus[1]),
                        static_cast<int>(modemStatus[2]),
                        static_cast<int>(modemStatus[3]),
                        static_cast<int>(modemStatus[4]),
                        static_cast<int>(modemStatus[5]),
                        (static_cast<unsigned>(modemStatus[6]) << 8U)
                            + static_cast<unsigned>(modemStatus[7]));
        }
        RODOS::AT(RODOS::NOW() + 1 * RODOS::MILLISECONDS);
        i++;
    }
    auto modemStatus = ReadModemStatus();
    DEBUG_PRINT(
        "Modem status: %02x %02x %d %d %d %d %d\n",
        static_cast<int>(modemStatus[0]),
        static_cast<int>(modemStatus[1]),
        static_cast<int>(modemStatus[2]),
        static_cast<int>(modemStatus[3]),
        static_cast<int>(modemStatus[4]),
        static_cast<int>(modemStatus[5]),
        (static_cast<unsigned>(modemStatus[6]) << 8U) + static_cast<unsigned>(modemStatus[7]));

    auto rxBuffer = std::array<Byte, maxRxBytes>{};
    static constexpr auto chunkSize = static_cast<unsigned int>(rxFifoThreshold);
    ReadFromFifo(Span(&rxBuffer).first<chunkSize>());

    DEBUG_PRINT("Retrieved first %d bytes from FIFO\n", chunkSize);

    ReadAndClearInterruptStatus();

    // Wait for RX FIFO Almost Full Interrupt
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
    }

    ReadFromFifo(Span(&rxBuffer).subspan<chunkSize, chunkSize>());

    EnterStandby();
    ReadAndClearInterruptStatus();

    return rxBuffer;
}


// --- Private function definitions ---

auto InitializeGpiosAndSpi() -> void
{
    csGpioPin.Direction(hal::PinDirection::out);
#if HW_VERSION >= 30
    csGpioPin.OutputType(hal::PinOutputType::openDrain);
#else
    csGpioPin.OutputType(hal::PinOutputType::pushPull);
#endif
    csGpioPin.Set();
    nirqGpioPin.Direction(hal::PinDirection::in);
    sdnGpioPin.Direction(hal::PinDirection::out);
    sdnGpioPin.Set();
    gpio0GpioPin.Direction(hal::PinDirection::out);
    gpio0GpioPin.Reset();
    paEnablePin.Direction(hal::PinDirection::out);
    paEnablePin.Reset();
    watchdogResetGpioPin.Direction(hal::PinDirection::out);
    // The watchdog must be fed regularely for the TX to work. Even without the watchdog timer on
    // the PCB it needs to be triggered at least once after boot to enable the TX.
    watchdogResetGpioPin.Reset();
    SuspendFor(watchDogResetPinDelay);
    watchdogResetGpioPin.Set();
    SuspendFor(watchDogResetPinDelay);
    watchdogResetGpioPin.Reset();

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
auto ApplyPatch() -> void
{
    // Template argument deduction for std::array doesn't work because the array is too large. Lol.
    // Every line of the patch array starts with a one-byte length and then as many data bytes. We
    // got the patch data from some configuration tool that Andriy found.
    //
    // clang-format off
    static constexpr Byte patch[] = {
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
    };
    // clang-format on

    // We assume that the data length is the same for every line.
    static constexpr auto dataLength = static_cast<std::uint8_t>(patch[0]);
    static_assert(std::size(patch) % (dataLength + 1U) == 0);
    for(auto i = 1U; i < std::size(patch); i += dataLength + 1U)
    {
        SendCommand(Span(patch).subspan(i, dataLength));
    }
}


auto PowerUp() -> void
{
    static constexpr auto bootOptions = 0x81_b;
    static constexpr auto xtalOptions = 0x00_b;
    static constexpr std::uint32_t xoFreq = 26'000'000;  // MHz
    SendCommand(FlatArray(
        cmdPowerUp, bootOptions, xtalOptions, Serialize<std::endian::big, std::uint32_t>(xoFreq)));
}


auto Configure(TxType txType) -> void
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
    // NIRQ is still used as NIRQ
    static constexpr auto nirqConfig = 0x27_b;
    // SDO is still used as SDO but enable internal pull-up
    static constexpr auto sdoConfig = 0x4B_b;
    // GPIOs configured as outputs will have highest drive strength
    static constexpr auto genConfig = 0x00_b;
    SendCommand<7>(Span({cmdGpioPinCfg,
                         gpio0Config,
                         gpio1Config,
                         gpio2Config,
                         gpio3Config,
                         nirqConfig,
                         sdoConfig,
                         genConfig}));

    // Crystal oscillator frequency and clock
    static constexpr auto iGlobalXoTune = 0x00_b;
    static constexpr auto globalXoTune = 0x52_b;
    static constexpr auto globalClkCfg = 0x00_b;
    SetProperties(PropertyGroup::global, iGlobalXoTune, Span({globalXoTune, globalClkCfg}));

    // Global config
    static constexpr auto iGlobalConfig = 0x03_b;
    // High performance mode, generic packet format, split FIFO mode, fast sequencer mode
    static constexpr auto globalConfig = 0x60_b;
    SetProperties(PropertyGroup::global, iGlobalConfig, Span(globalConfig));

    // Interrupt
    static constexpr auto iIntCtlEnable = 0x00_b;
    // Enable all three general interrupt sources (chip, modem, packet handler)
    static constexpr auto intCtlEnable = 0x07_b;
    // Disable all interrupts
    // TODO: We could enable the chip ready interrupt to potentially remove some hardcoded delays?
    static constexpr auto intPhEnable = 0x00_b;
    static constexpr auto intModemEnable = 0x00_b;
    static constexpr auto intChipEnable = 0x00_b;
    SetProperties(PropertyGroup::intCtl, iIntCtlEnable, Span({intCtlEnable, intPhEnable, intModemEnable, intChipEnable}));

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
    // RX Standard preamble, first received preamble bit is 0, unit of preamble TX length is in bytes, use standard preamble 0101 pattern
    static constexpr auto preambleConfig = 0b00010010_b;
    // Non-standard pattern
    static constexpr auto preamblePattern = std::array<Byte, 4>{};
    SetProperties(PropertyGroup::preamble,
                  iPreambleTxLength,
                  Span(FlatArray(preambleTxLength,
                                 preambleConfigStd1,
                                 preambleConfigNstd,
                                 preambleConfigStd2,
                                 preambleConfig,
                                 preamblePattern)));

    // Sync word
    static constexpr auto iSyncConfig = 0x00_b;
    // Do not transmit sync word, allow 4-bit sync word errors on receive, 4-byte sync word length
    static constexpr auto syncConfig = 0b11000011_b;
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
        std::array{0b01011000_b, 0b11110011_b, 0b00111111_b, 0b10111000_b};
    SetProperties(PropertyGroup::sync, iSyncConfig, Span(FlatArray(syncConfig, syncBits)));

    // CRC
    static constexpr auto iPktCrcConfig = 0x00_b;
    // No CRC
    static constexpr auto pktCrcConfig = 0x00_b;
    SetProperties(PropertyGroup::pkt, iPktCrcConfig, Span({pktCrcConfig}));

    // Packet length
    static constexpr auto iPktLen = 0x08_b;
    // Infinite receive, big endian (MSB first)
    static constexpr auto pktLen = 0x60_b;
    static constexpr auto pktLenFieldSource = 0x00_b;
    static constexpr auto pktLenAdjust = 0x00_b;
    static constexpr auto pktTxThreshold = txFifoThreshold;
    static constexpr auto pktRxThreshold = rxFifoThreshold;
    static constexpr auto pktField1Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktField1Config = 0x04_b;
    static constexpr auto pktField1CrcConfig = 0x00_b;
    SetProperties(PropertyGroup::pkt,
                  iPktLen,
                  Span(FlatArray(pktLen,
                                 pktLenFieldSource,
                                 pktLenAdjust,
                                 pktTxThreshold,
                                 pktRxThreshold,
                                 pktField1Length,
                                 pktField1Config,
                                 pktField1CrcConfig)));

    // RF modem mod type
    SetTxType(txType);
    // SetTxType sets modem properties from 0x00 to 0x05
    static constexpr auto iModemTxNcoMode = 0x06_b;
    // TXOSR = x40 = 0, NCOMOD = F_XTAL = 26'000'000 = 0x018CBA80
    static constexpr auto modemTxNcoMode = std::array{0x05_b, 0x8C_b, 0xBA_b, 0x80_b};
    // We use minimum shift keying, i.e., a frequency deviation of baudrate / 4. The value we need
    // to write to the property is (2^19 * outdiv * deviation_Hz) / (N_presc * F_xo) = (2^19 * 8 *
    // (9600 / 4)) / (2 * 26000000) = 194 = 0x0000C2
    static constexpr auto modemFreqDeviation = std::array{0x00_b, 0x00_b, 0xC2_b};
    SetProperties(
        PropertyGroup::modem, iModemTxNcoMode, Span(FlatArray(modemTxNcoMode, modemFreqDeviation)));

    // RF modem TX ramp delay, modem MDM control, modem IF control, modem IF frequency & modem
    // decimation
    static constexpr auto iModemTxRampDelay = 0x18_b;
    // Ramp delay 1
    static constexpr auto modemTxRampDelay = 0x01_b;
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
    SetProperties(PropertyGroup::modem,
                  iModemTxRampDelay,
                  Span(FlatArray(modemTxRampDelay,
                                 modemMdmCtrl,
                                 modemIfControl,
                                 modemIfFreq,
                                 modemDecimationCfg1,
                                 modemDecimationCfg0)));

    // RF modem BCR vversampling rate, modem BCR NCO offset, modem BCR gain, modem BCR gear & modem
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
    SetProperties(
        PropertyGroup::modem,
        iModemBcrOsr,
        Span(FlatArray(modemBcrOsr, modemBcrNcoOffset, modemBcrGain, modemBcrGear, modemBcrMisc1)));

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
    // - AFC correction of PLL will be frozen if a consecutive string of 1 s or 0 s that exceed the search period is encountered
    // - don't switch clock source for frequency estimator
    // - don't freeze AFC at preamble end
    // - AFC correction uses the frequency estimation developed by the 2*Tb estimator in the Synchronous Demodulator
    // - disable AFC value feedback to PLL
    // - freeze AFC after gear switching
    static constexpr auto modemAfcMisc = 0xA0_b;
    SetProperties(
        PropertyGroup::modem,
        iModemAfcGear,
        Span(FlatArray(modemAfcGear, modemAfcWait, modemAfcGain, modemAfcLimiter, modemAfcMisc)));

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
    SetProperties(PropertyGroup::modem, iModemAgcControl, Span({modemAgcControl}));

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
    SetProperties(PropertyGroup::modem,
                  iModemAgcWindowSize,
                  Span(FlatArray(modemAgcWindowSize,
                                 modemAgcRfpdDecay,
                                 modemAgcIfpdDecay,
                                 modemFsk4Gain1,
                                 modemFsk4Gain0,
                                 modemFsk4Th,
                                 modemFsk4Map,
                                 modemOokPdtc)));

    // RF modem OOK control, OOK misc, RAW search, RAW control, RAW eye, Antenna diversity mode,
    // antenna diversity control, RSSI threshold
    static constexpr auto iModemOokCnt1 = 0x42_b;
    // - Squelch function is off.
    // - Discriminator's slicer output is de-glitched by sample clock to reduce turn-around time.
    // - Raw data output is not synchronized to bit clock.
    // - Estimated frequency from MA detector will not be truncated.
    // - AGC and OOK moving average detector's threshold output will be frozen after the preamble is detected.
    // - S2p_mapping 2.
    static constexpr auto modemOokCnt1 = 0xA4_b;
    // - The min-max detector is selected to establish the slicing threshold level as the mid-point between the measured extreme frequency deviation levels.
    // - Does not affect OOK decay rate specified in decay[3:0] in MODEM_OOK_PDTC.
    // - Disable OOK Squelch functionality.
    // - Peak detector discharge is disabled when the detected peak is lower than the input signal for low input levels.
    // - Normal MA filter window.
    static constexpr auto modemOokMisc = 0x23_b;
    SetProperties(
        PropertyGroup::modem,
        iModemOokCnt1,
        Span(FlatArray(
            modemOokCnt1,
            modemOokMisc)));

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
    SetProperties(
        PropertyGroup::modem,
        iModemRawControl,
        Span(FlatArray(
            modemRawControl,
            modemRawEye,
            modemAntDivMode,
            modemAntDivControl,
            modemRssiThresh,
	    modemRssiJumpThresh,
	    modemRssiControl,
	    modemRssiControl2,
	    modemRssiComp)));

    // RF modem clock generation band
    static constexpr auto iModemRawSearch = 0x50_b;
    // - Search window period after gear switching = 8*TB
    // - Search window period before gear switching = 2*TB
    // - Disable raw data filter to use the 4-tap MA filter.
    // - Freeze the Moving Average or Min-Max slicing threshold search engine upon switching to low gear.
    static constexpr auto modemRawSearch = 0x84_b;
    // Band = FVCO_DIV_8, high performance mode fixed prescaler div2, force recalibration
    static constexpr auto modemClkgenBand = 0x0A_b;
    SetProperties(PropertyGroup::modem, iModemRawSearch, Span({modemRawSearch, modemClkgenBand}));

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
    SetProperties(PropertyGroup::modem, iModemSpikeDet, Span({modemSpikeDet, modemOneShotAfc}));

    // RF modem DSA control
    static constexpr auto iModemDsaCtrl1 = 0x5B_b;
    static constexpr auto modemDsaCtrl1 = 0x40_b;
    static constexpr auto modemDsaCtrl2 = 0x04_b;
    static constexpr auto modemDsaQual = 0x04_b;
    static constexpr auto modemDsaRssi = 0x78_b;
    static constexpr auto modemDsaMisc = 0x20_b;
    SetProperties(PropertyGroup::modem, iModemDsaCtrl1, Span({
                modemDsaCtrl1,
                modemDsaCtrl2,
                modemDsaQual,
                modemDsaRssi,
                modemDsaMisc}));

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
    SetProperties(
        PropertyGroup::modemChflt, iRxFilterCoefficientsBlock1, Span(rxFilterCoefficientsBlock1));

    // Block 2
    static constexpr auto iRxFilterCoefficientsBlock2 = 0x0C_b;
    static constexpr auto rxFilterCoefficientsBlock2 = std::array{
        0x03_b,  // RX1_CHFLT_COE1[7:0]
        0x00_b,  // RX1_CHFLT_COE0[7:0]
        0x15_b,  // RX1_CHFLT_COE10[9:8]  | RX1_CHFLT_COE11[9:8]  | RX1_CHFLT_COE12[9:8]  | RX1_CHFLT_COE13[9:8]
        0xFF_b,  // RX1_CHFLT_COE6[9:8]   | RX1_CHFLT_COE7[9:8]   | RX1_CHFLT_COE8[9:8]   | RX1_CHFLT_COE9[9:8]
        0x00_b,  // RX1_CHFLT_COE2[9:8]   | RX1_CHFLT_COE3[9:8]   | RX1_CHFLT_COE4[9:8]   | RX1_CHFLT_COE5[9:8]
        0x00_b,  // 0 | 0 | 0 | 0         | RX1_CHFLT_COE0[9:8]   | RX1_CHFLT_COE1[9:8]
        0xFF_b,  // RX2_CHFLT_COE13[7:0]
        0xC4_b,  // RX2_CHFLT_COE12[7:0]
        0x30_b,  // RX2_CHFLT_COE11[7:0]
        0x7F_b,  // RX2_CHFLT_COE10[7:0]
        0xF5_b,  // RX2_CHFLT_COE9[7:0]
        0xB5_b   // RX2_CHFLT_COE8[7:0]
    };
    SetProperties(
        PropertyGroup::modemChflt, iRxFilterCoefficientsBlock2, Span(rxFilterCoefficientsBlock2));

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
        0x15_b,  // RX2_CHFLT_COE10[9:8]  | RX2_CHFLT_COE11[9:8]  | RX2_CHFLT_COE12[9:8] | RX2_CHFLT_COE13[9:8]
        0xFF_b,  // RX2_CHFLT_COE6[9:8]   | RX2_CHFLT_COE7[9:8]   | RX2_CHFLT_COE8[9:8]  | RX2_CHFLT_COE9[9:8]
        0x00_b,  // RX2_CHFLT_COE2[9:8]   | RX2_CHFLT_COE3[9:8]   | RX2_CHFLT_COE4[9:8]  | RX2_CHFLT_COE5[9:8]
        0x00_b   // 0 | 0 | 0 | 0         | RX2_CHFLT_COE0[9:8]   | RX2_CHFLT_COE1[9:8]  |
    };
    SetProperties(
        PropertyGroup::modemChflt, iRxFilterCoefficientsBlock3, Span(rxFilterCoefficientsBlock3));

    // RF PA mode
    static constexpr auto iPaMode = 0x00_b;
    // PA switching amp mode, PA_SEL = HP_COARSE, disable power sequencing, disable external TX ramp signal
    static constexpr auto paMode = 0x08_b;
    // Enabled PA fingers (sets output power but not linearly; 10 µA bias current per enabled
    // finger, complementary drive signal with 50 % duty cycle)
    static constexpr auto paPwrLvl = 0x2f_b;
    static constexpr auto paBiasClkduty = 0x00_b;
    // Ramping time constant = 0x1B (~10 µs to full - 0.5 dB), FSK modulation delay 10 µs
    static constexpr auto paTc = 0x91_b;
    SetProperties(PropertyGroup::pa, iPaMode, Span({paMode, paPwrLvl, paBiasClkduty, paTc}));

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
    SetProperties(PropertyGroup::synth,
                  iSynthPfdcpCpff,
                  Span({synthPfdcpCpff,
                        synthPfdcpCpint,
                        synthVcoKv,
                        synthLpfilt3,
                        synthLpfilt2,
                        synthLpfilt1,
                        synthLpfilt0}));

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
    SetProperties(PropertyGroup::match,
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
                        matchCtrl4}));

    // Frequency control
    static constexpr auto iFreqControlInte = 0x00_b;
    // FC_inte = 0x41
    static constexpr auto freqControlInte = 0x41_b;
    // FC_frac. 0xD89D9 = 433.5, 0xEC4EC = 434.5, N_presc = 2, outdiv = 8, F_xo = 26 MHz,
    // RF_channel_Hz = (FC_inte + FC_frac / 2^19) * ((N_presc * F_xo) / outdiv)
    static constexpr auto freqControlFrac = std::array{0x0E_b, 0xC4_b, 0xEC_b};
    // Channel step size = 0x4EC5
    static constexpr auto freqControlChannelStepSize = std::array{0x4E_b, 0xC5_b};
    // Window gating period (in number of crystal clock cycles) = 32
    static constexpr auto freqControlWSize = 0x20_b;
    // Adjust target mode for VCO calibration in RX mode = 0xFE int8_t
    static constexpr auto freqControlVcontRxAdj = 0xFE_b;
    SetProperties(PropertyGroup::freqControl,
                  iFreqControlInte,
                  Span(FlatArray(freqControlInte,
                                 freqControlFrac,
                                 freqControlChannelStepSize,
                                 freqControlWSize,
                                 freqControlVcontRxAdj)));

    // Frequency adjust (stolen from Arduino demo code)
    //static constexpr auto globalXoTuneUpdated = 0x62_b;
    //SetProperties(PropertyGroup::global, iGlobalXoTune, Span({globalXoTuneUpdated}));

    // Change sequencer mode to guaranteed
    //
    // TODO: Why?
    //
    // Split FIFO and guaranteed sequencer mode
    //static constexpr auto newGlobalConfig = 0x40_b;
    //SetProperties(PropertyGroup::global, iGlobalConfig, Span({newGlobalConfig}));
}


auto SendCommand(std::span<Byte const> data) -> void
{
    (void)SendCommand<0>(data);
}


template<std::size_t answerLength>
auto SendCommand(std::span<Byte const> data) -> std::array<Byte, answerLength>
{
    csGpioPin.Reset();
    hal::WriteTo(&rfSpi, data, spiTimeout);
    csGpioPin.Set();
    WaitForCts();
    auto answer = std::array<Byte, answerLength>{};
    if constexpr(answerLength != 0)
    {
        hal::ReadFrom(&rfSpi, Span(&answer), spiTimeout);
    }
    csGpioPin.Set();
    return answer;
}


//! @brief Polls the CTS byte until the Si4463 chip is ready for a new command.
//!
//! @note This function keeps the CS pin low when it returns.
// TODO: Refactor the whole waiting for CTS so that the caller of this function does not need to
// remember to pull the CS pin high afterwards. Maybe rework it into something like
// WaitForResponse/Answer<answerLength>().
auto WaitForCts() -> void
{
    auto const dataIsReadyValue = 0xFF_b;
    auto const pollingDelay = 50 * us;
    do
    {
        csGpioPin.Reset();
        hal::WriteTo(&rfSpi, Span(cmdReadCmdBuff), spiTimeout);
        auto cts = 0x00_b;
        hal::ReadFrom(&rfSpi, Span(&cts), spiTimeout);
        if(cts == dataIsReadyValue)
        {
            break;
        }
        csGpioPin.Set();
        SuspendFor(pollingDelay);
    } while(true);
    // TODO: We need to get rid of this infinite loop once we do proper error handling for the whole
    // RF code
}


template<std::size_t extent>
    requires(extent <= maxNProperties)
inline auto SetProperties(PropertyGroup propertyGroup,
                          Byte startIndex,
                          std::span<Byte const, extent> propertyValues) -> void
{
    SendCommand(FlatArray(cmdSetProperty,
                          static_cast<Byte>(propertyGroup),
                          static_cast<Byte>(extent),
                          startIndex,
                          propertyValues));
}


auto ClearTxFifo() -> void
{
    static constexpr auto resetTxFifo = 0b01_b;
    // FIXME: Acc. the datasheet this command has a 3-byte answer. Why does this work?
    SendCommand(Span({cmdFifoInfo, resetTxFifo}));
}


auto ClearRxFifo() -> void
{
    static constexpr auto resetRxFifo = 0b10_b;
    // FIXME: Acc. the datasheet this command has a 3-byte answer. Why does this work?
    SendCommand(Span({cmdFifoInfo, resetRxFifo}));
}


auto ClearFifos() -> void
{
    ClearTxFifo();
    ClearRxFifo();
}


// Read and clear all pending interrupts
auto ReadAndClearInterruptStatus() -> std::array<Byte, interruptStatusAnswerLength>
{
    return SendCommand<interruptStatusAnswerLength>(
        Span({cmdGetIntStatus, 0x00_b, 0x00_b, 0x00_b}));
}


auto EnterStandby() -> void
{
    static constexpr auto standbyMode = 0x01_b;
    SendCommand(Span({cmdChangeState, standbyMode}));
}


// TODO: Refactor (issue #226)
auto WriteToFifo(std::span<Byte const> data) -> void
{
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    WriteTo(&spi, Span(cmdWriteTxFifo), spiTimeout);
    WriteTo(&spi, data, spiTimeout);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();
    WaitForCts();
    csGpioPin.Set();
}


auto ReadFromFifo(std::span<Byte> data) -> void
{
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    // auto buf = std::to_array<std::uint8_t>({0x77});
    // spi.write(std::data(buf), std::size(buf));
    WriteTo(&spi, Span(0x77_b), spiTimeout);
    // spi.read(data, length);
    ReadFrom(&spi, data, spiTimeout);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();
}


auto StartTx() -> void
{
    static constexpr auto channel = 0x00_b;
    // [7:4]: TXCOMPLETE_STATE: 0b0011 -> READY state
    // [3]: UPDATE: 0b0 -> Use TX parameters to enter TX mode
    // [2]: RETRANSMIT: 0b0 -> Send from TX FIFO, not last packet
    // [1:0]: START: 0b0 -> Start TX immediately
    static constexpr auto condition = 0x30_b;
    // Length is set in Send() via SetProperty, not in StartTx
    // TODO: Decide on either approach, and use Deserialize()
    static constexpr auto txLen = std::array{0x00_b, 0x00_b};
    static constexpr auto txDelay = 0x00_b;
    static constexpr auto numRepeat = 0x00_b;
    // FIXME: Acc. the datasheet this command consists of only 5 bytes. Why does this work?
    SendCommand(Span(FlatArray(cmdStartTx, channel, condition, txLen, txDelay, numRepeat)));
}
}

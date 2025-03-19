//! @file
//! @brief  Driver for the RF module Si4463.
//!
//! See "AN625: Si446x API Descriptions" for more information.


#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Periphery/Spis.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Time/RodosTime.hpp>
#include <Sts1CobcSw/Utility/FlatArray.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <span>


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
constexpr auto cmdSetProperty = 0x11_b;
constexpr auto cmdGpioPinCfg = 0x13_b;
constexpr auto cmdReadCmdBuff = 0x44_b;

// Command answer lengths
//
// TODO: We should do it similarly to SimpleInstruction and SendInstruction() in Flash.cpp. Unless
// this remains the only command with an answer. Then we should probably get rid of SendCommand<>()
// instead.
constexpr auto partInfoAnswerLength = 8U;
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


// --- Public function definitions ---

auto Initialize(TxType txType) -> void
{
    // TODO: Don't forget that WDT_Clear has to be triggered regularely for the TX to work! (even
    // without the watchdog timer on the PCB it needs to be triggered at least once after boot to
    // enable the TX)
    InitializeGpiosAndSpi();
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


auto SetTxType(TxType txType) -> void
{
    // Constants for setting the TX type (morse, 2GFSK)
    // MODEM_DATA_RATE: unused, 20 kBaud
    static constexpr std::uint32_t dataRateMorse = 20'000U;
    // MODEM_DATA_RATE: For 9k6 Baud: (TX_DATA_RATE * MODEM_TX_NCO_MODE * TXOSR) / F_XTAL_Hz = (9600
    // * 2600000 * 10) / 26000000 = 9600 = 0x002580
    static constexpr std::uint32_t dataRate2Gfsk = 9'600U;
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
    // The watchdog must be reset at least once to enable the RF module
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


auto PowerUp() -> void
{
    static constexpr auto bootOptions = 0x01_b;
    static constexpr auto xtalOptions = 0x00_b;
    static constexpr std::uint32_t xoFreq = 26'000'000;  // 26 MHz
    SendCommand(FlatArray(
        cmdPowerUp, bootOptions, xtalOptions, Serialize<std::endian::big, std::uint32_t>(xoFreq)));
}


auto Configure(TxType txType) -> void
{
    // Configure GPIO pins, NIRQ, and SDO
    // Don't change
    static constexpr auto gpio0Config = 0x41_b;
    // Don't change
    static constexpr auto gpio1Config = 0x41_b;
    // GPIO2 active in RX state
    static constexpr auto gpio2Config = 0x21_b;
    // GPIO3 active in TX state
    static constexpr auto gpio3Config = 0x20_b;
    // NIRQ is still used as NIRQ
    static constexpr auto nirqConfig = 0x27_b;
    // SDO is still used as SDO
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
    static constexpr auto intCtlEnable = 0x01_b;
    SetProperties(PropertyGroup::intCtl, iIntCtlEnable, Span(intCtlEnable));

    // Preamble
    static constexpr auto iPreambleTxLength = 0x00_b;
    // 0 bytes preamble
    static constexpr auto preambleTxLength = 0x08_b;
    // Normal sync timeout, 14-bit preamble RX threshold
    static constexpr auto preambleConfigStd1 = 0x14_b;
    // No non-standard preamble pattern
    //
    // TODO: Maybe we can detect RS+CC encoded preamble this way and be CCSDS compliant on uplink
    // too? Problem: Max pattern length is 32 bit
    static constexpr auto preambleConfigNstd = 0x00_b;
    // No extended RX preamble timeout, 0x0F nibbles timeout until detected preamble is discarded as
    // invalid
    static constexpr auto preambleConfigStd2 = 0x0F_b;
    // First transmitted preamble bit is 1, unit of preamble TX length is in bytes
    static constexpr auto preambleConfig = 0x31_b;
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
    // Allow 4-bit sync word errors, 4-byte sync word
    static constexpr auto syncConfig = 0x43_b;
    // Valid CCSDS TM sync word for Reed-Solomon or convolutional coding. Be careful: Send order is
    // MSB-first but little endian so the lowest bit of the highest byte is transmitted first, which
    // is different to how the CCSDS spec annotates those bit patterns!
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

    // Whitening and packet parameters
    static constexpr auto iPktWhtBitNum = 0x05_b;
    // Disable whitening
    static constexpr auto pktWhtBitNum = 0x00_b;
    // Don't split RX and TX field information (length, ...), enable RX packet handler, use normal
    // (2)FSK, no Manchester coding, no CRC, data transmission with MSB first
    static constexpr auto pktConfig1 = 0x00_b;
    SetProperties(PropertyGroup::pkt, iPktWhtBitNum, Span({pktWhtBitNum, pktConfig1}));

    // Packet length part 1
    static constexpr auto iPktLen = 0x08_b;
    // Infinite receive, big endian (MSB first)
    static constexpr auto pktLen = 0x60_b;
    static constexpr auto pktLenFieldSource = 0x00_b;
    static constexpr auto pktLenAdjust = 0x00_b;
    static constexpr auto pktTxThreshold = txFifoThreshold;
    static constexpr auto pktRxThreshold = rxFifoThreshold;
    static constexpr auto pktField1Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktField1Config = 0x04_b;
    static constexpr auto pktField1CrcConfig = 0x80_b;
    static constexpr auto pktField2Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktField2Config = 0x00_b;
    SetProperties(PropertyGroup::pkt,
                  iPktLen,
                  Span(FlatArray(pktLen,
                                 pktLenFieldSource,
                                 pktLenAdjust,
                                 pktTxThreshold,
                                 pktRxThreshold,
                                 pktField1Length,
                                 pktField1Config,
                                 pktField1CrcConfig,
                                 pktField2Length,
                                 pktField2Config)));

    // Packet length part 2
    static constexpr auto iPktField2CrcConfig = 0x14_b;
    static constexpr auto pktField2CrcConfig = 0x00_b;
    static constexpr auto pktField3Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktField3Config = 0x00_b;
    static constexpr auto pktField3CrcConfig = 0x00_b;
    static constexpr auto pktField4Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktField4Config = 0x00_b;
    static constexpr auto pktField4CrcConfig = 0x00_b;
    static constexpr auto pktField5Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktField5Config = 0x00_b;
    SetProperties(PropertyGroup::pkt,
                  iPktField2CrcConfig,
                  Span(FlatArray(pktField2CrcConfig,
                                 pktField3Length,
                                 pktField3Config,
                                 pktField3CrcConfig,
                                 pktField4Length,
                                 pktField4Config,
                                 pktField4CrcConfig,
                                 pktField5Length,
                                 pktField5Config)));

    // Packet length part 3
    static constexpr auto iPktField5CrcConfig = 0x20_b;
    static constexpr auto pktField5CrcConfig = 0x00_b;
    static constexpr auto pktRxField1Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktRxField1Config = 0x00_b;
    static constexpr auto pktRxField1CrcConfig = 0x00_b;
    static constexpr auto pktRxField2Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktRxField2Config = 0x00_b;
    static constexpr auto pktRxField2CrcConfig = 0x00_b;
    static constexpr auto pktRxField3Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktRxField3Config = 0x00_b;
    SetProperties(PropertyGroup::pkt,
                  iPktField5CrcConfig,
                  Span(FlatArray(pktField5CrcConfig,
                                 pktRxField1Length,
                                 pktRxField1Config,
                                 pktRxField1CrcConfig,
                                 pktRxField2Length,
                                 pktRxField2Config,
                                 pktRxField2CrcConfig,
                                 pktRxField3Length,
                                 pktRxField3Config)));

    // Packet length part 4
    static constexpr auto iPktRxField3CrcConfig = 0x2C_b;
    static constexpr auto pktRxField3CrcConfig = 0x00_b;
    static constexpr auto pktRxField4Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktRxField4Config = 0x00_b;
    static constexpr auto pktRxField4CrcConfig = 0x00_b;
    static constexpr auto pktRxField5Length = std::array{0x00_b, 0x00_b};
    static constexpr auto pktRxField5Config = 0x00_b;
    static constexpr auto pktRxField5CrcConfig = 0x00_b;
    SetProperties(PropertyGroup::pkt,
                  iPktRxField3CrcConfig,
                  Span(FlatArray(pktRxField3CrcConfig,
                                 pktRxField4Length,
                                 pktRxField4Config,
                                 pktRxField4CrcConfig,
                                 pktRxField5Length,
                                 pktRxField5Config,
                                 pktRxField5CrcConfig)));

    // RF modem mod type
    SetTxType(txType);
    // SetTxType sets modem properties from 0x00 to 0x05
    static constexpr auto iModemTxNcoMode = 0x06_b;
    // TXOSR = x10 = 0, NCOMOD = F_XTAL / 10 = 2600000 = 0x027ac40
    static constexpr auto modemTxNcoMode = std::array{0x00_b, 0x27_b, 0xAC_b, 0x40_b};
    // We use minimum shift keying, i.e., a frequency deviation of baudrate / 4. The value we need
    // to write to the property is (2^19 * outdiv * deviation_Hz) / (N_presc * F_xo) = (2^19 * 8 *
    // (9600 / 4)) / (2 * 26000000) = 194 = 0x0000C2
    // 0x308 = 4 * 194
    static constexpr auto modemFreqDeviation = std::array{0x00_b, 0x0C_b, 0x20_b};
    SetProperties(
        PropertyGroup::modem, iModemTxNcoMode, Span(FlatArray(modemTxNcoMode, modemFreqDeviation)));

    // RF modem TX ramp delay, modem MDM control, modem IF control, modem IF frequency & modem
    // decimation
    static constexpr auto iModemTxRampDelay = 0x18_b;
    // Ramp delay 1
    static constexpr auto modemTxRampDelay = 0x01_b;
    // Slicer phase source from detector's output
    static constexpr auto modemMdmCtrl = 0x80_b;
    // No ETSI mode, fixed IF mode, normal IF mode (nonzero IF)
    static constexpr auto modemIfControl = 0x08_b;
    // IF = (2^19 * outdiv * IF_Freq_Hz) / (npresc * freq_xo) = (2^19 * 8 * xxx) / (2 * 26000000) =
    // 0x03C000 (default value)
    //
    // TODO: Is it important what we chose here?
    static constexpr auto modemIfFreq = std::array{0x03_b, 0xC0_b, 0x00_b};
    // Decimation NDEC0 = 0, NDEC1 = decimation by 8, NDEC2 = decimation by 2
    static constexpr auto modemDecimationCfg1 = 0x70_b;
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
    // RX symbol oversampling rate of 0x30D / 8 = 781 / 8 = 97.625 (According to the datasheet usual
    // values are in the range of 8 to 12 where this value seems to be odd?)
    static constexpr auto modemBcrOsr = std::array{0x03_b, 0x0D_b};
    // BCR NCO offset of 0x00A7C6 / 64 = 42950 / 64 = 671.09375
    static constexpr auto modemBcrNcoOffset = std::array{0x00_b, 0xA7_b, 0xC6_b};
    // BCR gain 0x054 = 84
    static constexpr auto modemBcrGain = std::array{0x00_b, 0x54_b};
    // BCR loop gear control, CRSLOW=2, CRFAST=0
    static constexpr auto modemBcrGear = 0x02_b;
    // Stop NCO for one sample clock in BCR mid-point phase sampling condition to escape, disable
    // NCO resetting in case of mid-point phase sampling condition, don't double BCR loop gain, BCR
    // NCO compensation is sampled upon detection of the preamble end, disable NCO frequency
    // compensation, bypass compensation term feedback to slicer, bypass compensation term feedback
    // to BCR tracking loop
    static constexpr auto modemBcrMisc1 = 0xC2_b;
    SetProperties(
        PropertyGroup::modem,
        iModemBcrOsr,
        Span(FlatArray(modemBcrOsr, modemBcrNcoOffset, modemBcrGain, modemBcrGear, modemBcrMisc1)));

    // RF modem AFC gear, modem AFC wait, modem AFC gain, modem AFC limiter & modem AFC misc
    //
    // TODO: What values to use here?
    static constexpr auto iModemAfcGear = 0x2C_b;
    // AFC_SLOW gain 4, AFC_FAST gain 0, switch gear after detection of preamble
    static constexpr auto modemAfcGear = 0x04_b;
    // LGWAIT = 6, SHWAIT = 3
    static constexpr auto modemAfcWait = 0x36_b;
    // AFC loop gain = 0x003, don't half the loop gain, disable adaptive RX bandwidth, enable
    // frequency error estimation
    static constexpr auto modemAfcGain = std::array{0x80_b, 0x03_b};
    // 0x30AF
    static constexpr auto modemAfcLimiter = std::array{0x30_b, 0xAF_b};
    // Expected frequency error is less then 12 * symbol rate, AFC correction of PLL will be frozen
    // if a consecutive string of 1 s or 0 s that exceed the search period is encountered, don't
    // switch clock source for frequency estimator, don't freeze AFC at preamble end, AFC correction
    // uses frequency estimation by moving average or minmax detector in async demod, disable AFC
    // value feedback to PLL, freeze AFC after gear switching
    static constexpr auto modemAfcMisc = 0x80_b;
    SetProperties(
        PropertyGroup::modem,
        iModemAfcGear,
        Span(FlatArray(modemAfcGear, modemAfcWait, modemAfcGain, modemAfcLimiter, modemAfcMisc)));

    // RF modem AGC control
    //
    // TODO: What values to use here?
    static constexpr auto iModemAgcControl = 0x35_b;
    // Reset peak detectors only on change of gain indicated by peak detector output, reduce ADC
    // gain when AGC gain is at minimum, normal AGC speed, don't increase AGC gain during signal
    // reductions in ant diversity mode, always perform gain decreases in 3 dB steps instead of 6 dB
    // steps, AGC is enabled over whole packet length
    static constexpr auto modemAgcControl = 0xE2_b;
    SetProperties(PropertyGroup::modem, iModemAgcControl, Span({modemAgcControl}));

    // RF modem AGC window size, AGC RF peak detector decay, AGC IF peak detector decay, 4FSK gain,
    // 4FSK slicer threshold, 4FSK symbol mapping code, OOK attack/decay times
    //
    // TODO: What values to use here?
    static constexpr auto iModemAgcWindowSize = 0x38_b;
    // AGC gain settling window size = 1, AGC signal level measurement window = 1
    static constexpr auto modemAgcWindowSize = 0x11_b;
    // RF peak detector decay time = 0xAB = 171
    static constexpr auto modemAgcRfpdDecay = 0xAB_b;
    // IF peak detector decay time = 0xAB = 171
    static constexpr auto modemAgcIfpdDecay = 0xAB_b;
    // 4FSK Gain1 = 0, Normal second phase compensation factor
    static constexpr auto modemFsk4Gain1 = 0x00_b;
    // 4FSK Gain0 = 2, disable 2FSK phase compensation
    static constexpr auto modemFsk4Gain0 = 0x02_b;
    // 4FSK slicer threshold = 0xFFFF
    static constexpr auto modemFsk4Th = std::array{0xFF_b, 0xFF_b};
    // 4FSK symbol map 0 (`00 `01 `11 `10)
    static constexpr auto modemFsk4Map = 0x00_b;
    // OOK decay = 11, OOK attack = 2
    static constexpr auto modemOokPdtc = 0x2B_b;
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
    // OOK squelch off, OOK slicer output de-glitching by bit clock, raw output is synced to clock,
    // MA_FREQUDOWN = 0, AGC and OOK moving average detector threshold will be frozen after preamble
    // detection, S2P_MAP = 2
    static constexpr auto modemOokCnt1 = 0xA4_b;
    // OOK uses moving average detector, OOK peak detector discharge does not affect decay rate,
    // disable OOK squelch, always discharge peak detector, normal moving average window
    static constexpr auto modemOokMisc = 0x02_b;
    static constexpr auto modemRawControl = 0x83_b;
    // RAW eye open detector threshold
    static constexpr auto modemRawEye = std::array{0x00_b, 0xAD_b};
    // Antenna diversity mode
    static constexpr auto modemAntDivMode = 0x01_b;
    // Antenna diversity control
    static constexpr auto modemAntDivControl = 0x80_b;
    // Threshold for clear channel assessment and RSSI interrupt generation
    static constexpr auto modemRssiThresh = 0xFF_b;
    SetProperties(
        PropertyGroup::modem,
        iModemOokCnt1,
        Span(FlatArray(
            modemOokCnt1,
            modemOokMisc,
            // NOLINTNEXTLINE(*magic-numbers)
            0xD6_b,  // TODO: index 0x44 is not described in the API, what does this value do?
            modemRawControl,
            modemRawEye,
            modemAntDivMode,
            modemAntDivControl,
            modemRssiThresh)));

    // RF modem RSSI control
    static constexpr auto iModemRssiControl = 0x4C_b;
    // Disable RSSI latch, RSSI value is avg over last 4 * Tb bit periods, disable RSSI threshold
    // check after latch
    static constexpr auto modemRssiControl = 0x00_b;
    SetProperties(PropertyGroup::modem, iModemRssiControl, Span({modemRssiControl}));

    // RF modem RSSI compensation
    //
    // TODO: Measure this
    static constexpr auto iModemRssiComp = 0x4E_b;
    // Compensation/offset of measured RSSI value
    static constexpr auto modemRssiComp = 0x40_b;
    SetProperties(PropertyGroup::modem, iModemRssiComp, Span({modemRssiComp}));

    // RF modem clock generation band
    static constexpr auto iModemClkgenBand = 0x51_b;
    // Band = FVCO_DIV_8, high performance mode fixed prescaler div2, force recalibration
    static constexpr auto modemClkgenBand = 0x0A_b;
    SetProperties(PropertyGroup::modem, iModemClkgenBand, Span({modemClkgenBand}));

    // RX filter coefficients
    //
    // TODO: What values to use here?
    //
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
        0x15_b,  // RX2_CHFLT_COE10[9:8]  | RX2_CHFLT_COE11[9:8]  |
                 // RX2_CHFLT_COE12[9:8]  | RX2_CHFLT_COE13[9:8]
        0xFF_b,  // RX2_CHFLT_COE6[9:8]   | RX2_CHFLT_COE7[9:8]   | RX2_CHFLT_COE8[9:8] |
                 // RX2_CHFLT_COE9[9:8]
        0x00_b,  // RX2_CHFLT_COE2[9:8]   | RX2_CHFLT_COE3[9:8]   | RX2_CHFLT_COE4[9:8] |
                 // RX2_CHFLT_COE5[9:8]
        0x00_b   // 0 | 0 | 0 | 0         | RX2_CHFLT_COE0[9:8]   | RX2_CHFLT_COE1[9:8] |
    };
    SetProperties(
        PropertyGroup::modemChflt, iRxFilterCoefficientsBlock3, Span(rxFilterCoefficientsBlock3));

    // RF PA mode
    static constexpr auto iPaMode = 0x00_b;
    // PA switching amp mode, PA_SEL = HP_COARSE, disable power sequencing, disable external TX ramp
    // signal
    static constexpr auto paMode = 0x08_b;
    // Enabled PA fingers (sets output power but not linearly; 10 µA bias current per enabled
    // finger, complementary drive signal with 50 % duty cycle)
    static constexpr auto paPwrLvl = 0x18_b;
    static constexpr auto paBiasClkduty = 0x00_b;
    // Ramping time constant = 0x1B (~10 µs to full - 0.5 dB), FSK modulation delay 10 µs
    static constexpr auto paTc = 0x91_b;
    SetProperties(PropertyGroup::pa, iPaMode, Span({paMode, paPwrLvl, paBiasClkduty, paTc}));

    // RF synth feed forward charge pump current, integrated charge pump current, VCO gain scaling
    // factor, FF loop filter values
    //
    // TODO: What values to use here?
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
    // RF_channel_Hz = (FC_inte + FC_frac / 2^19) * ((N_presc * F_xo) / outdiv) = 433.5000048MHz MHz
    static constexpr auto freqControlFrac = std::array{0x0E_b, 0xC4_b, 0xEC_b};
    // Channel step size = 0x4444
    static constexpr auto freqControlChannelStepSize = std::array{0x44_b, 0x44_b};
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
    static constexpr auto globalXoTuneUpdated = 0x62_b;
    SetProperties(PropertyGroup::global, iGlobalXoTune, Span({globalXoTuneUpdated}));

    // Change sequencer mode to guaranteed
    //
    // TODO: Why?
    //
    // Split FIFO and guaranteed sequencer mode
    static constexpr auto newGlobalConfig = 0x40_b;
    SetProperties(PropertyGroup::global, iGlobalConfig, Span({newGlobalConfig}));
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
}

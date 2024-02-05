#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstddef>
#include <iterator>
#include <span>
#include <type_traits>


namespace sts1cobcsw::rf
{
using RODOS::AT;
using RODOS::MICROSECONDS;
using RODOS::MILLISECONDS;
using RODOS::NOW;


enum class PowerUpBootOptions : std::uint8_t
{
    noPatch = 0x01,
    patch = 0x81
};


enum class PowerUpXtalOptions : std::uint8_t
{
    xtal = 0x00,  // Reference signal is derived from the internal crystal oscillator
    txco = 0x01   // Reference signal is derived from an external TCXO
};


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

constexpr std::uint32_t powerUpXoFrequency = 26'000'000;  // 26 MHz

// Si4463 commands
constexpr auto cmdPartInfo = 0x01_b;
constexpr auto cmdPowerUp = 0x02_b;
constexpr auto cmdSetProperty = 0x11_b;
constexpr auto cmdGpioPinCfg = 0x13_b;
constexpr auto cmdReadCmdBuff = 0x44_b;

// Command answer lengths
// TODO: We should do it similarly to SimpleInstruction and SendInstruction() in Flash.cpp. Unless
// this remains the only command with an answer. Then we should probably get rid of SendCommand<>()
// instead.
constexpr auto partInfoAnswerLength = 8U;

// Max. number of properties that can be set in a single command
constexpr auto maxNProperties = 12;
constexpr auto setPropertiesHeaderSize = 4;

auto spi = RODOS::HAL_SPI(
    hal::rfSpiIndex, hal::rfSpiSckPin, hal::rfSpiMisoPin, hal::rfSpiMosiPin, hal::spiNssDummyPin);
auto csGpioPin = hal::GpioPin(hal::rfCsPin);
auto nirqGpioPin = hal::GpioPin(hal::rfNirqPin);
auto sdnGpioPin = hal::GpioPin(hal::rfSdnPin);
auto gpio0GpioPin = hal::GpioPin(hal::rfGpio0Pin);
auto gpio1GpioPin = hal::GpioPin(hal::rfGpio1Pin);
auto paEnablePin = hal::GpioPin(hal::rfPaEnablePin);

// TODO: This should probably be somewhere else as it is not directly related to the RF module
auto watchdogResetGpioPin = hal::GpioPin(hal::watchdogClearPin);

// Pause values for pin setting/resetting and PoR
// Jakob: Pause times are VERY generously overestimated
// TODO: Patrick: Are those delays really necessary? I have never seen something like that for SPI
// communication
// TODO: Use delay instead of pause, because that's how we did it everywhere else
// TODO: Do not use trailing comments since they cause line breaks
constexpr auto csPinAfterResetPause =
    20 * MICROSECONDS;  // Pause time after pulling NSEL (here CS) low
constexpr auto csPinPreSetPause =
    2 * MICROSECONDS;  // Pause time before pulling NSEL (here CS) high
constexpr auto porRunningPause =
    20 * MILLISECONDS;  // Pause time to wait for Power on Reset to finish
constexpr auto porCircuitSettlePause =
    100 * MILLISECONDS;  // Time until PoR circuit settles after applying power
constexpr auto initialWaitForCtsDelay =
    20 * MICROSECONDS;  // Pause time at the beginning of the CTS wait loop
constexpr auto watchDogResetPinPause =
    1 * MILLISECONDS;  // Pause time for the sequence reset -> pause -> set -> pause -> reset in
                       // initialization


// --- Private function declarations ---

auto InitializeGpioAndSpi() -> void;
auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void;

[[deprecated]] auto SendCommand(std::uint8_t * data,
                                std::size_t length,
                                std::uint8_t * responseData,
                                std::size_t responseLength) -> void;
auto SendCommand(std::span<Byte const> data) -> void;
template<std::size_t answerLength>
auto SendCommand(std::span<Byte const> data) -> std::array<Byte, answerLength>;

auto WaitForCts() -> void;
auto SetTxType(TxType txType) -> void;
auto SetProperties(PropertyGroup propertyGroup,
                   Byte startProperty,
                   std::span<Byte const> propertyValues) -> void;


// --- Public function definitions ---

// TODO: Get rid of all the magic numbers
// TODO: Replace all C-style arrays with std::array

auto Initialize(TxType txType) -> void
{
    // TODO: Don't forget that WDT_Clear has to be triggered regularely for the TX to work! (even
    // without the watchdog timer on the PCB it needs to be triggered at least once after boot to
    // enable the TX)

    InitializeGpioAndSpi();

    auto sendBuffer = std::array<std::uint8_t, 32>{};

    PowerUp(PowerUpBootOptions::noPatch, PowerUpXtalOptions::xtal, powerUpXoFrequency);

    // GPIO Pin Cfg
    SendCommand(Span({cmdGpioPinCfg, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b}));

    // Global XO Tune 2
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x02;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x52;  // GLOBAL_XO_TUNE
    sendBuffer[5] = 0x00;  // GLOBAL_CLK_CFG
    SendCommand(data(sendBuffer), 6, nullptr, 0);

    // RF Global Config 1
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x03;
    sendBuffer[4] = 0x60;  // GLOBAL_CONFIG: High performance mode, Generic packet format, Split
                           // FiFo mode, Fast sequencer mode
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Int Ctl Enable
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x01;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x01;  // INT_CTL: Enable packet handler interrupts
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // TX Preamble Length
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x10;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x00;   // PREAMBLE_TX_LENGTH: 0 bytes preamble
    sendBuffer[5] = 0x14;   // PREAMBLE_CONFIG_STD_1: Normal sync timeout,
                            // 14 bit preamble RX threshold
    sendBuffer[6] = 0x00;   // PREAMBLE_CONFIG_NSTD: No non-standard preamble pattern TODO: Maybe we
                            // can detect RS+CC encoded preamble this way and be CCSDS compliant on
                            // uplink too? Problem: Max pattern length is 32 bit
    sendBuffer[7] = 0x0F;   // PREAMBLE_CONFIG_STD_2: No extended RX preamble timeout, 0x0f nibbles
                            // timeout until detected preamble is discarded as invalid
    sendBuffer[8] = 0x31;   // PREAMBLE_CONFIG: First transmitted preamble bit is 1, unit of
                            // preampreamble TX length is in bytes
    sendBuffer[9] = 0x00;   // PREAMBLE_PATTERN: Non-standard pattern
    sendBuffer[10] = 0x00;  // Non-standard pattern
    sendBuffer[11] = 0x00;  // Non-standard pattern
    sendBuffer[12] = 0x00;  // Non-standard pattern
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // Sync word config
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x11;
    sendBuffer[2] = 0x05;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x43;        // SYNC_CONFIG: Allow 4 bit sync word errors, 4 byte sync word
    sendBuffer[5] = 0b01011000;  // SYNC_BITS: Valid CCSDS TM sync word for
                                 // Reed-Solomon or convolutional coding
    sendBuffer[6] = 0b11110011;  // Be careful: Send order is MSB-first but Little endian so the
                                 // lowest bit of the
    sendBuffer[7] = 0b00111111;  // highest byte is transmitted first,
                                 // which is different to how the CCSDS spec
    sendBuffer[8] = 0b10111000;  // annotates those bit patterns!
    // TODO: Check that pattern!
    SendCommand(data(sendBuffer), 9, nullptr, 0);

    // CRC Config
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x00;  // PKT_CRC_CONFIG: No CRC
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // Whitening and Packet Parameters
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x02;
    sendBuffer[3] = 0x05;
    sendBuffer[4] = 0x00;  // PKT_WHT_BIT_NUM: Disable whitening
    sendBuffer[5] = 0x01;  // PKT_CONFIG1: Don't split RX and TX field information (length, ...),
                           // enable RX packet handler, use normal (2)FSK, no Manchester coding, no
                           // CRC, data transmission with MSB first.
    SendCommand(data(sendBuffer), 6, nullptr, 0);

    // Pkt Length part 1
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x08;
    sendBuffer[4] = 0x60;  // PKT_LEN: Infinite receive, big endian (MSB first)
    sendBuffer[5] = 0x00;  // PKT_LEN_FIELD_SOURCE
    sendBuffer[6] = 0x00;  // PKT_LEN_ADJUST
    sendBuffer[7] = 0x30;  // PKT_TX_THRESHOLD: Trigger TX FiFo almost empty interrupt when 0x30
                           // bytes in FiFo (size 0x40) are empty
    sendBuffer[8] = 0x30;  // PKT_RX_THRESHOLD: Trigger RX FiFo almost full interrupt when 0x30
                           // bytes in FiFo (size 0x40) are full
    sendBuffer[9] = 0x00;  // PKT_FIELD_1_LENGTH
    sendBuffer[10] = 0x00;
    sendBuffer[11] = 0x04;  // PKT_FIELD_1_CONFIG
    sendBuffer[12] = 0x80;  // PKT_FIELD_1_CRC_CONFIG
    sendBuffer[13] = 0x00;  // PKT_FIELD_2_LENGTH
    sendBuffer[14] = 0x00;
    sendBuffer[15] = 0x00;  // PKT_FIELD_2_CONFIG
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // Pkt Length part 2
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x14;
    sendBuffer[4] = 0x00;  // PKT_FIELD_2_CRC_CONFIG
    sendBuffer[5] = 0x00;  // PKT_FIELD_3_LENGTH
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;  // PKT_FIELD_3_CONFIG
    sendBuffer[8] = 0x00;  // PKT_FIELD_3_CRC_CONFIG
    sendBuffer[9] = 0x00;  // PKT_FIELD_4_LENGTH
    sendBuffer[10] = 0x00;
    sendBuffer[11] = 0x00;  // PKT_FIELD_4_CONFIG
    sendBuffer[12] = 0x00;  // PKT_FIELD_4_CRC_CONFIG
    sendBuffer[13] = 0x00;  // PKT_FIELD_5_LENGTH
    sendBuffer[14] = 0x00;
    sendBuffer[15] = 0x00;  // PKT_FIELD_5_CONFIG
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // Pkt Length part 3
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x20;
    sendBuffer[4] = 0x00;  // PKT_FIELD_5_CRC_CONFIG
    sendBuffer[5] = 0x00;  // PKT_RX_FIELD_1_LENGTH
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;  // PKT_RX_FIELD_1_CONFIG
    sendBuffer[8] = 0x00;  // PKT_RX_FIELD_1_CRC_CONFIG
    sendBuffer[9] = 0x00;  // PKT_RX_FIELD_2_LENGTH
    sendBuffer[10] = 0x00;
    sendBuffer[11] = 0x00;  // PKT_RX_FIELD_2_CONFIG
    sendBuffer[12] = 0x00;  // PKT_RX_FIELD_2_CRC_CONFIG
    sendBuffer[13] = 0x00;  // PKT_RX_FIELD_3_LENGTH
    sendBuffer[14] = 0x00;
    sendBuffer[15] = 0x00;  // PKT_RX_FIELD_3_CONFIG
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // Pkt Length part 4
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x2C;
    sendBuffer[4] = 0x00;  // PKT_RX_FIELD_3_CRC_CONFIG
    sendBuffer[5] = 0x00;  // PKT_RX_FIELD_4_LENGTH
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;  // PKT_RX_FIELD_4_CONFIG
    sendBuffer[8] = 0x00;  // PKT_RX_FIELD_4_CRC_CONFIG
    sendBuffer[9] = 0x00;  // PKT_RX_FIELD_5_LENGTH
    sendBuffer[10] = 0x00;
    sendBuffer[11] = 0x00;  // PKT_RX_FIELD_5_CONFIG
    sendBuffer[12] = 0x00;  // PKT_RX_FIELD_5_CRC_CONFIG
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // RF Modem Mod Type
    SetTxType(txType);
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x07;
    sendBuffer[3] = 0x06;  // SetTxType sets modem properties from 0x00 to 0x05
    sendBuffer[4] = 0x00;  // MODEM_TX_NCO_MODE: TXOSR=x10=0, NCOMOD=F_XTAL/10=2600000=0x027ac40
    sendBuffer[5] = 0x27;
    sendBuffer[6] = 0xAC;
    sendBuffer[7] = 0x40;
    sendBuffer[8] = 0x00;  // MODEM_FREQ_DEVIATION: (2^19 * outdiv * deviation_Hz)/(N_presc * F_xo)
                           // = (2^19 * 8 * 9600/4)/(2 * 26000000) = 194 = 0x0000C2
    sendBuffer[9] = 0x00;
    sendBuffer[10] = 0xC2;
    SendCommand(data(sendBuffer), 11, nullptr, 0);


    // RF Modem TX Ramp Delay, Modem MDM Ctrl, Modem IF Ctrl, Modem IF Freq & Modem Decimation Cfg
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x08;
    sendBuffer[3] = 0x18;
    sendBuffer[4] = 0x01;  // MODEM_TX_RAMP_DELAY: Ramp Delay 1
    sendBuffer[5] = 0x80;  // MODEM_MDM_CTRL: Slicer phase source from detector's output
    sendBuffer[6] = 0x08;  // MODEM_IF_CONTROL: No ETSI mode, fixed IF mode,
                           // normal IF mode (nonzero IF)
    sendBuffer[7] = 0x03;  // MODEM_IF_FREQ: IF = (2^19 * outdiv * IF_Freq_Hz)/(npresc * freq_xo) =
                           // (2^19 * 8 * xxx)/(2 * 26000000) = 0x03C000 (default value)
                           // TODO: Is it important what we chose here?
    sendBuffer[8] = 0xC0;
    sendBuffer[9] = 0x00;
    sendBuffer[10] = 0x70;  // MODEM_DECIMATION_CFG1: Decimation NDEC0 = 0, NDEC1 = decimation by 8,
                            // NDEC2 = decimation by 2
    sendBuffer[11] = 0x20;  // MODEM_DECIMATION_CFG0: Normal decimate-by-8 filter gain,
                            // don't bypass the
    // decimate-by-2 polyphase filter, bypass the decimate-by-3 polyphase filter, enable
    // droop compensation, channel selection filter in normal mode (27 tap filter)
    SendCommand(data(sendBuffer), 12, nullptr, 0);

    // RF Modem BCR Oversampling Rate, Modem BCR NCO Offset, Modem BCR Gain, Modem BCR Gear & Modem
    // BCR Misc
    // TODO: What values to use here?
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x22;
    sendBuffer[4] = 0x03;  // MODEM_BCR_OSR: RX symbol oversampling rate of 0x30D/8 = 781/8 = 97.625
                           // (According to the datasheet usual values are in the range of 8 to 12
                           // where this value seems to be odd?)
    sendBuffer[5] = 0x0D;
    sendBuffer[6] = 0x00;  // MODEM_BCR_NCO_OFFSET: BCR NCO offset of
                           // 0x00A7C6/64 = 42950/64 = 671.09375
    sendBuffer[7] = 0xA7;
    sendBuffer[8] = 0xC6;
    sendBuffer[9] = 0x00;  // MODEM_BCR_GAIN: BCR gain 0x054 = 84
    sendBuffer[10] = 0x54;
    sendBuffer[11] = 0x02;  // MODEM_BCR_GEAR: BCR loop gear control. CRSLOW=2, CRFAST=0
    sendBuffer[12] = 0xC2;  // MODEM_BCR_MISC1: Stop NCO for one sample clock in BCR mid-point phase
                            // sampling condition to escape, disable NCO resetting in case of
                            // mid-point phase sampling condition, don't double BCR loop gain, BCR
                            // NCO compensation is sampled upon detection of the preamble end,
                            // disable NCO frequency compensation, bypass compensation term feedback
                            // to slicer, bypass compensation term feedback to BCR tracking loop
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // RF Modem AFC Gear, Modem AFC Wait, Modem AFC Gain, Modem AFC Limiter & Modem AFC Misc
    // TODO: What values to use here?
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x07;
    sendBuffer[3] = 0x2C;
    sendBuffer[4] = 0x04;  // MODEM_AFC_GEAR: AFC_SLOW gain 4, AFC_FAST gain 0, Switch gear after
                           // detection of preamble
    sendBuffer[5] = 0x36;  // MODEM_AFC_WAIT: LGWAIT = 6, SHWAIT = 3
    sendBuffer[6] = 0x80;  // MODEM_AFC_GAIN: AFC loop gain = 0x003, don't half the loop gain,
                           // disable adaptive RX bandwidth, enable frequency error estimation
    sendBuffer[7] = 0x03;
    sendBuffer[8] = 0x30;  // MODEM_AFC_LIMITER: 0x30AF
    sendBuffer[9] = 0xAF;
    sendBuffer[10] =
        0x80;  // MODEM_AFC_MISC: Expected freq error is less then 12*symbol rate, AFC correction of
               // PLL will be frozen if a consecutive string of 1s or 0s that exceed the search
               // period is encountered, don't switch clock source for frequency estimator, don't
               // freeze AFC at preamble end, AFC correction uses freq estimation by moving average
               // or minmax detector in async demod,disable AFC value feedback to PLL, freeze AFC
               // after gear switching
    SendCommand(data(sendBuffer), 11, nullptr, 0);

    // RF Modem AGC Control
    // TODO: What values to use here?
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x35;
    sendBuffer[4] = 0xE2;  // MODEM_AGC_CONTROL: reset peak detectors only on change of gain
                           // indicated by peak detector output, reduce ADC gain when AGC gain is at
                           // minimum, normal AGC speed, don't increase AGC gain during signal
                           // reductions in ant diversity mode, always perform gain decreases in 3dB
                           // steps instead of 6dB steps, AGC is enabled over whole packet length
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Modem AGC Window Size, AGC RF Peak Detector Decay, AGC IF Peak Detector Decay, 4FSK Gain,
    // 4FSK Slicer Threshold, 4FSK SYmbol Mapping Code, OOK Attack/Decay Times
    // TODO: What values to use here?
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x38;
    sendBuffer[4] = 0x11;  // MODEM_AGC_WINDOW_SIZE: AGC gain settling window size = 1, AGC signal
                           // level measurement window = 1
    sendBuffer[5] = 0xAB;  // MODEM_AGC_RFPD_DECAY: RF peak detector decay time = 0xAB = 171
    sendBuffer[6] = 0xAB;  // MODEM_AGC_IFPD_DECAY: IF peak detector decay time = 0xAB = 171
    sendBuffer[7] = 0x00;  // MODEM_FSK4_GAIN1: 4FSK Gain1 = 0,
                           // Normal second phase compensation factor
    sendBuffer[8] = 0x02;  // MODEM_FSK4_GAIN0: 4FSK Gain0 = 2, disable 2FSK phase compensation
    sendBuffer[9] = 0xFF;  // MODEM_FSK4_TH: 4FSK slicer threshold = 0xFFFF
    sendBuffer[10] = 0xFF;
    sendBuffer[11] = 0x00;  // MODEM_FSK4_MAP: 4FSK symbol map 0 (`00 `01 `11 `10)
    sendBuffer[12] = 0x2B;  // MODEM_OOK_PDTC: OOK decay = 11, OOK attack = 2
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // RF Modem OOK Control, OOK Misc, RAW Search, RAW Control, RAW Eye, Antenna Diversity Mode,
    // Antenna Diversity Control, RSSI Threshold
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x42;
    sendBuffer[4] =
        0xA4;  // MODEM_OOK_CNT1: OOK Squelch off, OOK slicer output de-glitching by bit clock, raw
               // output is synced to clock, MA_FREQUDOWN=0, AGC and OOK movign average detector
               // threshold will be frozen after preamble detection, S2P_MAP=2
    sendBuffer[5] = 0x02;  // MODEM_OOK_MISC: OOK uses moving average detector, OOK peak detector
                           // discharge does not affect decay rate, disable OOK squelch, always
                           // discharge peak detector, normal moving average window
    sendBuffer[6] = 0xD6;  // ??
    sendBuffer[7] = 0x83;  // MODEM_RAW_CONTROL
    sendBuffer[8] = 0x00;  // MODEM_RAW_EYE: RAW eye open detector threshold
    sendBuffer[9] = 0xAD;
    sendBuffer[10] = 0x01;  // MODEM_ANT_DIV_MODE: Antenna diversity mode
    sendBuffer[11] = 0x80;  // MODEM_ANT_DIV_CONTROL: Antenna diversity control
    sendBuffer[12] = 0xFF;  // MODEM_RSSI_THRESH: Threshold for clear channel assessment and RSSI
                            // interrupt generation
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // RF Modem RSSI Control
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x4C;
    sendBuffer[4] = 0x00;  // MODEM_RSSI_CONTROL: Disable RSSI latch, RSSI value is avg over last
                           // 4*Tb bit periods, disable RSSI threshold check after latch
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Modem RSSI Compensation
    // TODO: Measure this
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x4E;
    sendBuffer[4] = 0x40;  // MODEM_RSSI_COMP: Compensation/offset of measured RSSI value
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Modem Clock generation Band
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x51;
    sendBuffer[4] = 0x0A;  // MODEM_CLKGEN_BAND: Band = FVCO_DIV_8, high performance mode fixed
                           // prescaler div2, force recalibration
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RX Filter Coefficients
    // TODO: What values to use here?
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x21;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0xFF;
    sendBuffer[5] = 0xC4;
    sendBuffer[6] = 0x30;
    sendBuffer[7] = 0x7F;
    sendBuffer[8] = 0xF5;
    sendBuffer[9] = 0xB5;
    sendBuffer[10] = 0xB8;
    sendBuffer[11] = 0xDE;
    sendBuffer[12] = 0x05;
    sendBuffer[13] = 0x17;
    sendBuffer[14] = 0x16;
    sendBuffer[15] = 0x0C;
    SendCommand(data(sendBuffer), 16, nullptr, 0);
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x21;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x0C;
    sendBuffer[4] = 0x03;
    sendBuffer[5] = 0x00;
    sendBuffer[6] = 0x15;
    sendBuffer[7] = 0xFF;
    sendBuffer[8] = 0x00;
    sendBuffer[9] = 0x00;
    sendBuffer[10] = 0xFF;
    sendBuffer[11] = 0xC4;
    sendBuffer[12] = 0x30;
    sendBuffer[13] = 0x7F;
    sendBuffer[14] = 0xF5;
    sendBuffer[15] = 0xB5;
    SendCommand(data(sendBuffer), 16, nullptr, 0);
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x21;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x18;
    sendBuffer[4] = 0xB8;
    sendBuffer[5] = 0xDE;
    sendBuffer[6] = 0x05;
    sendBuffer[7] = 0x17;
    sendBuffer[8] = 0x16;
    sendBuffer[9] = 0x0C;
    sendBuffer[10] = 0x03;
    sendBuffer[11] = 0x00;
    sendBuffer[12] = 0x15;
    sendBuffer[13] = 0xFF;
    sendBuffer[14] = 0x00;
    sendBuffer[15] = 0x00;
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // RF PA Mode
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x22;
    sendBuffer[2] = 0x04;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x08;  // PA_MODE: PA switching amp mode, PA_SEL = HP_COARSE, disable power
                           // sequencing, disable external TX ramp signal
    sendBuffer[5] =
        0x18;  // PA_PWR_LVL: Enabled PA fingers (sets output power but not linearly; 10µA bias
               // current per enabled finger, complementary drive signal with 50% duty cycle)
    sendBuffer[6] = 0x00;  // PA_BIAS_CLKDUTY
    sendBuffer[7] = 0x91;  // PA_TC: Ramping time constant = 0x1B (~10us to full-0.5dB), FSK
                           // modulation delay 10µs
    SendCommand(data(sendBuffer), 8, nullptr, 0);

    // RF Synth Feed Forward Charge Pump Current, Integrated Charge Pump Current, VCO Gain Scaling
    // Factor, FF Loop Filter Values
    // TODO: What values to use here?
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x23;
    sendBuffer[2] = 0x07;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x2C;   // SYNTH_PFDCP_CPFF: FF charge pump current = 60µA
    sendBuffer[5] = 0x0E;   // SYNTH_PFDCP_CPINT: Int charge pump current = 30µA
    sendBuffer[6] = 0x0B;   // SYNTH_VCO_KV: Set VCO scaling factor to maximum value, set tuning
                            // varactor gain to maximum value
    sendBuffer[7] = 0x04;   // SYNTH_LPFILT3: R2 value 90kOhm
    sendBuffer[8] = 0x0C;   // SYNTH_LPFILT2: C2 value 11.25pF
    sendBuffer[9] = 0x73;   // SYNTH_LPFILT1: C3 value 12pF, C1 offset 0pF, C1 value 7.21pF
    sendBuffer[10] = 0x03;  // SYNTH_LPFILT0: FF amp bias current 100µA
    SendCommand(data(sendBuffer), 11, nullptr, 0);

    // RF Match Mask
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x30;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x00;   // MATCH_VALUE_1
    sendBuffer[5] = 0x00;   // MATCH_MASK_1
    sendBuffer[6] = 0x00;   // MATCH_CTRL_1
    sendBuffer[7] = 0x00;   // MATCH_VALUE_2
    sendBuffer[8] = 0x00;   // MATCH_MASK_2
    sendBuffer[9] = 0x00;   // MATCH_CTRL_2
    sendBuffer[10] = 0x00;  // MATCH_VALUE_3
    sendBuffer[11] = 0x00;  // MATCH_MASK_3
    sendBuffer[12] = 0x00;  // MATCH_CTRL_3
    sendBuffer[13] = 0x00;  // MATCH_VALUE_4
    sendBuffer[14] = 0x00;  // MATCH_MASK_4
    sendBuffer[15] = 0x00;  // MATCH_CTRL_4
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // Frequency Control
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x40;
    sendBuffer[2] = 0x08;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x41;  // FREQ_CONTROL_INTE: FC_inte = 0x41
    sendBuffer[5] = 0x0E;  // FREQ_CONTROL_FRAC: FC_frac. 0xD89D9 = 433.5, 0xEC4EC = 434.5
    sendBuffer[6] = 0xC4;
    sendBuffer[7] = 0xEC;
    // N_presc = 2, outdiv = 8, F_xo = 26MHz
    // RF_channel_Hz = (FC_inte + FC_frac/2^19)*((N_presc*F_xo)/outdiv) = 433.5000048MHz MHz
    sendBuffer[8] = 0x44;  // FREQ_CONTROL_CHANNEL_STEP_SIZE: Channel step size = 0x4444
    sendBuffer[9] = 0x44;
    sendBuffer[10] =
        0x20;  // FREQ_CONTROL_W_SIZE: Window gating period (in number of crystal clock cycles) = 32
    sendBuffer[11] = 0xFE;  // FREQ_CONTROL_VCOCNT_RX_ADJ: Adjust target mode for VCO calibration in
                            // RX mode = 0xFE int8_t
    SendCommand(data(sendBuffer), 12, nullptr, 0);

    // Set RF4463 Module Antenna Switch
    sendBuffer[0] = 0x13;
    sendBuffer[1] = 0x00;  // Don't change GPIO0 setting
    sendBuffer[2] = 0x00;  // Don't change GPIO1 setting
    sendBuffer[3] = 0x21;  // GPIO2 is active in RX state
    sendBuffer[4] = 0x20;  // GPIO3 is active in TX state
    sendBuffer[5] = 0x27;  // NIRQ is still used as NIRQ
    sendBuffer[6] = 0x0B;  // SDO is still used as SDO
    SendCommand(data(sendBuffer), 7, nullptr, 0);

    // Frequency Adjust (stolen from Arduino demo code)
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x62;
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // TX Buffer = RX Buffer = 64 Byte
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x03;
    sendBuffer[4] = 0x40;
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    paEnablePin.Direction(hal::PinDirection::out);
    paEnablePin.Set();
}


auto ReadPartNumber() -> std::uint16_t
{
    auto answer = SendCommand<partInfoAnswerLength>(Span(cmdPartInfo));
    return Deserialize<std::endian::big, std::uint16_t>(Span(answer).subspan<1, 2>());
}


// --- Private function definitions ---

auto InitializeGpioAndSpi() -> void
{
    csGpioPin.Direction(hal::PinDirection::out);
    csGpioPin.Set();

    nirqGpioPin.Direction(hal::PinDirection::in);

    sdnGpioPin.Direction(hal::PinDirection::out);
    sdnGpioPin.Set();

    gpio0GpioPin.Direction(hal::PinDirection::out);
    gpio0GpioPin.Reset();

    watchdogResetGpioPin.Direction(hal::PinDirection::out);
    watchdogResetGpioPin.Reset();
    AT(NOW() + watchDogResetPinPause);
    watchdogResetGpioPin.Set();
    AT(NOW() + watchDogResetPinPause);
    watchdogResetGpioPin.Reset();

    constexpr auto baudrate = 10'000'000;
    hal::Initialize(&spi, baudrate);

    // Enable Si4463 and wait for PoR to finish
    AT(NOW() + porCircuitSettlePause);
    sdnGpioPin.Reset();
    AT(NOW() + porRunningPause);
}


auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void
{
    auto const powerUpBuffer = std::to_array<Byte>(
        {cmdPowerUp,
         static_cast<Byte>(bootOptions),
         static_cast<Byte>(xtalOptions),
         static_cast<Byte>(xoFrequency >> (CHAR_BIT * 3)),  // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency >> (CHAR_BIT * 2)),  // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency >> (CHAR_BIT)),      // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency)});

    SendCommand(Span(powerUpBuffer));
}


[[deprecated]] auto SendCommand(std::uint8_t * data,
                                std::size_t length,
                                std::uint8_t * responseData,
                                std::size_t responseLength) -> void
{
    // RODOS::PRINTF("SendCommand()\n");
    csGpioPin.Reset();
    AT(NOW() + csPinAfterResetPause);
    spi.write(data, length);
    AT(NOW() + csPinPreSetPause);
    csGpioPin.Set();

    auto cts = std::to_array<uint8_t>({0x00, 0x00});
    auto req = std::to_array<uint8_t>({0x44, 0x00});
    do
    {
        AT(NOW() + initialWaitForCtsDelay);
        csGpioPin.Reset();
        AT(NOW() + csPinAfterResetPause);
        spi.writeRead(std::data(req), std::size(req), std::data(cts), std::size(cts));
        if(cts[1] != 0xFF)
        {
            AT(NOW() + csPinPreSetPause);
            csGpioPin.Set();
        }
    } while(cts[1] != 0xFF);

    if(responseLength > 0)
    {
        spi.read(responseData, responseLength);
    }

    AT(NOW() + csPinPreSetPause);
    csGpioPin.Set();
}


auto SendCommand(std::span<Byte const> data) -> void
{
    csGpioPin.Reset();
    AT(NOW() + csPinAfterResetPause);
    hal::WriteTo(&spi, data);
    AT(NOW() + csPinPreSetPause);
    csGpioPin.Set();
    WaitForCts();
}


template<std::size_t answerLength>
auto SendCommand(std::span<Byte const> data) -> std::array<Byte, answerLength>
{
    SendCommand(data);
    auto answer = std::array<Byte, answerLength>{};
    csGpioPin.Reset();
    AT(NOW() + csPinAfterResetPause);
    hal::ReadFrom(&spi, Span(&answer));
    AT(NOW() + csPinPreSetPause);
    csGpioPin.Set();
    return answer;
}


//! @brief Polls the CTS byte until 0xFF is received (i.e. Si4463 is ready for command).
auto WaitForCts() -> void
{
    auto const dataIsReadyValue = 0xFF_b;
    AT(NOW() + initialWaitForCtsDelay);
    do
    {
        csGpioPin.Reset();
        AT(NOW() + csPinAfterResetPause);
        hal::WriteTo(&spi, Span(cmdReadCmdBuff));
        auto cts = 0x00_b;
        hal::ReadFrom(&spi, Span(&cts));
        AT(NOW() + csPinPreSetPause);
        csGpioPin.Set();
        if(cts == dataIsReadyValue)
        {
            break;
        }
    } while(true);
    // TODO: We need to get rid of this infinite loop once we do proper error handling for the whole
    // RF code
}


auto SetTxType(TxType txType) -> void
{
    // Constants for setting the TX type (morse, 2GFSK)
    constexpr uint32_t dataRateMorse = 20'000U;  // MODEM_DATA_RATE: unused, 20k Baud
    constexpr uint32_t dataRate2Gfsk =
        9'600U;  // MODEM_DATA_RATE: For 9k6 Baud: (TX_DATA_RATE * MODEM_TX_NCO_MODE *
                 // TXOSR)/F_XTAL_Hz = (9600 * 2600000 * 10)/26000000 = 9600 = 0x002580

    constexpr auto modemModTypeMorse =
        0x09_b;  // MODEM_MODE_TYPE: TX data from GPIO0 pin, modulation OOK
    constexpr auto modemModType2Gfsk =
        0x03_b;  // MODEM_MODE_TYPE: TX data from packet handler, modulation 2GFSK

    auto modemModType = (txType == TxType::morse ? modemModTypeMorse : modemModType2Gfsk);
    auto dataRate = (txType == TxType::morse ? dataRateMorse : dataRate2Gfsk);

    auto propertyValues = std::to_array(
        {modemModType,
         0x00_b,
         0x07_b,  // NOLINT(*magic-numbers*), Delta-Sigma Modulator (DSM) default config
         static_cast<Byte>(dataRate >> (2 * CHAR_BIT)),  // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(dataRate >> (CHAR_BIT)),      // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(dataRate)});

    SetProperties(PropertyGroup::modem, 0x00_b, Span(propertyValues));
}


auto SetProperties(PropertyGroup propertyGroup,
                   Byte startProperty,
                   std::span<Byte const> propertyValues) -> void
{
    auto setPropertiesBuffer = std::array<Byte, setPropertiesHeaderSize + maxNProperties>{};

    setPropertiesBuffer[0] = cmdSetProperty;
    setPropertiesBuffer[1] = static_cast<Byte>(propertyGroup);
    setPropertiesBuffer[2] = static_cast<Byte>(std::size(propertyValues));
    setPropertiesBuffer[3] = startProperty;

    std::copy(std::begin(propertyValues),
              std::end(propertyValues),
              std::begin(setPropertiesBuffer) + setPropertiesHeaderSize);
    auto nProperties = std::size(propertyValues);
    auto bytesToSend = setPropertiesHeaderSize + nProperties;

    SendCommand(Span(setPropertiesBuffer).first(bytesToSend));
}
}

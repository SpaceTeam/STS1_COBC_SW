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

// Pause values for watchdog reset pin and PoR
// Jakob: Pause times are VERY generously overestimated
// TODO: Patrick: Are those delays really necessary? I have never seen something like that for SPI
// communication
// TODO: Do not use trailing comments since they cause line breaks

// Pause time to wait for Power on Reset to finish
constexpr auto porRunningDelay = 20 * MILLISECONDS;
// Time until PoR circuit settles after applying power
constexpr auto porCircuitSettleDelay = 100 * MILLISECONDS;
// Pause time for the sequence reset -> pause -> set -> pause -> reset in initialization
constexpr auto watchDogResetPinDelay = 1 * MILLISECONDS;


// --- Private function declarations ---

auto InitializeGpioAndSpi() -> void;

auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void;

auto SendCommand(std::span<Byte const> data) -> void;

template<std::size_t answerLength>
auto SendCommand(std::span<Byte const> data) -> std::array<Byte, answerLength>;

auto WaitForCts() -> void;

auto SetProperties(PropertyGroup propertyGroup,
                   Byte startProperty,
                   std::span<Byte const> propertyValues) -> void;


// --- Public function definitions ---

// TODO: Get rid of all the magic numbers

auto Initialize(TxType txType) -> void
{
    // TODO: Don't forget that WDT_Clear has to be triggered regularely for the TX to work! (even
    // without the watchdog timer on the PCB it needs to be triggered at least once after boot to
    // enable the TX)

    InitializeGpioAndSpi();

    PowerUp(PowerUpBootOptions::noPatch, PowerUpXtalOptions::xtal, powerUpXoFrequency);

    // GPIO Pin Cfg
    SendCommand(Span({cmdGpioPinCfg, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b}));

    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    // Global XO Tune 2
    SetProperties(PropertyGroup::global,
                  0x00_b,
                  Span({
                      0x52_b,  // GLOBAL_XO_TUNE
                      0x00_b   // GLOBAL_CLK_CFG
                  }));

    // RF Global Config 1
    SetProperties(PropertyGroup::global,
                  0x03_b,
                  Span({
                      0x60_b  // GLOBAL_CONFIG: High performance mode,
                              // Generic packet format, Split FiFo mode, Fast sequencer mode
                  }));

    // RF Int Ctl Enable
    SetProperties(PropertyGroup::intCtl,
                  0x00_b,
                  Span({
                      0x01_b  // INT_CTL: Enable packet handler interrupts
                  }));

    // TX Preamble Length
    SetProperties(PropertyGroup::preamble,
                  0x00_b,
                  Span({
                      0x00_b,  // PREAMBLE_TX_LENGTH: 0 bytes preamble
                      0x14_b,  // PREAMBLE_CONFIG_STD_1: Normal sync timeout,
                               // 14 bit preamble RX threshold
                      0x00_b,  // PREAMBLE_CONFIG_NSTD: No non-standard preamble pattern TODO: Maybe
                               // we can detect RS+CC encoded preamble this way and be CCSDS
                               // compliant on uplink too? Problem: Max pattern length is 32 bit
                      0x0F_b,  // PREAMBLE_CONFIG_STD_2: No extended RX preamble timeout, 0x0f
                               // nibbles timeout until detected preamble is discarded as invalid
                      0x31_b,  // PREAMBLE_CONFIG: First transmitted preamble bit is 1, unit of
                               // preampreamble TX length is in bytes
                      0x00_b,  // PREAMBLE_PATTERN: Non-standard pattern
                      0x00_b,  // Non-standard pattern
                      0x00_b,  // Non-standard pattern
                      0x00_b,  // Non-standard pattern
                  }));

    // Sync word config
    SetProperties(PropertyGroup::sync,
                  0x00_b,
                  Span({
                      0x43_b,        // SYNC_CONFIG: Allow 4 bit sync word errors, 4 byte sync word
                      0b01011000_b,  // SYNC_BITS: Valid CCSDS TM sync word for
                                     // Reed-Solomon or convolutional coding
                      0b11110011_b,  // Be careful: Send order is MSB-first but Little endian so the
                                     // lowest bit of the
                      0b00111111_b,  // highest byte is transmitted first,
                                     // which is different to how the CCSDS spec
                      0b10111000_b   // annotates those bit patterns!
                  }));
    // TODO: Check that pattern!

    // CRC Config
    SetProperties(PropertyGroup::pkt,
                  0x00_b,
                  Span({
                      0x00_b  // PKT_CRC_CONFIG: No CRC
                  }));

    // Whitening and Packet Parameters
    SetProperties(PropertyGroup::pkt,
                  0x05_b,
                  Span({
                      0x00_b,  // PKT_WHT_BIT_NUM: Disable whitening
                      0x01_b   // PKT_CONFIG1: Don't split RX and TX field information (length,
                               // ...), enable RX packet handler, use normal (2)FSK, no Manchester
                               // coding, no CRC, data transmission with MSB first.
                  }));

    // Pkt Length part 1
    SetProperties(PropertyGroup::pkt,
                  0x08_b,
                  Span({
                      0x60_b,  // PKT_LEN: Infinite receive, big endian (MSB first)
                      0x00_b,  // PKT_LEN_FIELD_SOURCE
                      0x00_b,  // PKT_LEN_ADJUST
                      0x30_b,  // PKT_TX_THRESHOLD: Trigger TX FiFo almost empty interrupt when 0x30
                               // bytes in FiFo (size 0x40) are empty
                      0x30_b,  // PKT_RX_THRESHOLD: Trigger RX FiFo almost full interrupt when 0x30
                               // bytes in FiFo (size 0x40) are full
                      0x00_b,  // PKT_FIELD_1_LENGTH
                      0x00_b,
                      0x04_b,  // PKT_FIELD_1_CONFIG
                      0x80_b,  // PKT_FIELD_1_CRC_CONFIG
                      0x00_b,  // PKT_FIELD_2_LENGTH
                      0x00_b,
                      0x00_b  // PKT_FIELD_2_CONFIG
                  }));

    // Pkt Length part 2
    SetProperties(PropertyGroup::pkt,
                  0x14_b,
                  Span({
                      0x00_b,  // PKT_FIELD_2_CRC_CONFIG
                      0x00_b,  // PKT_FIELD_3_LENGTH
                      0x00_b,
                      0x00_b,  // PKT_FIELD_3_CONFIG
                      0x00_b,  // PKT_FIELD_3_CRC_CONFIG
                      0x00_b,  // PKT_FIELD_4_LENGTH
                      0x00_b,
                      0x00_b,  // PKT_FIELD_4_CONFIG
                      0x00_b,  // PKT_FIELD_4_CRC_CONFIG
                      0x00_b,  // PKT_FIELD_5_LENGTH
                      0x00_b,
                      0x00_b  // PKT_FIELD_5_CONFIG
                  }));

    // Pkt Length part 3
    SetProperties(PropertyGroup::pkt,
                  0x20_b,
                  Span({
                      0x00_b,  // PKT_FIELD_5_CRC_CONFIG
                      0x00_b,  // PKT_RX_FIELD_1_LENGTH
                      0x00_b,
                      0x00_b,  // PKT_RX_FIELD_1_CONFIG
                      0x00_b,  // PKT_RX_FIELD_1_CRC_CONFIG
                      0x00_b,  // PKT_RX_FIELD_2_LENGTH
                      0x00_b,
                      0x00_b,  // PKT_RX_FIELD_2_CONFIG
                      0x00_b,  // PKT_RX_FIELD_2_CRC_CONFIG
                      0x00_b,  // PKT_RX_FIELD_3_LENGTH
                      0x00_b,
                      0x00_b  // PKT_RX_FIELD_3_CONFIG
                  }));

    // Pkt Length part 4
    SetProperties(PropertyGroup::pkt,
                  0x2C_b,
                  Span({
                      0x00_b,  // PKT_RX_FIELD_3_CRC_CONFIG
                      0x00_b,  // PKT_RX_FIELD_4_LENGTH
                      0x00_b,
                      0x00_b,  // PKT_RX_FIELD_4_CONFIG
                      0x00_b,  // PKT_RX_FIELD_4_CRC_CONFIG
                      0x00_b,  // PKT_RX_FIELD_5_LENGTH
                      0x00_b,
                      0x00_b,  // PKT_RX_FIELD_5_CONFIG
                      0x00_b   // PKT_RX_FIELD_5_CRC_CONFIG
                  }));

    // RF Modem Mod Type
    SetTxType(txType);
    SetProperties(
        PropertyGroup::modem,
        0x06_b,  // SetTxType sets modem properties from 0x00 to 0x05
        Span({0x00_b,  // MODEM_TX_NCO_MODE: TXOSR=x10=0, NCOMOD=F_XTAL/10=2600000=0x027ac40
              0x27_b,
              0xAC_b,
              0x40_b,
              0x00_b,  // MODEM_FREQ_DEVIATION: (2^19 * outdiv * deviation_Hz)/(N_presc *
                       // F_xo) = (2^19 * 8 * 9600/4)/(2 * 26000000) = 194 = 0x0000C2
              0x00_b,
              0xC2_b}));


    // RF Modem TX Ramp Delay, Modem MDM Ctrl, Modem IF Ctrl, Modem IF Freq & Modem Decimation
    // Cfg
    SetProperties(
        PropertyGroup::modem,
        0x18_b,
        Span({
            0x01_b,  // MODEM_TX_RAMP_DELAY: Ramp Delay 1
            0x80_b,  // MODEM_MDM_CTRL: Slicer phase source from detector's output
            0x08_b,  // MODEM_IF_CONTROL: No ETSI mode, fixed IF mode,
                     // normal IF mode (nonzero IF)
            0x03_b,  // MODEM_IF_FREQ: IF = (2^19 * outdiv * IF_Freq_Hz)/(npresc * freq_xo) =
                     // (2^19 * 8 * xxx)/(2 * 26000000) = 0x03C000 (default value)
                     // TODO: Is it important what we chose here?
            0xC0_b,
            0x00_b,
            0x70_b,  // MODEM_DECIMATION_CFG1: Decimation NDEC0 = 0, NDEC1 = decimation
                     // by 8, NDEC2 = decimation by 2
            0x20_b   // MODEM_DECIMATION_CFG0: Normal decimate-by-8 filter gain,
                     // don't bypass the decimate-by-2 polyphase filter, bypass the decimate-by-3
                     // polyphase filter, enable
                     // droop compensation, channel selection filter in normal mode (27 tap filter)
        }));

    // RF Modem BCR Oversampling Rate, Modem BCR NCO Offset, Modem BCR Gain, Modem BCR Gear &
    // Modem BCR Misc
    // TODO: What values to use here?
    SetProperties(
        PropertyGroup::modem,
        0x22_b,
        Span({
            0x03_b,  // MODEM_BCR_OSR: RX symbol oversampling rate of 0x30D/8 = 781/8
                     // = 97.625 (According to the datasheet usual values are in the range
                     // of 8 to 12 where this value seems to be odd?)
            0x0D_b,
            0x00_b,  // MODEM_BCR_NCO_OFFSET: BCR NCO offset of
                     // 0x00A7C6/64 = 42950/64 = 671.09375
            0xA7_b,
            0xC6_b,
            0x00_b,  // MODEM_BCR_GAIN: BCR gain 0x054 = 84
            0x54_b,
            0x02_b,  // MODEM_BCR_GEAR: BCR loop gear control. CRSLOW=2, CRFAST=0
            0xC2_b   // MODEM_BCR_MISC1: Stop NCO for one sample clock in BCR mid-point phase
                     // sampling condition to escape, disable NCO resetting in case of
                     // mid-point phase sampling condition, don't double BCR loop gain, BCR
                     // NCO compensation is sampled upon detection of the preamble end,
                     // disable NCO frequency compensation, bypass compensation term feedback
                     // to slicer, bypass compensation term feedback to BCR tracking loop
        }));

    // RF Modem AFC Gear, Modem AFC Wait, Modem AFC Gain, Modem AFC Limiter & Modem AFC Misc
    // TODO: What values to use here?
    SetProperties(
        PropertyGroup::modem,
        0x2C_b,
        Span({
            0x04_b,  // MODEM_AFC_GEAR: AFC_SLOW gain 4, AFC_FAST gain 0, Switch gear
                     // after detection of preamble
            0x36_b,  // MODEM_AFC_WAIT: LGWAIT = 6, SHWAIT = 3
            0x80_b,  // MODEM_AFC_GAIN: AFC loop gain = 0x003, don't half the loop gain,
                     // disable adaptive RX bandwidth, enable frequency error estimation
            0x03_b,
            0x30_b,  // MODEM_AFC_LIMITER: 0x30AF
            0xAF_b,
            0x80_b  // MODEM_AFC_MISC: Expected freq error is less then 12*symbol rate, AFC
                    // correction of PLL will be frozen if a consecutive string of 1s or 0s that
                    // exceed the search period is encountered, don't switch clock source for
                    // frequency estimator, don't freeze AFC at preamble end, AFC correction uses
                    // freq estimation by moving average or minmax detector in async demod,disable
                    // AFC value feedback to PLL, freeze AFC after gear switching
        }));

    // RF Modem AGC Control
    // TODO: What values to use here?
    SetProperties(
        PropertyGroup::modem,
        0x35_b,
        Span({
            0xE2_b  // MODEM_AGC_CONTROL: reset peak detectors only on change of gain
                    // indicated by peak detector output, reduce ADC gain when AGC gain is at
                    // minimum, normal AGC speed, don't increase AGC gain during signal
                    // reductions in ant diversity mode, always perform gain decreases in 3dB
                    // steps instead of 6dB steps, AGC is enabled over whole packet length
        }));

    // RF Modem AGC Window Size, AGC RF Peak Detector Decay, AGC IF Peak Detector Decay, 4FSK
    // Gain, 4FSK Slicer Threshold, 4FSK SYmbol Mapping Code, OOK Attack/Decay Times
    // TODO: What values to use here?
    SetProperties(PropertyGroup::modem,
                  0x38_b,
                  Span({
                      0x11_b,  // MODEM_AGC_WINDOW_SIZE: AGC gain settling window size = 1, AGC
                               // signal level measurement window = 1
                      0xAB_b,  // MODEM_AGC_RFPD_DECAY: RF peak detector decay time = 0xAB = 171
                      0xAB_b,  // MODEM_AGC_IFPD_DECAY: IF peak detector decay time = 0xAB = 171
                      0x00_b,  // MODEM_FSK4_GAIN1: 4FSK Gain1 = 0,
                               // Normal second phase compensation factor
                      0x02_b,  // MODEM_FSK4_GAIN0: 4FSK Gain0 = 2, disable 2FSK phase compensation
                      0xFF_b,  // MODEM_FSK4_TH: 4FSK slicer threshold = 0xFFFF
                      0xFF_b,
                      0x00_b,  // MODEM_FSK4_MAP: 4FSK symbol map 0 (`00 `01 `11 `10)
                      0x2B_b   // MODEM_OOK_PDTC: OOK decay = 11, OOK attack = 2
                  }));

    // RF Modem OOK Control, OOK Misc, RAW Search, RAW Control, RAW Eye, Antenna Diversity Mode,
    // Antenna Diversity Control, RSSI Threshold
    SetProperties(
        PropertyGroup::modem,
        0x42_b,
        Span({
            0xA4_b,  // MODEM_OOK_CNT1: OOK Squelch off, OOK slicer output de-glitching by bit
                     // clock, raw output is synced to clock, MA_FREQUDOWN=0, AGC and OOK movign
                     // average detector threshold will be frozen after preamble detection,
                     // S2P_MAP=2
            0x02_b,  // MODEM_OOK_MISC: OOK uses moving average detector, OOK peak detector
                     // discharge does not affect decay rate, disable OOK squelch, always
                     // discharge peak detector, normal moving average window
            0xD6_b,  // ??
            0x83_b,  // MODEM_RAW_CONTROL
            0x00_b,  // MODEM_RAW_EYE: RAW eye open detector threshold
            0xAD_b,
            0x01_b,  // MODEM_ANT_DIV_MODE: Antenna diversity mode
            0x80_b,  // MODEM_ANT_DIV_CONTROL: Antenna diversity control
            0xFF_b   // MODEM_RSSI_THRESH: Threshold for clear channel assessment and
                     // RSSI interrupt generation
        }));

    // RF Modem RSSI Control
    SetProperties(PropertyGroup::modem,
                  0x4C_b,
                  Span({
                      0x00_b  // MODEM_RSSI_CONTROL: Disable RSSI latch, RSSI value is avg over
                              // last 4*Tb bit periods, disable RSSI threshold check after latch
                  }));

    // RF Modem RSSI Compensation
    // TODO: Measure this
    SetProperties(PropertyGroup::modem,
                  0x4E_b,
                  Span({
                      0x40_b  // MODEM_RSSI_COMP: Compensation/offset of measured RSSI value
                  }));

    // RF Modem Clock generation Band
    SetProperties(PropertyGroup::modem,
                  0x51_b,
                  Span({
                      0x0A_b  // MODEM_CLKGEN_BAND: Band = FVCO_DIV_8, high performance mode fixed
                              // prescaler div2, force recalibration
                  }));

    // RX Filter Coefficients
    // TODO: What values to use here?
    SetProperties(PropertyGroup::modemChflt,
                  0x00_b,
                  Span({
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
                  }));

    SetProperties(
        PropertyGroup::modemChflt,
        0x0C_b,
        Span({
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
        }));

    SetProperties(
        PropertyGroup::modemChflt,
        0x18_b,
        Span({
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
        }));

    // RF PA Mode
    SetProperties(PropertyGroup::pa,
                  0x00_b,
                  Span({
                      0x08_b,  // PA_MODE: PA switching amp mode, PA_SEL = HP_COARSE, disable power
                               // sequencing, disable external TX ramp signal
                      0x18_b,  // PA_PWR_LVL: Enabled PA fingers (sets output power but not
                               // linearly; 10µA bias current per enabled finger, complementary
                               // drive signal with 50% duty cycle)
                      0x00_b,  // PA_BIAS_CLKDUTY
                      0x91_b   // PA_TC: Ramping time constant = 0x1B (~10us to full-0.5dB), FSK
                               // modulation delay 10µs
                  }));

    // RF Synth Feed Forward Charge Pump Current, Integrated Charge Pump Current, VCO Gain
    // Scaling Factor, FF Loop Filter Values
    // TODO: What values to use here?
    SetProperties(PropertyGroup::synth,
                  0x00_b,
                  Span({
                      0x2C_b,  // SYNTH_PFDCP_CPFF: FF charge pump current = 60µA
                      0x0E_b,  // SYNTH_PFDCP_CPINT: Int charge pump current = 30µA
                      0x0B_b,  // SYNTH_VCO_KV: Set VCO scaling factor to maximum value, set tuning
                               // varactor gain to maximum value
                      0x04_b,  // SYNTH_LPFILT3: R2 value 90kOhm
                      0x0C_b,  // SYNTH_LPFILT2: C2 value 11.25pF
                      0x73_b,  // SYNTH_LPFILT1: C3 value 12pF, C1 offset 0pF, C1 value 7.21pF
                      0x03_b   // SYNTH_LPFILT0: FF amp bias current 100µA
                  }));

    // RF Match Mask
    SetProperties(PropertyGroup::match,
                  0x00_b,
                  Span({
                      0x00_b,  // MATCH_VALUE_1
                      0x00_b,  // MATCH_MASK_1
                      0x00_b,  // MATCH_CTRL_1
                      0x00_b,  // MATCH_VALUE_2
                      0x00_b,  // MATCH_MASK_2
                      0x00_b,  // MATCH_CTRL_2
                      0x00_b,  // MATCH_VALUE_3
                      0x00_b,  // MATCH_MASK_3
                      0x00_b,  // MATCH_CTRL_3
                      0x00_b,  // MATCH_VALUE_4
                      0x00_b,  // MATCH_MASK_4
                      0x00_b   // MATCH_CTRL_4
                  }));

    // Frequency Control
    SetProperties(
        PropertyGroup::freqControl,
        0x00_b,
        Span({
            0x41_b,  // FREQ_CONTROL_INTE: FC_inte = 0x41
            0x0E_b,  // FREQ_CONTROL_FRAC: FC_frac. 0xD89D9 = 433.5, 0xEC4EC = 434.5
            0xC4_b,
            0xEC_b,
            // N_presc = 2, outdiv = 8, F_xo = 26MHz
            // RF_channel_Hz = (FC_inte + FC_frac/2^19)*((N_presc*F_xo)/outdiv) = 433.5000048MHz MHz
            0x44_b,  // FREQ_CONTROL_CHANNEL_STEP_SIZE: Channel step size = 0x4444
            0x44_b,
            0x20_b,  // FREQ_CONTROL_W_SIZE: Window gating period (in number of crystal clock
                     // cycles) = 32
            0xFE_b   // FREQ_CONTROL_VCOCNT_RX_ADJ: Adjust target mode for VCO
                     // calibration in RX mode = 0xFE int8_t
        }));

    // Set RF4463 Module Antenna Switch
    SendCommand(Span({
        cmdGpioPinCfg,
        0x00_b,  // Don't change GPIO0 setting
        0x00_b,  // Don't change GPIO1 setting
        0x21_b,  // GPIO2 is active in RX state
        0x20_b,  // GPIO3 is active in TX state
        0x27_b,  // NIRQ is still used as NIRQ
        0x0B_b   // SDO is still used as SDO
    }));

    // Frequency Adjust (stolen from Arduino demo code)
    SetProperties(PropertyGroup::global,
                  0x00_b,
                  Span({
                      0x62_b  // GLOBAL_XO_TUNE
                  }));

    // Change sequencer mode to guaranteed
    // TODO: Why?
    SetProperties(PropertyGroup::global,
                  0x03_b,
                  Span({
                      0x40_b  // GLOBAL_CONFIG: Split FIFO and guaranteed sequencer mode
                  }));

    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

    paEnablePin.Direction(hal::PinDirection::out);
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
    AT(NOW() + watchDogResetPinDelay);
    watchdogResetGpioPin.Set();
    AT(NOW() + watchDogResetPinDelay);
    watchdogResetGpioPin.Reset();

    constexpr auto baudrate = 10'000'000;
    hal::Initialize(&spi, baudrate);

    // Enable Si4463 and wait for PoR to finish
    AT(NOW() + porCircuitSettleDelay);
    sdnGpioPin.Reset();
    AT(NOW() + porRunningDelay);
}


auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void
{
    SendCommand(
        Span({cmdPowerUp,
              static_cast<Byte>(bootOptions),
              static_cast<Byte>(xtalOptions),
              static_cast<Byte>(xoFrequency >> (CHAR_BIT * 3)),  // NOLINT(hicpp-signed-bitwise)
              static_cast<Byte>(xoFrequency >> (CHAR_BIT * 2)),  // NOLINT(hicpp-signed-bitwise)
              static_cast<Byte>(xoFrequency >> (CHAR_BIT)),      // NOLINT(hicpp-signed-bitwise)
              static_cast<Byte>(xoFrequency)}));
}


auto SendCommand(std::span<Byte const> data) -> void
{
    csGpioPin.Reset();
    hal::WriteTo(&spi, data);
    csGpioPin.Set();
    WaitForCts();
}


template<std::size_t answerLength>
auto SendCommand(std::span<Byte const> data) -> std::array<Byte, answerLength>
{
    SendCommand(data);
    auto answer = std::array<Byte, answerLength>{};
    csGpioPin.Reset();
    hal::ReadFrom(&spi, Span(&answer));
    csGpioPin.Set();
    return answer;
}


//! @brief Polls the CTS byte until 0xFF is received (i.e. Si4463 is ready for command).
auto WaitForCts() -> void
{
    auto const dataIsReadyValue = 0xFF_b;
    do
    {
        csGpioPin.Reset();
        hal::WriteTo(&spi, Span(cmdReadCmdBuff));
        auto cts = 0x00_b;
        hal::ReadFrom(&spi, Span(&cts));
        csGpioPin.Set();
        if(cts == dataIsReadyValue)
        {
            break;
        }
    } while(true);
    // TODO: We need to get rid of this infinite loop once we do proper error handling for the whole
    // RF code
}


auto SetProperties(PropertyGroup propertyGroup,
                   Byte startProperty,
                   std::span<Byte const> propertyValues) -> void
{
    auto setPropertiesBuffer = std::array<Byte, setPropertiesHeaderSize + maxNProperties>{};
    auto nProperties = std::size(propertyValues);
    auto bytesToSend = setPropertiesHeaderSize + nProperties;

    setPropertiesBuffer[0] = cmdSetProperty;
    setPropertiesBuffer[1] = static_cast<Byte>(propertyGroup);
    setPropertiesBuffer[2] = static_cast<Byte>(nProperties);
    setPropertiesBuffer[3] = startProperty;

    std::copy(std::begin(propertyValues),
              std::end(propertyValues),
              std::begin(setPropertiesBuffer) + setPropertiesHeaderSize);

    SendCommand(Span(setPropertiesBuffer).first(bytesToSend));
}
}

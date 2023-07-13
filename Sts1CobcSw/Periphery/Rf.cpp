#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Periphery/RfNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <climits>
#include <cstdint>
#include <span>
#include <string_view>


namespace sts1cobcsw::periphery::rf
{
using sts1cobcsw::serial::operator""_b;
using RODOS::AT;
using RODOS::MICROSECONDS;
using RODOS::MILLISECONDS;
using RODOS::NOW;
using sts1cobcsw::serial::Byte;


// --- Globals ---

constexpr auto maxMorseLetterLength = 5U;

// Morse letters
// If punctuation is left out, the max. length is 5 dots/dashes
// Therefore we can use the first 3 bits of a uint8_t to store the length
// The rest for the dot/dash representation (dot ... 1, dash ...0)
constexpr auto morseDot = 1;
constexpr auto morseDash = 0;
constexpr std::uint8_t morseA = (2U << maxMorseLetterLength) | 0b10U;
constexpr std::uint8_t morseB = (4U << maxMorseLetterLength) | 0b0111U;
constexpr std::uint8_t morseC = (4U << maxMorseLetterLength) | 0b0101U;
constexpr std::uint8_t morseD = (3U << maxMorseLetterLength) | 0b011U;
constexpr std::uint8_t morseE = (1U << maxMorseLetterLength) | 0b1U;
constexpr std::uint8_t morseF = (4U << maxMorseLetterLength) | 0b1101U;
constexpr std::uint8_t morseG = (3U << maxMorseLetterLength) | 0b001U;
constexpr std::uint8_t morseH = (4U << maxMorseLetterLength) | 0b1111U;
constexpr std::uint8_t morseI = (2U << maxMorseLetterLength) | 0b11U;
constexpr std::uint8_t morseJ = (4U << maxMorseLetterLength) | 0b1000U;
constexpr std::uint8_t morseK = (3U << maxMorseLetterLength) | 0b010U;
constexpr std::uint8_t morseL = (4U << maxMorseLetterLength) | 0b1011U;
constexpr std::uint8_t morseM = (2U << maxMorseLetterLength) | 0b00U;
constexpr std::uint8_t morseN = (2U << maxMorseLetterLength) | 0b01U;
constexpr std::uint8_t morseO = (3U << maxMorseLetterLength) | 0b000U;
constexpr std::uint8_t morseP = (4U << maxMorseLetterLength) | 0b1001U;
constexpr std::uint8_t morseQ = (4U << maxMorseLetterLength) | 0b0010U;
constexpr std::uint8_t morseR = (3U << maxMorseLetterLength) | 0b101U;
constexpr std::uint8_t morseS = (3U << maxMorseLetterLength) | 0b111U;
constexpr std::uint8_t morseT = (1U << maxMorseLetterLength) | 0b0U;
constexpr std::uint8_t morseU = (3U << maxMorseLetterLength) | 0b110U;
constexpr std::uint8_t morseV = (4U << maxMorseLetterLength) | 0b1110U;
constexpr std::uint8_t morseW = (3U << maxMorseLetterLength) | 0b100U;
constexpr std::uint8_t morseX = (4U << maxMorseLetterLength) | 0b0110U;
constexpr std::uint8_t morseY = (4U << maxMorseLetterLength) | 0b0100U;
constexpr std::uint8_t morseZ = (4U << maxMorseLetterLength) | 0b0011U;
constexpr std::uint8_t morse1 = (5U << maxMorseLetterLength) | 0b10000U;
constexpr std::uint8_t morse2 = (5U << maxMorseLetterLength) | 0b11000U;
constexpr std::uint8_t morse3 = (5U << maxMorseLetterLength) | 0b11100U;
constexpr std::uint8_t morse4 = (5U << maxMorseLetterLength) | 0b11110U;
constexpr std::uint8_t morse5 = (5U << maxMorseLetterLength) | 0b11111U;
constexpr std::uint8_t morse6 = (5U << maxMorseLetterLength) | 0b01111U;
constexpr std::uint8_t morse7 = (5U << maxMorseLetterLength) | 0b00111U;
constexpr std::uint8_t morse8 = (5U << maxMorseLetterLength) | 0b00011U;
constexpr std::uint8_t morse9 = (5U << maxMorseLetterLength) | 0b00001U;
constexpr std::uint8_t morse0 = (5U << maxMorseLetterLength) | 0b00000U;
constexpr std::uint8_t morseSpace = 0;

auto spi = RODOS::HAL_SPI(hal::rfSpiIndex, hal::rfSpiSckPin, hal::rfSpiMisoPin, hal::rfSpiMosiPin);
auto csGpioPin = hal::GpioPin(hal::rfCsPin);
auto nirqGpioPin = hal::GpioPin(hal::rfNirqPin);
auto sdnGpioPin = hal::GpioPin(hal::rfSdnPin);
auto gpio0GpioPin = hal::GpioPin(hal::rfGpio0Pin);
auto gpio1GpioPin = hal::GpioPin(hal::rfGpio1Pin);
auto paEnablePin = hal::GpioPin(hal::rfPaEnablePin);

// TODO: This should probably be somewhere else as it is not directly related to the RF module
auto watchdogResetGpioPin = hal::GpioPin(hal::watchdogResetPin);

constexpr std::uint16_t partInfo = 0x4463;
constexpr std::uint32_t powerUpXoFrequency = 26'000'000;  // 26 MHz

constexpr auto fifoAlmostFullThreshold = 48;              // RX FIFO
constexpr auto fifoAlmostEmptyThreshold = 48;             // TX FIFO

// Morse timings, derived from dot time, from
// https://www.electronics-notes.com/articles/ham_radio/morse_code/characters-table-chart.php
constexpr auto morseDotTime = 100 * RODOS::MILLISECONDS;
constexpr auto morseDashTime = 3 * morseDotTime;
constexpr auto morseBitOffTime = 1 * morseDotTime;
constexpr auto morseLetterOffTime = 3 * morseDotTime;
constexpr auto morseWordOffTime = 7 * morseDotTime;


// --- Private function declarations ---

[[deprecated]] auto SendCommand(std::uint8_t * data,
                                std::size_t length,
                                std::uint8_t * responseData,
                                std::size_t responseLength) -> void;
auto SendCommandNoResponse(std::span<Byte> commandBuffer) -> void;

auto CharToMorseLetter(char c) -> std::uint8_t;

template<std::size_t nResponseBytes>
auto SendCommandWithResponse(std::span<Byte> commandBuffer) -> std::array<Byte, nResponseBytes>;

auto WriteFifo(std::uint8_t * data, std::size_t length) -> void;
auto ReadFifo(std::uint8_t * data, std::size_t length) -> void;

template<std::size_t nProperties>
    requires(nProperties >= 1 and nProperties <= maxNProperties)
auto SetProperty(PropertyGroup propertyGroup,
                 Byte startProperty,
                 std::span<Byte, nProperties> propertyValues) -> void;
auto WaitOnCts() -> void;
auto ConfigureGpio(
    Byte gpio0, Byte gpio1, Byte gpio2, Byte gpio3, Byte nirq, Byte sdo, Byte genConfig) -> void;


// --- Public function definitions ---

// TODO: Get rid of all the magic numbers
// TODO: Replace all C-style arrays with std::array

auto InitializeGpioAndSpi() -> void
{
    // RODOS::PRINTF("InitializeGpioAndSpi()\n");
    csGpioPin.Direction(hal::PinDirection::out);
    csGpioPin.Set();

    nirqGpioPin.Direction(hal::PinDirection::in);

    sdnGpioPin.Direction(hal::PinDirection::out);
    sdnGpioPin.Set();

    gpio0GpioPin.Direction(hal::PinDirection::out);
    gpio0GpioPin.Reset();

    watchdogResetGpioPin.Direction(hal::PinDirection::out);
    watchdogResetGpioPin.Reset();
    AT(NOW() + 1 * MILLISECONDS);
    watchdogResetGpioPin.Set();
    AT(NOW() + 1 * MILLISECONDS);
    watchdogResetGpioPin.Reset();

    constexpr auto baudrate = 10'000'000;
    auto spiError = spi.init(baudrate, /*slave=*/false, /*tiMode=*/false);
    if(spiError == -1)
    {
        RODOS::PRINTF("Error initializing RF SPI!\n");
        // TODO: proper error handling
        return;
    }

    // Enable Si4463 and wait for PoR to finish
    AT(NOW() + 100 * MILLISECONDS);
    sdnGpioPin.Reset();
    AT(NOW() + 20 * MILLISECONDS);
}


auto Initialize(TxType txType) -> void
{
    // TODO: Don't forget that WDT_Clear has to be triggered regularely for the TX to work! (even
    // without the watchdog timer on the PCB it needs to be triggered at least once after boot to
    // enable the TX)

    InitializeGpioAndSpi();

    auto sendBuffer = std::array<std::uint8_t, 32>{};

    PowerUp(PowerUpBootOptions::noPatch, PowerUpXtalOptions::xtal, powerUpXoFrequency);

    // GPIO Pin Cfg
    sendBuffer[0] = 0x13;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x00;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x00;
    sendBuffer[5] = 0x00;
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;
    SendCommand(data(sendBuffer), 8, nullptr, 0);

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
    sendBuffer[4] = 0x60;   // PKT_LEN: Infinite receive, big endian (MSB first)
    sendBuffer[5] = 0x00;   // PKT_LEN_FIELD_SOURCE
    sendBuffer[6] = 0x00;   // PKT_LEN_ADJUST
    sendBuffer[7] = 0x30;   // PKT_TX_THRESHOLD: Trigger TX FiFo almost empty interrupt when 0x30
                            // bytes in FiFo (size 0x40) are empty
    sendBuffer[8] = 0x30;   // PKT_RX_THRESHOLD: Trigger RX FiFo almost full interrupt when 0x30
                            // bytes in FiFo (size 0x40) are full
    sendBuffer[9] = 0x00;   // PKT_FIELD_1_LENGTH
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
    sendBuffer[4] = 0x00;   // PKT_FIELD_2_CRC_CONFIG
    sendBuffer[5] = 0x00;   // PKT_FIELD_3_LENGTH
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;   // PKT_FIELD_3_CONFIG
    sendBuffer[8] = 0x00;   // PKT_FIELD_3_CRC_CONFIG
    sendBuffer[9] = 0x00;   // PKT_FIELD_4_LENGTH
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
    sendBuffer[4] = 0x00;   // PKT_FIELD_5_CRC_CONFIG
    sendBuffer[5] = 0x00;   // PKT_RX_FIELD_1_LENGTH
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;   // PKT_RX_FIELD_1_CONFIG
    sendBuffer[8] = 0x00;   // PKT_RX_FIELD_1_CRC_CONFIG
    sendBuffer[9] = 0x00;   // PKT_RX_FIELD_2_LENGTH
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
    sendBuffer[4] = 0x00;   // PKT_RX_FIELD_3_CRC_CONFIG
    sendBuffer[5] = 0x00;   // PKT_RX_FIELD_4_LENGTH
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;   // PKT_RX_FIELD_4_CONFIG
    sendBuffer[8] = 0x00;   // PKT_RX_FIELD_4_CRC_CONFIG
    sendBuffer[9] = 0x00;   // PKT_RX_FIELD_5_LENGTH
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
                           // (2^19 * 8 * xxx)/(2 * 26000000) = 0x03C000 (defaullt value)
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
    sendBuffer[9] = 0x00;   // MODEM_BCR_GAIN: BCR gain 0x054 = 84
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
               // PLL will be frozen if a consecutive string of 1s or 0s that exceed the serach
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
    sendBuffer[4] = 0x11;   // MODEM_AGC_WINDOW_SIZE: AGC gain settling window size = 1, AGC signal
                            // level measurement window = 1
    sendBuffer[5] = 0xAB;   // MODEM_AGC_RFPD_DECAY: RF peak detector decay time = 0xAB = 171
    sendBuffer[6] = 0xAB;   // MODEM_AGC_IFPD_DECAY: IF peak detector decay time = 0xAB = 171
    sendBuffer[7] = 0x00;   // MODEM_FSK4_GAIN1: 4FSK Gain1 = 0,
                            // Normal second phase compensation factor
    sendBuffer[8] = 0x02;   // MODEM_FSK4_GAIN0: 4FSK Gain0 = 2, disable 2FSK phase compensation
    sendBuffer[9] = 0xFF;   // MODEM_FSK4_TH: 4FSK slicer threshold = 0xFFFF
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
    sendBuffer[5] = 0x02;   // MODEM_OOK_MISC: OOK uses moving average detector, OOK peak detector
                            // discharge does not affect decay rate, disable OOK squelch, always
                            // discharge peak detector, normal moving average window
    sendBuffer[6] = 0xD6;   // ??
    sendBuffer[7] = 0x83;   // MODEM_RAW_CONTROL
    sendBuffer[8] = 0x00;   // MODEM_RAW_EYE: RAW eye open detector threshold
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


// TODO: there is etl::set, so might as well use it
auto CharToMorseLetter(char c) -> std::uint8_t
{
    // If lowercase -> make uppercase
    if(c >= 'a' and c <= 'z')
    {
        c -= 32;
    }

    switch(c)
    {
        case 'A':
            return morseA;
            break;

        case 'B':
            return morseB;
            break;

        case 'C':
            return morseC;
            break;

        case 'D':
            return morseD;
            break;

        case 'E':
            return morseE;
            break;

        case 'F':
            return morseF;
            break;

        case 'G':
            return morseG;
            break;

        case 'H':
            return morseH;
            break;

        case 'I':
            return morseI;
            break;

        case 'J':
            return morseJ;
            break;

        case 'K':
            return morseK;
            break;

        case 'L':
            return morseL;
            break;

        case 'M':
            return morseM;
            break;

        case 'N':
            return morseN;
            break;

        case 'O':
            return morseO;
            break;

        case 'P':
            return morseP;
            break;

        case 'Q':
            return morseQ;
            break;

        case 'R':
            return morseR;
            break;

        case 'S':
            return morseS;
            break;

        case 'T':
            return morseT;
            break;

        case 'U':
            return morseU;
            break;

        case 'V':
            return morseV;
            break;

        case 'W':
            return morseW;
            break;

        case 'X':
            return morseX;
            break;

        case 'Y':
            return morseY;
            break;

        case 'Z':
            return morseZ;
            break;

        case '1':
            return morse1;
            break;

        case '2':
            return morse2;
            break;

        case '3':
            return morse3;
            break;

        case '4':
            return morse4;
            break;

        case '5':
            return morse5;
            break;

        case '6':
            return morse6;
            break;

        case '7':
            return morse7;
            break;

        case '8':
            return morse8;
            break;

        case '9':
            return morse9;
            break;

        case '0':
            return morse0;
            break;

        case ' ':
            return morseSpace;
            break;

        default:
            RODOS::PRINTF("Invalid character\n");
            return 0xFF;
            break;
    }
}


auto SetTxType(TxType txType) -> void
{
    Byte modulationMode;
    std::uint32_t dataRate;

    if(txType == TxType::morse)
    {
        modulationMode = 0x09_b;  // MODEM_MODE_TYPE: TX data from GPIO0 pin, modulation OOK
        dataRate = 20'000;        // MODEM_DATA_RATE: unused, 20k Baud
    }
    else
    {
        modulationMode = 0x03_b;  // MODEM_MODE_TYPE: TX data from packet handler, modulation 2GFSK
        dataRate = 9600;  // MODEM_DATA_RATE: For 9k6 Baud: (TX_DATA_RATE * MODEM_TX_NCO_MODE *
                          // TXOSR)/F_XTAL_Hz = (9600 * 2600000 * 10)/26000000 = 9600 = 0x002580
    }

    auto propertyValues = std::to_array({modulationMode,
                                         0x00_b,
                                         0x07_b,  // DSM default config
                                         static_cast<Byte>(dataRate >> (2 * CHAR_BIT)),
                                         static_cast<Byte>(dataRate >> (CHAR_BIT)),
                                         static_cast<Byte>(dataRate)});

    SetProperty<std::size(propertyValues)>(
        PropertyGroup::modem, 0x00_b, std::span<Byte, std::size(propertyValues)>(propertyValues));
}


auto PartInfoIsCorrect() -> bool
{
    auto sendBuffer = std::to_array<std::uint8_t>({0x01});
    auto receiveBuffer = std::array<std::uint8_t, 9>{};
    SendCommand(std::data(sendBuffer),
                std::size(sendBuffer),
                std::data(receiveBuffer),
                std::size(receiveBuffer));
    return partInfo == static_cast<std::uint16_t>(receiveBuffer[1] << CHAR_BIT | receiveBuffer[2]);
}


auto GetPartInfo() -> std::uint16_t
{
    auto sendBuffer = std::to_array<Byte>({cmdPartInfo});
    auto responseBuffer = SendCommandWithResponse<partInfoResponseLength>(
        std::span<Byte, std::size(sendBuffer)>(sendBuffer));

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(responseBuffer[1]) << CHAR_BIT
                                      | static_cast<std::uint16_t>(responseBuffer[2]));
}


// TODO: Change to Byte instead of uint8_t
// TODO: Adapt ReadFifo to use spans instead of using pointer arithmetic
// TODO: Discuss transmission of large amounts of data!
auto ReceiveTestData() -> std::array<std::uint8_t, maxRxBytes>
{
    auto sendBuffer = std::array<std::uint8_t, 32>{};

    ClearFifos();

    // Enable RX FiFo Almost Full Interrupt
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x01;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x01;
    sendBuffer[4] = 0b00000001;
    SendCommand(std::data(sendBuffer), 5, nullptr, 0);

    ClearInterrupts();

    // Enter RX Mode
    sendBuffer[0] = 0x32;
    sendBuffer[1] = 0x00;  // "Channel 0"
    sendBuffer[2] = 0x00;  // Start RX now
    sendBuffer[3] = 0x00;  // RX Length = 0
    sendBuffer[4] = 0x00;
    sendBuffer[5] = 0x00;  // Remain in RX state on preamble detection timeout
    sendBuffer[6] = 0x00;  // Do nothing on RX packet valid (we'll never enter this state)
    sendBuffer[7] = 0x00;  // Do nothing on RX packet invalid (we'll never enter this state)
    SendCommand(std::data(sendBuffer), 8, nullptr, 0);

    // Wait for RX FiFo Almost Full Interrupt
    // while(RF_NIRQ::read())
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
        // modm::delay_us(10);
    }

    RODOS::PRINTF("Got RX FiFo Almost Full Interrupt\n");

    auto rxBuffer = std::array<std::uint8_t, maxRxBytes>{0};
    ReadFifo(std::data(rxBuffer), 48);

    RODOS::PRINTF("Retrieved first 48 Byte from FiFo\n");

    ClearInterrupts();

    // Wait for RX FiFo Almost Full Interrupt
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
    }

    RODOS::PRINTF("Got RX FiFo Almost Full Interrupt\n");

    ReadFifo(std::data(rxBuffer) + 48, 48);

    EnterPowerMode(PowerMode::standby);
    ClearInterrupts();

    return rxBuffer;
}


// TODO: Rewrite using span instead of pointer + length
// It could also be helpful to overload this and provide a version for string_view
auto TransmitData(std::uint8_t * data, std::size_t length) -> void
{
    auto dataIndex = 0;
    ClearFifos();

    // Set TX Data Length
    // TODO: Check if we can just set the length in START_TX
    // TODO: Maybe put setting the data length in a function
    auto dataLengthPropertyValues =
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        std::to_array<Byte>({static_cast<Byte>(length >> 8), static_cast<Byte>(length)});
    SetProperty<2>(PropertyGroup::pkt,
                   0x0D_b,
                   std::span<Byte, std::size(dataLengthPropertyValues)>(dataLengthPropertyValues));

    auto nFillBytes = 60;  // Fill the TX FIFO with 60 bytes each "round"
    auto almostEmptyInterruptEnabled = false;

    // While the packet is longer than a single fill round, wait for the almost empty interrupt,
    // afterwards for the packet sent interrupt
    while(length - dataIndex > nFillBytes)
    {
        // Enable the almost empty interrupt in the first round
        if(not almostEmptyInterruptEnabled)
        {
            // TODO: Setting interrupts could be put in a function
            auto interruptPropertyValues = std::to_array<Byte, 1>({0b00000010_b});
            SetProperty<1>(
                PropertyGroup::intCtl, 0x01_b, std::span<Byte, 1>(interruptPropertyValues));
            almostEmptyInterruptEnabled = true;
        }

        // Write nFillBytes bytes to the TX FIFO
        WriteFifo(data + dataIndex, nFillBytes);
        dataIndex += nFillBytes;
        ClearInterrupts();
        StartTx(0);
        // Wait for TX FIFO almost empty interrupt
        while(nirqGpioPin.Read() == hal::PinState::set)
        {
            RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
        }
    }

    // Now enable the packet sent interrupt
    auto interruptPropertyValues = std::to_array<Byte, 1>({0b00100000_b});
    SetProperty<1>(PropertyGroup::intCtl, 0x01_b, std::span<Byte, 1>(interruptPropertyValues));
    ClearInterrupts();
    // Write the rest of the data
    WriteFifo(data + dataIndex, length - dataIndex);
    // Wait for packet sent interrupt
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
    }
    EnterPowerMode(PowerMode::standby);
}


auto TransmitTestData() -> void
{
    const char * txdata =
        "This is a test for looooooooooong transmit that is too large to fit into one FiFo size.";
    uint16_t len = strlen(txdata);
    uint16_t ptr = 0;
    uint8_t sendBuffer[40];

    // Clear FiFo
    sendBuffer[0] = 0x15;
    sendBuffer[1] = 0x03;
    SendCommand(sendBuffer, 2, nullptr, 0);

    // RODOS::PRINTF("Cleared FiFo\n");

    // Set TX Data Length
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x02;
    sendBuffer[3] = 0x0d;
    sendBuffer[4] = len >> 8;
    sendBuffer[5] = (len & 0xFF);
    SendCommand(sendBuffer, 6, nullptr, 0);

    // RODOS::PRINTF("Set TX data Length\n");

    // Write first part to FiFo
    WriteFifo((uint8_t *)(txdata), 60);
    ptr += 60;

    // RODOS::PRINTF("Wrote FiFO\n");

    // Enable TX FiFo Almost Empty Interrupt
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x01;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x01;
    sendBuffer[4] = 0b00000010;
    SendCommand(sendBuffer, 5, nullptr, 0);

    // RODOS::PRINTF("Configured almost empty interrupt\n");

    // Clear Interrupts
    sendBuffer[0] = 0x20;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x00;
    sendBuffer[3] = 0x00;
    SendCommand(sendBuffer, 4, nullptr, 0);

    // RODOS::PRINTF("Cleared interrupts\n");

    // Enter TX Mode
    // Length is set to 0 since we currently control it via a property
    StartTx(0);

    // RODOS::PRINTF("Enter TX Mode\n");

    // Wait for TX FiFo Almost Empty Interrupt
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
    }

    // RODOS::PRINTF("Got Interrupt\n");

    // Enable Packet Sent Interrupt
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x01;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x01;
    sendBuffer[4] = 0b00100000;
    SendCommand(sendBuffer, 5, nullptr, 0);

    // RODOS::PRINTF("Enable Packet Sent Interrupt\n");

    // Clear Interrupts
    ClearInterrupts();

    // RODOS::PRINTF("Cleared Interrupts\n");

    // Send second part of data
    WriteFifo((uint8_t *)(txdata + ptr), len - ptr);

    // RODOS::PRINTF("Wrote second FiFo Part\n");

    auto startTime = RODOS::NOW();

    // Wait for Packet Sent Interrupt
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        if(RODOS::NOW() - startTime > 1 * RODOS::SECONDS)
        {
            // RODOS::PRINTF("TX Timeout\n");
            break;
        }
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
    }

    // RODOS::PRINTF("Packet Sent\n");

    // Enter Standby Mode
    EnterPowerMode(PowerMode::standby);
}


// TODO: Change TX Mode in Morse/Transmit function instead of test
auto Morse(std::string_view message) -> void
{
    for(auto && c : message)
    {
        if(c == ' ')
        {
            AT(NOW() + morseWordOffTime);
            continue;
        }

        auto morseLetter = CharToMorseLetter(c);
        if(morseLetter == 0xFF)
        {
            continue;
        }

        auto letterLength = morseLetter >> maxMorseLetterLength;
        for(auto i = letterLength; i > 0; i--)
        {
            auto bit = (morseLetter >> (i - 1)) & 1;
            auto morseTime = (bit == morseDot ? morseDotTime : morseDashTime);

            gpio0GpioPin.Reset();

            ClearInterrupts();
            StartTx(0);

            gpio0GpioPin.Set();
            AT(NOW() + morseTime);
            gpio0GpioPin.Reset();

            ClearInterrupts();
            EnterPowerMode(PowerMode::standby);

            if(i > 1)
            {
                AT(NOW() + morseBitOffTime);
            }
        }
        AT(NOW() + morseLetterOffTime);
    }
}


auto MorseTest() -> void
{
    auto sendBuffer = std::array<std::uint8_t, 16>{};
    constexpr auto onTime = 500 * MILLISECONDS;
    constexpr auto offTime = 500 * MILLISECONDS;
    constexpr auto nCycles = 5;

    for(auto i = 0; i < nCycles; ++i)
    {
        gpio0GpioPin.Reset();

        ClearInterrupts();
        StartTx(0);

        AT(NOW() + 10 * MILLISECONDS);

        // TODO: Morse with gpio0GpioPin
        gpio0GpioPin.Set();
        AT(NOW() + onTime);
        gpio0GpioPin.Reset();

        ClearInterrupts();
        EnterPowerMode(PowerMode::standby);

        AT(NOW() + offTime);
    }
}


auto StartTx(std::uint16_t length) -> void
{
    auto commandBuffer = std::to_array({cmdStartTx,
                                        0x00_b,
                                        0x30_b,
                                        // NOLINTNEXTLINE(hicpp-signed-bitwise)
                                        static_cast<Byte>(length >> CHAR_BIT),
                                        static_cast<Byte>(length),
                                        0x00_b,
                                        0x00_b});
    SendCommandNoResponse(commandBuffer);
}


auto EnterPowerMode(PowerMode powerMode) -> void
{
    auto commandBuffer = std::to_array<Byte>({cmdChangeState, static_cast<Byte>(powerMode)});
    SendCommandNoResponse(commandBuffer);
}


auto ClearInterrupts() -> void
{
    auto commandBuffer = std::to_array<Byte>({cmdGetIntStatus, 0x00_b, 0x00_b, 0x00_b});
    SendCommandNoResponse(commandBuffer);
}


auto ClearFifos() -> void
{
    auto commandBuffer = std::to_array<Byte>({cmdFifoInfo, 0x03_b});
    SendCommandNoResponse(commandBuffer);
}


// --- Private function definitions ---
auto SendCommandNoResponse(std::span<Byte> commandBuffer) -> void
{
    // RODOS::PRINTF("SendCommandNoResponse()\n");
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    hal::WriteTo(&spi, commandBuffer);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();
    WaitOnCts();
    // No response -> just set the CS pin again
    csGpioPin.Set();
}


template<std::size_t nResponseBytes>
auto SendCommandWithResponse(std::span<Byte> commandBuffer) -> std::array<Byte, nResponseBytes>
{
    // RODOS::PRINTF("SendCommandWithResponse()\n");

    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    hal::WriteTo(&spi, commandBuffer);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();

    auto responseBuffer = std::array<Byte, nResponseBytes>{};
    WaitOnCts();
    // WaitOnCts leaves CS pin low, read response afterwards
    hal::ReadFrom(&spi, std::span<Byte, nResponseBytes>(responseBuffer));
    csGpioPin.Set();

    return responseBuffer;
}


[[deprecated]] auto SendCommand(std::uint8_t * data,
                                std::size_t length,
                                std::uint8_t * responseData,
                                std::size_t responseLength) -> void
{
    // RODOS::PRINTF("SendCommand()\n");
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    spi.write(data, length);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();

    auto cts = std::to_array<uint8_t>({0x00, 0x00});
    auto req = std::to_array<uint8_t>({0x44, 0x00});
    do
    {
        AT(NOW() + 20 * MICROSECONDS);
        csGpioPin.Reset();
        AT(NOW() + 20 * MICROSECONDS);
        spi.writeRead(std::data(req), std::size(req), std::data(cts), std::size(cts));
        if(cts[1] != 0xFF)
        {
            AT(NOW() + 2 * MICROSECONDS);
            csGpioPin.Set();
        }
    } while(cts[1] != 0xFF);

    if(responseLength > 0)
    {
        spi.read(responseData, responseLength);
    }

    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();
}


// TODO: modernize (span instead of pointer + length, our communication abstraction, WaitOnCts())
auto WriteFifo(std::uint8_t * data, std::size_t length) -> void
{
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    auto buf = std::to_array<std::uint8_t>({0x66});
    spi.write(std::data(buf), std::size(buf));
    spi.write(data, length);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();

    auto cts = std::to_array<std::uint8_t>({0x00, 0x00});
    auto req = std::to_array<std::uint8_t>({0x44, 0x00});
    do
    {
        AT(NOW() + 20 * MICROSECONDS);
        csGpioPin.Reset();
        AT(NOW() + 20 * MICROSECONDS);
        spi.writeRead(std::data(req), std::size(req), std::data(cts), std::size(cts));
        AT(NOW() + 2 * MICROSECONDS);
        csGpioPin.Set();
    } while(cts[1] != 0xFF);
}


auto ReadFifo(std::uint8_t * data, std::size_t length) -> void
{
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    auto buf = std::to_array<std::uint8_t>({0x77});
    spi.write(std::data(buf), std::size(buf));
    spi.read(data, length);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();
}


auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void
{
    // RODOS::PRINTF("PowerUp()\n");
    auto powerUpBuffer = std::to_array<Byte>(
        {cmdPowerUp,
         static_cast<Byte>(bootOptions),
         static_cast<Byte>(xtalOptions),
         static_cast<Byte>(xoFrequency >> (CHAR_BIT * 3)),  // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency >> (CHAR_BIT * 2)),  // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency >> (CHAR_BIT)),      // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency)});

    SendCommandNoResponse(powerUpBuffer);
}


//! @brief Polls the CTS byte until 0xFF is received (i.e. Si4463 is ready for command).
auto WaitOnCts() -> void
{
    // RODOS::PRINTF("WaitOnCts()\n");
    auto sendBuffer = std::to_array<Byte>({cmdReadCmdBuff});
    do
    {
        AT(NOW() + 20 * MICROSECONDS);
        csGpioPin.Reset();
        AT(NOW() + 20 * MICROSECONDS);

        hal::WriteTo(&spi, std::span<Byte, std::size(sendBuffer)>(sendBuffer));
        auto ctsBuffer = std::array<Byte, 1>{};
        hal::ReadFrom(&spi, std::span<Byte, std::size(ctsBuffer)>(ctsBuffer));

        if(ctsBuffer[0] != readyCtsByte)
        {
            AT(NOW() + 2 * MICROSECONDS);
            csGpioPin.Set();
        }
        else
        {
            break;
        }
    } while(true);
}


// TODO: This does not need any template stuff, just set the size of setPropertyBuffer to take 12
// (max.) properties and use a subspan afterwards
template<std::size_t nProperties>
    requires(nProperties >= 1 and nProperties <= maxNProperties)
auto SetProperty(PropertyGroup propertyGroup,
                 Byte startProperty,
                 std::span<Byte, nProperties> propertyValues) -> void
{
    auto setPropertyBuffer = std::array<Byte, setPropertyHeaderSize + nProperties>{};

    setPropertyBuffer[0] = cmdSetProperty;
    setPropertyBuffer[1] = static_cast<Byte>(propertyGroup);
    setPropertyBuffer[2] = static_cast<Byte>(nProperties);
    setPropertyBuffer[3] = startProperty;

    auto bufferIndex = setPropertyHeaderSize;
    for(auto && propertyValue : propertyValues)
    {
        setPropertyBuffer[bufferIndex] = propertyValue;
        bufferIndex++;
    }

    SendCommandNoResponse(setPropertyBuffer);
}
}

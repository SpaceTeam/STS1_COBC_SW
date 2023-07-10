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


namespace sts1cobcsw::periphery::rf
{
using sts1cobcsw::serial::operator""_b;
using RODOS::AT;
using RODOS::MICROSECONDS;
using RODOS::MILLISECONDS;
using RODOS::NOW;
using sts1cobcsw::serial::Byte;


// --- Globals ---

auto spi = RODOS::HAL_SPI(hal::rfSpiIndex, hal::rfSpiSckPin, hal::rfSpiMisoPin, hal::rfSpiMosiPin);
// auto spi = RODOS::HAL_SPI(hal::rfSpiIndex);
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


// --- Private function declarations ---

[[deprecated]] auto SendCommand(std::uint8_t * data,
                                std::size_t length,
                                std::uint8_t * responseData,
                                std::size_t responseLength) -> void;

auto SendCommandNoResponse(std::span<Byte> commandBuffer) -> void;

template<std::size_t nResponseBytes>
auto SendCommandWithResponse(std::span<Byte> commandBuffer) -> std::array<Byte, nResponseBytes>;

auto WriteFifo(std::uint8_t * data, std::size_t length) -> void;

auto ReadFifo(std::uint8_t * data, std::size_t length) -> void;

// auto PowerUp(PowerUpBootOptions bootOptions,
//              PowerUpXtalOptions xtalOptions,
//              std::uint32_t xoFrequency) -> void;

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
    sendBuffer[4] = 0x52;
    sendBuffer[5] = 0x00;
    SendCommand(data(sendBuffer), 6, nullptr, 0);

    // RF Global Config 1
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x03;
    sendBuffer[4] = 0x60;
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Int Ctl Enable
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x01;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x01;
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // TX Preamble Length
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x10;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x08;  // 8 bytes preamble
    sendBuffer[5] = 0x14;  // Normal sync timeout, 14 bit preamble RX threshold
    sendBuffer[6] = 0x00;  // No non-standard preamble pattern
    sendBuffer[7] = 0x0F;  // No extended RX preamble timeout, 0x0f nibbles timeout until detected
                           // preamble is discarded as invalid
    sendBuffer[8] =
        0x31;  // First transmitted preamble bit is 1, unit of preampreamble TX length is in bytes
    sendBuffer[9] = 0x00;   // Non-standard pattern
    sendBuffer[10] = 0x00;  // Non-standard pattern
    sendBuffer[11] = 0x00;  // Non-standard pattern
    sendBuffer[12] = 0x00;  // Non-standard pattern
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // Sync word config
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x11;
    sendBuffer[2] = 0x05;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x43;  // Allow 4 bit sync word errors, 4 byte sync word
    sendBuffer[5] =
        0b01011000;        // Valid CCSDS TM sync word for Reed-Solomon or convolutional coding
    sendBuffer[6] = 0b11110011;  // Be careful: Send order is MSB-first but Little endian so the
                                 // lowest bit of the
    sendBuffer[7] =
        0b00111111;  // highest byte is transmitted first, which is different to how the CCSDS spec
    sendBuffer[8] = 0b10111000;  // annotates those bit patterns!
    SendCommand(data(sendBuffer), 9, nullptr, 0);

    // CRC Config
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x00;  // No CRC
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // Whitening and Packet Parameters
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x04;
    sendBuffer[3] = 0x03;
    sendBuffer[4] = 0xFF;
    sendBuffer[5] = 0xFF;
    sendBuffer[6] = 0x00;  // Disable whitening
    sendBuffer[7] =
        0x01;  // Don't split RX and TX field information (length, ...), enable RX packet handler,
               // use normal (2)FSK, no Manchester coding, no CRC, data transmission with MSB first.
    SendCommand(data(sendBuffer), 8, nullptr, 0);

    // Pkt Length part 1
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x08;
    sendBuffer[4] = 0x60;  // Infinite receive, big endian (MSB first)
    sendBuffer[5] = 0x00;
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x30;  // Trigger TX FiFo almost empty interrupt when 0x30 bytes in FiFo (size
                           // 0x40) are empty
    sendBuffer[8] =
        0x30;  // Trigger RX FiFo almost full interrupt when 0x30 bytes in FiFo (size 0x40) are full
    sendBuffer[9] = 0x00;   // Field 1 len
    sendBuffer[10] = 0x00;  // Field 1 len
    sendBuffer[11] = 0x04;  // Field 1 config
    sendBuffer[12] = 0x80;  // Field 1 CRC config
    sendBuffer[13] = 0x00;  // Field 2 len
    sendBuffer[14] = 0x00;  // Field 2 len
    sendBuffer[15] = 0x00;  // Field 2 config
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // Pkt Length part 2
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x14;
    sendBuffer[4] = 0x00;
    sendBuffer[5] = 0x00;
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;
    sendBuffer[8] = 0x00;
    sendBuffer[9] = 0x00;
    sendBuffer[10] = 0x00;
    sendBuffer[11] = 0x00;
    sendBuffer[12] = 0x00;
    sendBuffer[13] = 0x00;
    sendBuffer[14] = 0x00;
    sendBuffer[15] = 0x00;
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // Pkt Length part 3
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x20;
    sendBuffer[4] = 0x00;
    sendBuffer[5] = 0x00;
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;
    sendBuffer[8] = 0x00;
    sendBuffer[9] = 0x00;
    sendBuffer[10] = 0x00;
    sendBuffer[11] = 0x00;
    sendBuffer[12] = 0x00;
    sendBuffer[13] = 0x00;
    sendBuffer[14] = 0x00;
    sendBuffer[15] = 0x00;
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // Pkt Length part 4
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x2C;
    sendBuffer[4] = 0x00;
    sendBuffer[5] = 0x00;
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;
    sendBuffer[8] = 0x00;
    sendBuffer[9] = 0x00;
    sendBuffer[10] = 0x00;
    sendBuffer[11] = 0x00;
    sendBuffer[12] = 0x00;
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // RF Modem Mod Type
    SetTxType(txType);
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x07;
    sendBuffer[3] = 0x06;  // SetTxType sets modem properties from 0x00 to 0x05
    sendBuffer[4] = 0x01;  // Modem TX NCO mode default values
    sendBuffer[5] = 0x27;
    sendBuffer[6] = 0xAC;
    sendBuffer[7] = 0x40;
    // FSK deviation has to be at least one-fourth of the bit rate. So in this case 9600/4 =
    // 2400Hz
    // register value is calculated with (2^19 * outdiv * deviation_Hz)/(N_presc * F_xo) =
    // 167.772 =
    // 168 = 0x0000A8 9k6 deviation = 671 = 0x29F
    sendBuffer[8] = 0x00;   // Modem frequency deviation MSB
    sendBuffer[9] = 0x1E;
    sendBuffer[10] = 0x3F;  // Modem frequency deviation LSB
    SendCommand(data(sendBuffer), 11, nullptr, 0);

    // RF Modem Mod Type
    // sendBuffer[0] = 0x11;
    // sendBuffer[1] = 0x20;
    // sendBuffer[2] = 0x0C;
    // sendBuffer[3] = 0x00;
    // sendBuffer[4] = 0x09;  // TX data direct mode from GPIO0 pin, modulation OOK
    // // sendBuffer[4] = 0x01;  // TX data from TX FIFO, modulation OOK
    // sendBuffer[5] = 0x00;
    // sendBuffer[6] = 0x07;  // DSM default config
    // sendBuffer[7] = 0x00;  // Modem data rate 20kbaud (unused in direct mode)
    // sendBuffer[8] = 0x4E;
    // sendBuffer[9] = 0x20;
    // sendBuffer[10] = 0x01;  // Modem TX NCO mode default values
    // sendBuffer[11] = 0xC9;
    // sendBuffer[12] = 0xC3;
    // sendBuffer[13] = 0x80;
    // // FSK deviation has to be at least one-fourth of the bit rate. So in this case 9600/4 =
    // // 2400Hz
    // // register value is calculated with (2^19 * outdiv * deviation_Hz)/(N_presc * F_xo) =
    // // 167.772 =
    // // 168 = 0x0000A8 9k6 deviation = 671 = 0x29F
    // sendBuffer[14] = 0x00;  // Modem frequency deviation MSB
    // sendBuffer[15] = 0x02;
    // SendCommand(data(sendBuffer), 16, nullptr, 0);

    // // RF Modem Freq Deviation continuation
    // sendBuffer[0] = 0x11;
    // sendBuffer[1] = 0x20;
    // sendBuffer[2] = 0x01;
    // sendBuffer[3] = 0x0C;
    // sendBuffer[4] = 0x9F;  // Modem frequency deviation LSB
    // SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Modem TX Ramp Delay, Modem MDM Ctrl, Modem IF Ctrl, Modem IF Freq & Modem Decimation Cfg
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x08;
    sendBuffer[3] = 0x18;
    sendBuffer[4] = 0x01;  // Ramp Delay 1
    sendBuffer[5] = 0x80;  // Slicer phase source from detector's output
    sendBuffer[6] = 0x08;  // No ETSI mode, fixed IF mode, normal IF mode (nonzero IF)
    sendBuffer[7] = 0x03;  // IF frequency 0x038000
    sendBuffer[8] = 0x80;
    sendBuffer[9] = 0x00;
    sendBuffer[10] =
        0x70;  // Decimation NDEC0 = 0, NDEC1 = decimation by 8, NDEC2 = decimation by 2
    sendBuffer[11] =
        0x20;  // Normal decimate-by-8 filter gain, don't bypass the decimate-by-2 polyphase filter,
               // bypass the decimate-by-3 polyphase filter, enable droop compensation, channel
               // selection filter in normal mode (27 tap filter)
    SendCommand(data(sendBuffer), 12, nullptr, 0);

    // RF Modem BCR Oversampling Rate, Modem BCR NCO Offset, Modem BCR Gain, Modem BCR Gear & Modem
    // BCR Misc
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x22;
    sendBuffer[4] = 0x03;  // RX symbol oversampling rate of 0x30D/8 = 781/8 = 97.625
    sendBuffer[5] = 0x0D;  // (According to the datasheet usual values are in the range of 8 to 12
                           // where this value seems to be odd?)
    sendBuffer[6] = 0x00;  // BCR NCO offset of 0x00A7C6/64 = 42950/64 = 671.09375
    sendBuffer[7] = 0xA7;
    sendBuffer[8] = 0xC6;
    sendBuffer[9] = 0x00;   // BCR gain 0x054 = 84
    sendBuffer[10] = 0x54;
    sendBuffer[11] = 0x02;  // BCR loop gear control. CRSLOW=2, CRFAST=0
    sendBuffer[12] =
        0xC2;  // Stop NCO for one sample clock in BCR mid-point phase sampling condition to escape,
               // disable NCO resetting in case of mid-point phase sampling condition, don't double
               // BCR loop gain, BCR NCO compensation is sampled upon detection of the preamble end,
               // disable NCO frequency compensation, bypass compensation term feedback to slicer,
               // bypass compensation term feedback to BCR tracking loop
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // RF Modem AFC Gear, Modem AFC Wait, Modem AFC Gain, Modem AFC Limiter & Modem AFC Misc
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x07;
    sendBuffer[3] = 0x2C;
    sendBuffer[4] =
        0x04;  // AFC_SLOW gain 4, AFC_FAST gain 0, Switch gear after detection of preamble
    sendBuffer[5] = 0x36;  // LGWAIT = 6, SHWAIT = 3
    sendBuffer[6] = 0x80;  // AFC loop gain = 0x003, don't half the loop gain, disable adaptive RX
                           // bandwidth, enable frequency error estimation
    sendBuffer[7] = 0x03;
    sendBuffer[8] = 0x30;  // AFC limiter = 0x30AF
    sendBuffer[9] = 0xAF;
    sendBuffer[10] =
        0x80;  // Expected freq error is less then 12*symbol rate, AFC correction of PLL will be
               // frozen if a consecutive string of 1s or 0s that exceed the search period is
               // encountered, don't switch clock source for frequency estimator, don't freeze AFC
               // at preamble end, AFC correction uses freq estimation by moving average or minmax
               // detector in async demod,disable AFC value feedback to PLL, freeze AFC after gear
               // switching
    SendCommand(data(sendBuffer), 11, nullptr, 0);

    // RF Modem AGC Control
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x35;
    sendBuffer[4] =
        0xE2;  // reset peak detectors only on change of gain indicated by peak detector output,
               // reduce ADC gain when AGC gain is at minimum, normal AGC speed, don't increase AGC
               // gain during signal reductions in ant diversity mode, always perform gain decreases
               // in 3dB steps instead of 6dB steps, AGC is enabled over whole packet length
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Modem AGC Window Size, AGC RF Peak Detector Decay, AGC IF Peak Detector Decay, 4FSK Gain,
    // 4FSK Slicer Threshold, 4FSK SYmbol Mapping Code, OOK Attack/Decay Times
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x38;
    sendBuffer[4] =
        0x11;  // AGC gain settling window size = 1, AGC signal level measurement window = 1
    sendBuffer[5] = 0xAB;   // RF peak detector decay time = 0xAB = 171
    sendBuffer[6] = 0xAB;   // IF peak detector decay time = 0xAB = 171
    sendBuffer[7] = 0x00;   // 4FSK Gain1 = 0, Normal second phase compensation factor
    sendBuffer[8] = 0x02;   // 4FSK Gain0 = 2, disable 2FSK phase compensation
    sendBuffer[9] = 0xFF;   // 4FSK slicer threshold = 0xFFFF
    sendBuffer[10] = 0xFF;
    sendBuffer[11] = 0x00;  // 4FSK symbol map 0 (`00 `01 `11 `10)
    sendBuffer[12] = 0x2B;  // OOK decay = 11, OOK attack = 2
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // RF Modem OOK Control, OOK Misc, RAW Search, RAW Control, RAW Eye, Antenna Diversity Mode,
    // Antenna Diversity Control, RSSI Threshold
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x09;
    sendBuffer[3] = 0x42;
    sendBuffer[4] = 0xA4;  // OOK Squelch off, OOK slicer output de-glitching by bit clock, raw
                           // output is synced to clock, MA_FREQUDOWN=0, AGC and OOK movign average
                           // detector threshold will be frozen after preamble detection, S2P_MAP=2
    sendBuffer[5] = 0x02;  // OOK uses moving average detector, OOK peak detector discharge does not
                           // affect decay rate, disable OOK squelch, always discharge peak
                           // detector, normal moving average window
    sendBuffer[6] = 0xD6;
    sendBuffer[7] = 0x83;  // RAW mode control
    sendBuffer[8] = 0x00;  // RAW eye open detector threshold
    sendBuffer[9] = 0xAD;
    sendBuffer[10] = 0x01;  // Antenna diversity mode
    sendBuffer[11] = 0x80;  // Antenna diversity control
    sendBuffer[12] = 0xFF;  // Threshold for clear channel assessment and RSSI interrupt generation
    SendCommand(data(sendBuffer), 13, nullptr, 0);

    // RF Modem RSSI Control
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x4C;
    sendBuffer[4] = 0x00;  // Disable RSSI latch, RSSI value is avg over last 4*Tb bit periods,
                           // disable RSSI threshold check after latch
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Modem RSSI Compensation
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x4E;
    sendBuffer[4] = 0x40;  // Compensation/offset of measured RSSI value
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RF Modem Clock generation Band
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x20;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x51;
    sendBuffer[4] =
        0x0A;  // Band = FVCO_DIV_8, high performance mode fixed prescaler div2, force recalibration
    SendCommand(data(sendBuffer), 5, nullptr, 0);

    // RX Filter Coefficients
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
    sendBuffer[4] = 0x08;  // PA switching amp mode, PA_SEL = HP_COARSE, disable power sequencing,
                           // disable external TX ramp signal
    // TODO: Tune output power to fully utilize the amplifier without clipping
    // TODO: 0x1A for field test!
    sendBuffer[5] = 0x07;  // Enabled PA fingers (sets output power but not linearly). See page 35
                           // of the Si446x datasheet. We need around 6dBm to go close to the
                           // maximum of our amplifier (33dBm max output, ~20-27dBm gain on 433MHz).
    sendBuffer[6] = 0x00;  // 10µA bias current per enabled finger, complementary drive signal with
                           // 50% duty cycle
    sendBuffer[7] = 0x3D;  // Ramping time constant = 0x1d, FSK modulation delay 6µs
    SendCommand(data(sendBuffer), 8, nullptr, 0);

    // RF Synth Feed Forward Charge Pump Current, Integrated Charge Pump Current, VCO Gain Scaling
    // Factor, FF Loop Filter Values
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x23;
    sendBuffer[2] = 0x07;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x2C;  // FF charge pump current = 60µA
    sendBuffer[5] = 0x0E;  // Int charge pump current = 30µA
    sendBuffer[6] =
        0x0B;  // Set VCO scaling factor to maximum value, set tuning varactor gain to maximum value
    sendBuffer[7] = 0x04;   // R2 value 90kOhm
    sendBuffer[8] = 0x0C;   // C2 value 11.25pF
    sendBuffer[9] = 0x73;   // C3 value 12pF, C1 offset 0pF, C1 value 7.21pF
    sendBuffer[10] = 0x03;  // FF amp bias current 100µA
    SendCommand(data(sendBuffer), 11, nullptr, 0);

    // RF Match Mask
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x30;
    sendBuffer[2] = 0x0C;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x00;
    sendBuffer[5] = 0x00;
    sendBuffer[6] = 0x00;
    sendBuffer[7] = 0x00;
    sendBuffer[8] = 0x00;
    sendBuffer[9] = 0x00;
    sendBuffer[10] = 0x00;
    sendBuffer[11] = 0x00;
    sendBuffer[12] = 0x00;
    sendBuffer[13] = 0x00;
    sendBuffer[14] = 0x00;
    sendBuffer[15] = 0x00;
    SendCommand(data(sendBuffer), 16, nullptr, 0);

    // Frequency Control
    // sendBuffer[0] = 0x11;
    // sendBuffer[1] = 0x40;
    // sendBuffer[2] = 0x08;
    // sendBuffer[3] = 0x00;
    // sendBuffer[4] = 0x38;  // FC_inte = 0x38 = 56
    // sendBuffer[5] = 0x0E;  // FC_frac = 0x0E6666 = 943718
    // sendBuffer[6] = 0x66;
    // sendBuffer[7] = 0x66;
    // // N_presc = 2, outdiv = 8, F_xo = 30MHz
    // // RF_channel_Hz = (FC_inte + FC_frac/2^19)*((N_presc*F_xo)/outdiv) = 433.499994 MHz
    // sendBuffer[8] = 0x44;   // Channel step size = 0x4444
    // sendBuffer[9] = 0x44;
    // sendBuffer[10] = 0x20;  // Window gating period (in number of crystal clock cycles) = 32
    // sendBuffer[11] = 0xFE;  // Adjust target mode for VCO calibration in RX mode = 0xFE int8_t
    // SendCommand(data(sendBuffer), 12, nullptr, 0);

    // Frequency Control
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x40;
    sendBuffer[2] = 0x08;
    sendBuffer[3] = 0x00;
    sendBuffer[4] = 0x41;  // FC_inte = 0x41
    sendBuffer[5] = 0x0E;  // FC_frac. 0xD89D9 = 433.5, 0xEC4EC = 434.5
    sendBuffer[6] = 0xC4;
    sendBuffer[7] = 0xEC;
    // N_presc = 2, outdiv = 8, F_xo = 26MHz
    // RF_channel_Hz = (FC_inte + FC_frac/2^19)*((N_presc*F_xo)/outdiv) = 433.5000048MHz MHz
    sendBuffer[8] = 0x44;   // Channel step size = 0x4444
    sendBuffer[9] = 0x44;
    sendBuffer[10] = 0x20;  // Window gating period (in number of crystal clock cycles) = 32
    sendBuffer[11] = 0xFE;  // Adjust target mode for VCO calibration in RX mode = 0xFE int8_t
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


auto SetTxType(TxType txType) -> void
{
    Byte modulationMode;
    std::uint32_t dataRate;

    if(txType == TxType::morse)
    {
        modulationMode = 0x09_b;  // Direct, OOK
        dataRate = 20'000;        // Baud
    }
    else
    {
        RODOS::PRINTF("FSK\n");
        modulationMode = 0x03_b;  // Packet, 2GFSK
        dataRate = 9600;          // Baud
    }

    auto propertyValues = std::to_array({modulationMode,
                                         0x00_b,
                                         0x07_b,  // DSM default config
                                         0x00_b,
                                         0x25_b,
                                         0x80_b});

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
    // RODOS::PRINTF("GetPartInfo()\n");
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

    RODOS::PRINTF("Cleared FiFo\n");

    // Set TX Data Length
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x12;
    sendBuffer[2] = 0x02;
    sendBuffer[3] = 0x0d;
    sendBuffer[4] = len >> 8;
    sendBuffer[5] = (len & 0xFF);
    SendCommand(sendBuffer, 6, nullptr, 0);

    RODOS::PRINTF("Set TX data Length\n");

    // Write first part to FiFo
    WriteFifo((uint8_t *)(txdata), 60);
    ptr += 60;

    RODOS::PRINTF("Wrote FiFO\n");

    // Enable TX FiFo Almost Empty Interrupt
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x01;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x01;
    sendBuffer[4] = 0b00000010;
    SendCommand(sendBuffer, 5, nullptr, 0);

    RODOS::PRINTF("Configured almost empty interrupt\n");

    // Clear Interrupts
    sendBuffer[0] = 0x20;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x00;
    sendBuffer[3] = 0x00;
    SendCommand(sendBuffer, 4, nullptr, 0);

    RODOS::PRINTF("Cleared interrupts\n");

    // Enter TX Mode
    StartTx(0);
    // sendBuffer[0] = 0x31;
    // sendBuffer[1] = 0x00;
    // sendBuffer[2] = 0x30;
    // sendBuffer[3] = 0x00;
    // sendBuffer[4] = 0x00;
    // SendCommand(sendBuffer, 5, nullptr, 0);

    RODOS::PRINTF("Enter TX Mode\n");

    // Wait for TX FiFo Almost Empty Interrupt
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
    }

    RODOS::PRINTF("Got Interrupt\n");

    // Enable Packet Sent Interrupt
    sendBuffer[0] = 0x11;
    sendBuffer[1] = 0x01;
    sendBuffer[2] = 0x01;
    sendBuffer[3] = 0x01;
    sendBuffer[4] = 0b00100000;
    SendCommand(sendBuffer, 5, nullptr, 0);

    RODOS::PRINTF("Enable Packet Sent Interrupt\n");

    // Clear Interrupts
    ClearInterrupts();

    RODOS::PRINTF("Cleared Interrupts\n");

    // Send second part of data
    WriteFifo((uint8_t *)(txdata + ptr), len - ptr);

    RODOS::PRINTF("Wrote second FiFo Part\n");

    auto startTime = RODOS::NOW();

    // Wait for Packet Sent Interrupt
    while(nirqGpioPin.Read() == hal::PinState::set)
    {
        if(RODOS::NOW() - startTime > 1 * RODOS::SECONDS)
        {
            RODOS::PRINTF("TX Timeout\n");
            break;
        }
        RODOS::AT(RODOS::NOW() + 10 * RODOS::MICROSECONDS);
    }

    RODOS::PRINTF("Packet Sent\n");

    // Enter Standby Mode
    EnterPowerMode(PowerMode::standby);
}


auto Morse() -> void
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
    // TODO: Replace with RODOS SPI HAL functions
    // SpiMaster4::transferBlocking(data, nullptr, length);
    // spi.writeRead(data, length, nullptr, 0);
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
        // TODO: Replace with RODOS SPI HAL functions
        // SpiMaster4::transferBlocking(req, cts, 2);
        spi.writeRead(std::data(req), std::size(req), std::data(cts), std::size(cts));
        if(cts[1] != 0xFF)
        {
            AT(NOW() + 2 * MICROSECONDS);
            csGpioPin.Set();
        }
    } while(cts[1] != 0xFF);

    // TODO: Replace with RODOS SPI HAL functions
    // SpiMaster4::transferBlocking(nullptr, resp_data, resp_len);
    // spi.writeRead(nullptr, 0, responseData, responseLength);
    if(responseLength > 0)
    {
        spi.read(responseData, responseLength);
    }

    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();
}


auto WriteFifo(std::uint8_t * data, std::size_t length) -> void
{
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    auto buf = std::to_array<std::uint8_t>({0x66});
    // TODO: Replace with RODOS SPI HAL functions
    // SpiMaster4::transferBlocking(buf, nullptr, 1);
    spi.write(std::data(buf), std::size(buf));
    // SpiMaster4::transferBlocking(data, nullptr, length);
    spi.write(data, length);
    // spi.writeRead(data, length, nullptr, 0);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();

    auto cts = std::to_array<std::uint8_t>({0x00, 0x00});
    auto req = std::to_array<std::uint8_t>({0x44, 0x00});
    do
    {
        AT(NOW() + 20 * MICROSECONDS);
        csGpioPin.Reset();
        AT(NOW() + 20 * MICROSECONDS);
        // TODO: Replace with RODOS SPI HAL functions
        // SpiMaster4::transferBlocking(req, cts, 2);
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
    // TODO: Replace with RODOS SPI HAL functions
    // SpiMaster4::transferBlocking(buf, nullptr, 1);
    // spi.writeRead(std::data(buf), std::size(buf), nullptr, 0);
    spi.write(std::data(buf), std::size(buf));
    // SpiMaster4::transferBlocking(nullptr, data, length);
    // spi.writeRead(nullptr, 0, data, length);
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
        // RODOS::PRINTF("CTS: %x\n", static_cast<std::uint8_t>(ctsBuffer[0]));

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

    // RODOS::PRINTF("CTS ready\n");
}


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

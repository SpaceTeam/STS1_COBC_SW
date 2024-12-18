#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Eps.hpp>
#include <Sts1CobcSw/Periphery/Spis.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/FlatArray.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/difference.hpp>

#include <bit>
#include <cstddef>


namespace sts1cobcsw::eps
{
// TODO: ask David F.
// STS_EPS_ADCS schematics do not match Wiki description
// EPS ADC channel info:
// Pin   | ADC4 (CS1)
//--------------------------------
// AIN0  | Panel Y+ Cell 1 Voltage
// AIN1  | Panel Y+ Cell 2 Voltage
// AIN2  | Panel Y- Cell 1 Voltage
// AIN3  | Panel Y- Cell 2 Voltage
// AIN4  | Panel X+ Cell 1 Voltage
// AIN5  | Panel X- Cell 1 Voltage
// AIN6  | Panel Z- Cell 1 Voltage
// AIN7  | Panel Z- Cell 2 Voltage
// AIN8  | Panel Y+ Cell 1 Current
// AIN9  | Panel Y+ Cell 2 Current
// AIN10 | Panel Y- Cell 1 Current
// AIN11 | Panel Y- Cell 2 Current
// AIN12 | Panel X+ Cell 1 Current
// AIN13 | Panel X- Cell 1 Current
// AIN14 | Panel Z- Cell 1 Current
// AIN15 | Panel Z- Cell 2 Current

// Pin   | ADC5 (CS2)
//--------------------------------
// AIN0  | BATT_SCALED
// AIN1  | Battery Center Tap
// AIN2  | Battery Pack Current
// AIN3  | Battery Temperature
// AIN4  | Panel Y+ Temperature
// AIN5  | Panel Y- Temperature
// AIN6  | Panel X+ Temperature
// AIN7  | Panel X- Temperature
// AIN8  | Panel Z- Temperature
// AIN9  | VOUT_I
// AIN10 | GND
// AIN11 | GND
// AIN12 | GND
// AIN13 | GND
// AIN14 | GND
// AIN15 | BATT_RAW_SCALED

// Pin   | ADC6 (CS3)
//--------------------------------
// AIN0  | MPPT Bus Current
// AIN1  | Panel X+ MPPT 1 Current
// AIN2  | Panel Y+ MPPT 2 Current
// AIN3  | Panel Y- MPPT 1 Current
// AIN4  | Panel Y- MPPT 2 Current
// AIN5  | Panel Y+ MPPT 1 Current
// AIN6  | Panel X- MPPT 1 Current
// AIN7  | Panel Z- MPPT 1 Current
// AIN8  | Panel Z- MPPT 2 Current
// AIN9  | MPPT Bus Voltage
// AIN10 | GND
// AIN11 | GND
// AIN12 | GND
// AIN13 | GND
// AIN14 | GND
// AIN15 | GND

using AdcValues = std::array<AdcValue, nChannels>;


enum class ResetType
{
    registers,
    fifo
};


// --- Private globals ---

constexpr auto spiTimeout = 1 * ms;

auto adc4CsGpioPin = hal::GpioPin(hal::epsAdc4CsPin);
auto adc5CsGpioPin = hal::GpioPin(hal::epsAdc5CsPin);
auto adc6CsGpioPin = hal::GpioPin(hal::epsAdc6CsPin);


// --- Private function declarations ---

auto ConfigureSetupRegister(hal::GpioPin * adcCsPin) -> void;
auto ConfigureAveragingRegister(hal::GpioPin * adcCsPin) -> void;
auto ReadAdc(hal::GpioPin * adcCsPin) -> AdcValues;
auto Reset(hal::GpioPin * adcCsPin, ResetType resetType) -> void;


// --- Public function definitions ---

auto Initialize() -> void
{
    adc4CsGpioPin.Direction(hal::PinDirection::out);
    adc4CsGpioPin.Set();
    adc5CsGpioPin.Direction(hal::PinDirection::out);
    adc5CsGpioPin.Set();
    adc6CsGpioPin.Direction(hal::PinDirection::out);
    adc6CsGpioPin.Set();

    static constexpr auto baudrate = 6'000'000;
    Initialize(&framEpsSpi, baudrate);

    // Setup ADCs
    ConfigureSetupRegister(&adc4CsGpioPin);
    ConfigureSetupRegister(&adc5CsGpioPin);
    ConfigureSetupRegister(&adc6CsGpioPin);

    // Set averaging mode
    ConfigureAveragingRegister(&adc4CsGpioPin);
    ConfigureAveragingRegister(&adc5CsGpioPin);
    ConfigureAveragingRegister(&adc6CsGpioPin);
}


auto Read() -> SensorValues
{
    auto adc4Values = ReadAdc(&adc4CsGpioPin);
    auto adc5Values = ReadAdc(&adc5CsGpioPin);
    auto adc6Values = ReadAdc(&adc6CsGpioPin);
    return FlatArray(adc4Values, adc5Values, adc6Values);
}


auto ResetAdcRegisters() -> void
{
    Reset(&adc4CsGpioPin, ResetType::registers);
    Reset(&adc5CsGpioPin, ResetType::registers);
    Reset(&adc6CsGpioPin, ResetType::registers);
}


auto ClearFifos() -> void
{
    Reset(&adc4CsGpioPin, ResetType::fifo);
    Reset(&adc5CsGpioPin, ResetType::fifo);
    Reset(&adc6CsGpioPin, ResetType::fifo);
}


// --- Private function definitions ---

auto ConfigureSetupRegister(hal::GpioPin * adcCsPin) -> void
{
    // Setup register values
    // [7:6]: Register selection bits = 0b01
    // [5:4]: Clock mode and CNVST configuration
    // [3:2]: Reference mode configuration
    // [1:0]: Don't care

    static constexpr auto setupRegister = 0b01_b;
    // Don't use CNVST modes, we use the analog configuration for the CNVST pin
    // Therefore clock mode = 0b10 << 4: No CNVST, internal clock
    static constexpr auto clockMode = 0b10_b;
    // Reference off after scan; need wake-up delay: 0b00
    // Reference always on; no wake-up delay: 0b10
    static constexpr auto referenceMode = 0b00_b;
    static constexpr auto setupData =
        (setupRegister << 6) | (clockMode << 4) | (referenceMode << 2);

    // Changing to CNVST mode could be bad (analog input in digital pin)
    // So statically check that bit 5 is set to 1
    static_assert((setupData & (1_b << 5)) > 0_b);  // NOLINT(*magic-numbers*)

    adcCsPin->Reset();
    hal::WriteTo(&framEpsSpi, Span(setupData), spiTimeout);
    adcCsPin->Set();
}


auto ConfigureAveragingRegister(hal::GpioPin * adcCsPin) -> void
{
    // Averaging register values
    // [7:5]: Register selection bits = 0b001
    // [4]:   Averaging on/off
    // [3:2]: Number of conversions used
    // [1:0]: Single-channel scan count (scan mode 0b10 in conversion only)
    static constexpr auto averagingRegister = 0b001_b;
    static constexpr auto enableAveraging = 0b1_b;
    // Use max. number of averages (32)
    static constexpr auto nAverages = 0b11_b;
    // Probably not relevant, leave on 4 results
    static constexpr auto nSingleScans = 0b00_b;
    static constexpr auto averagingData =
        (averagingRegister << 5) | (enableAveraging << 4) | (nAverages << 2) | nSingleScans;
    adcCsPin->Reset();
    hal::WriteTo(&framEpsSpi, Span(averagingData), spiTimeout);
    adcCsPin->Set();
}


auto ReadAdc(hal::GpioPin * adcCsPin) -> AdcValues
{
    // Conversion register values
    // [7]:   Register selection bit = 0b1
    // [6:3]: Channel select
    // [2:1]: Scan mode
    // [1]:   Don't care
    static constexpr auto conversionRegister = 0b1_b;
    // Select highest channel, since we scan through all of them every time
    static constexpr auto channel = 0b1111_b;
    // Scan through channel 0 to N (set in channel select) -> All channels in our case
    static constexpr auto scanMode = 0b00_b;
    static constexpr auto conversionCommand =
        (conversionRegister << 7) | (channel << 3) | (scanMode << 1);
    adcCsPin->Reset();
    hal::WriteTo(&framEpsSpi, Span(conversionCommand), spiTimeout);
    adcCsPin->Set();

    // According to the datasheet at most 514 conversions are done after a conversion command
    // (depends on averaging and channels). This takes 514 * (t_acq + t_conv) + wakeup = 514 * (0.6
    // + 3.5) us + 65 us = 2172.4 us.
    static constexpr auto conversionTime = 3 * ms;
    SuspendFor(conversionTime);

    // Resolution is 12 bit, sent like this: [0 0 0 0 MSB x x x], [x x x x x x x LSB]
    auto adcData = SerialBuffer<AdcValues>{};
    adcCsPin->Reset();
    hal::ReadFrom(&framEpsSpi, Span(&adcData), spiTimeout);
    adcCsPin->Set();
    return Deserialize<std::endian::big, AdcValues>(Span(adcData));
}


// Reset either ADC registers to power-up configuration or clear the FIFO
auto Reset(hal::GpioPin * adcCsPin, ResetType resetType) -> void
{
    // Reset register values
    // [7:4]: Register selection bits = 0b0001
    //   [3]: Reset bit: 0 -> registers, 1 -> FIFO
    // [2:0]: Don't care
    auto data = 0b0001_b << 4;
    data = resetType == ResetType::fifo ? data | 1_b << 3 : data;
    adcCsPin->Reset();
    WriteTo(&framEpsSpi, Span(data), spiTimeout);
    adcCsPin->Set();
}
}

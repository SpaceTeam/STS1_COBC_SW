#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Eps.hpp>
#include <Sts1CobcSw/Periphery/FramEspSpi.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

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


enum class ResetType
{
    registers,
    fifo
};


// --- Private globals ---

// Pins and SPI

auto adc4CsGpioPin = hal::GpioPin(hal::epsAdc4CsPin);
auto adc5CsGpioPin = hal::GpioPin(hal::epsAdc5CsPin);
auto adc6CsGpioPin = hal::GpioPin(hal::epsAdc6CsPin);


// --- Private function declarations ---

auto ConfigureSetupRegister(hal::GpioPin * adcCsPin, std::int64_t timeout = RODOS::END_OF_TIME)
    -> void;
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

    constexpr auto baudrate = 6'000'000;
    Initialize(&spi, baudrate);

    // Setup ADCs
    ConfigureSetupRegister(&adc4CsGpioPin);
    ConfigureSetupRegister(&adc5CsGpioPin);
    ConfigureSetupRegister(&adc6CsGpioPin);
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

auto ConfigureSetupRegister(hal::GpioPin * adcCsPin, std::int64_t timeout) -> void
{
    // Setup register values
    // [7:6]: Register selection bits = 0b01
    // [5:4]: Clock mode and CNVST configuration
    //      Don't use CNVST modes, we use the analog configuration for the CNVST pin
    //      Therefore clock mode = 0b10 << 4: No CNVST, internal clock
    // [3:2]: Reference mode configuration
    //      Reference off after scan; need wake-up delay: 0b00
    //      Reference always on; no wake-up delay: 0b10
    // [1:0]: Don't care

    constexpr auto setupRegister = 0b01_b;
    constexpr auto clockMode = 0b10_b;
    constexpr auto referenceMode = 0b00_b;
    constexpr auto setupData = (setupRegister << 6) | (clockMode << 4) | (referenceMode << 2);

    // Changing to CNVST mode could be bad (analog input in digital pin)
    // So statically check that bit 5 is set to 1
    static_assert((setupData & (1_b << 5)) > 0_b);  // NOLINT(*magic-numbers*)

    adcCsPin->Reset();
    hal::WriteTo(&spi, Span(setupData), timeout);
    adcCsPin->Set();
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
    hal::WriteTo(&spi, Span(data));
    adcCsPin->Set();
}
}

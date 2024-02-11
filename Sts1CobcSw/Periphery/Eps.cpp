#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Eps.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

namespace sts1cobcsw::periphery::eps
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


// --- Private globals ---

// MAX11632EEG+T registers

constexpr auto setupRegister = 1_b << 6;

// Pins and SPI

auto cs1GpioPin = hal::GpioPin(hal::epsCs1Pin);
auto cs2GpioPin = hal::GpioPin(hal::epsCs2Pin);
auto cs3GpioPin = hal::GpioPin(hal::epsCs3Pin);
auto epsSpi = RODOS::HAL_SPI(
    hal::framEpsSpiIndex, hal::framEpsSpiSckPin, hal::framEpsSpiMisoPin, hal::framEpsSpiMosiPin);


// --- Private function declarations ---

auto SetSetupRegister() -> void;


// --- Public function definitions ---

auto Initialize() -> void
{
    cs1GpioPin.Direction(hal::PinDirection::out);
    cs1GpioPin.Set();
    cs2GpioPin.Direction(hal::PinDirection::out);
    cs2GpioPin.Set();
    cs3GpioPin.Direction(hal::PinDirection::out);
    cs3GpioPin.Set();

    // TODO: Check if external clock mode is ever necessary
    // Datasheet says 10 MHz max.
    // TODO: FRAM code sets baudrate to 12 MHz
    // SCLK can only run up to 4.8 MHz in clock mode 0b11
    constexpr auto baudrate = 10'000'000;
    hal::Initialize(&epsSpi, baudrate);
}


// --- Private function definitions ---

auto SetSetupRegister() -> void
{
    // Setup register values
    // [7:6]: Register selection bits = 0b01
    // [5:4]: Clock mode and CNVST configuration
    //      Don't use CNVST modes, we use the analog configuration for the CNVST pin
    //      Therefore clock mode = 0b10 << 4
    // [3:2]: Reference mode configuration
    //      Reference off after scan; need wake-up delay: 0b00
    //      Reference always on; no wake-up delay: 0b10
    // [1:0]: Don't care

    // Changing to CNVST mode could be bad (analog input in digital pin)
    constexpr auto clockMode = 0b10_b << 4;
    constexpr auto referenceMode = 0b00_b;
    auto setupData = 0_b | setupRegister | clockMode | referenceMode;
    hal::WriteTo(&epsSpi, Span(setupData));
}
}
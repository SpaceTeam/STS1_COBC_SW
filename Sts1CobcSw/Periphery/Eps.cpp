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

#include <array>
#include <bit>
#include <cstddef>


// The following tables are taken from the wiki page "EPS - Electrical Power Supply" (11.03.2025)
//
// ADC4 (CS1):
//
// | Pin   | Measured value                |
// | ----- | ----------------------------- |
// | AIN0  | Panel Y- solar cell 1 voltage |
// | AIN1  | Panel Y- solar cell 2 voltage |
// | AIN2  | Panel Y+ solar cell 1 voltage |
// | AIN3  | Panel Y+ solar cell 2 voltage |
// | AIN4  | Panel Z- solar cell 1 voltage |
// | AIN5  | Panel Z+ solar cell 1 voltage |
// | AIN6  | Panel X+ solar cell 1 voltage |
// | AIN7  | Panel X+ solar cell 2 voltage |
// | AIN8  | Panel Y- solar cell 1 current |
// | AIN9  | Panel Y- solar cell 2 current |
// | AIN10 | Panel Y+ solar cell 1 current |
// | AIN11 | Panel Y+ solar cell 2 current |
// | AIN12 | Panel Z- solar cell 1 current |
// | AIN13 | Panel Z+ solar cell 1 current |
// | AIN14 | Panel X+ solar cell 1 current |
// | AIN15 | Panel X+ solar cell 2 current |
//
// ADC5 (CS2):
//
// | Pin   | Measured Value              |
// | ----- | --------------------------- |
// | AIN0  | Battery pack voltage        |
// | AIN1  | Battery center tap voltage  |
// | AIN2  | Battery pack current        |
// | AIN3  | Battery temperature         |
// | AIN4  | Panel Y- temperature        |
// | AIN5  | Panel Y+ temperature        |
// | AIN6  | Panel Z- temperature        |
// | AIN7  | Panel Z+ temperature        |
// | AIN8  | Panel X+ temperature        |
// | AIN9  | EPS output current to VBUS  |
// | AIN10 | GND                         |
// | AIN11 | GND                         |
// | AIN12 | GND                         |
// | AIN13 | GND                         |
// | AIN14 | GND                         |
// | AIN15 | GND                         |
//
// ADC6 (CS3):
//
// | Pin   | Measured Value          |
// | ----- | ----------------------- |
// | AIN0  | MPPT bus current        |
// | AIN1  | Panel Z- MPPT 1 current |
// | AIN2  | Panel Y- MPPT 2 current |
// | AIN3  | Panel Y+ MPPT 1 current |
// | AIN4  | Panel Y+ MPPT 2 current |
// | AIN5  | Panel Y- MPPT 1 current |
// | AIN6  | Panel Z+ MPPT 1 current |
// | AIN7  | Panel X+ MPPT 1 current |
// | AIN8  | Panel X+ MPPT 2 current |
// | AIN9  | MPPT bus voltage        |
// | AIN10 | GND                     |
// | AIN11 | GND                     |
// | AIN12 | GND                     |
// | AIN13 | GND                     |
// | AIN14 | GND                     |
// | AIN15 | GND                     |

namespace sts1cobcsw::eps
{
namespace
{
enum class ResetType
{
    registers,
    fifo
};


// --- Private globals ---

inline constexpr auto nChannels = 16U;
using AdcValues = std::array<AdcValue, nChannels>;

constexpr auto spiTimeout = 1 * ms;

auto adc4CsGpioPin = hal::GpioPin(hal::epsAdc4CsPin);
auto adc5CsGpioPin = hal::GpioPin(hal::epsAdc5CsPin);
auto adc6CsGpioPin = hal::GpioPin(hal::epsAdc6CsPin);


// --- Private function declarations ---

auto ConfigureSetupRegister(hal::GpioPin * adcCsPin) -> void;
auto ConfigureAveragingRegister(hal::GpioPin * adcCsPin) -> void;
auto ReadAdc(hal::GpioPin * adcCsPin) -> AdcValues;
auto Reset(hal::GpioPin * adcCsPin, ResetType resetType) -> void;
}


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


auto Read() -> SensorData
{
    auto adc4Values = ReadAdc(&adc4CsGpioPin);
    auto adc5Values = ReadAdc(&adc5CsGpioPin);
    auto adc6Values = ReadAdc(&adc6CsGpioPin);
    // NOLINTBEGIN(*magic-numbers)
    return SensorData{.panelYMinusSolarCell1Voltage = adc4Values[0],
                      .panelYMinusSolarCell2Voltage = adc4Values[1],
                      .panelYPlusSolarCell1Voltage = adc4Values[2],
                      .panelYPlusSolarCell2Voltage = adc4Values[3],
                      .panelZMinusSolarCell1Voltage = adc4Values[4],
                      .panelZPlusSolarCell1Voltage = adc4Values[5],
                      .panelXPlusSolarCell1Voltage = adc4Values[6],
                      .panelXPlusSolarCell2Voltage = adc4Values[7],
                      .panelYMinusSolarCell1Current = adc4Values[8],
                      .panelYMinusSolarCell2Current = adc4Values[9],
                      .panelYPlusSolarCell1Current = adc4Values[10],
                      .panelYPlusSolarCell2Current = adc4Values[11],
                      .panelZMinusSolarCell1Current = adc4Values[12],
                      .panelZPlusSolarCell1Current = adc4Values[13],
                      .panelXPlusSolarCell1Current = adc4Values[14],
                      .panelXPlusSolarCell2Current = adc4Values[15],

                      .batteryPackVoltage = adc5Values[0],
                      .batteryCenterTapVoltage = adc5Values[1],
                      .batteryPackCurrent = adc5Values[2],
                      .batteryTemperature = adc5Values[3],
                      .panelYMinusTemperature = adc5Values[4],
                      .panelYPlusTemperature = adc5Values[5],
                      .panelZMinusTemperature = adc5Values[6],
                      .panelZPlusTemperature = adc5Values[7],
                      .panelXPlusTemperature = adc5Values[8],
                      .epsOutputCurrentToVbus = adc5Values[9],

                      .mpptBusCurrent = adc6Values[0],
                      .panelYMinusMppt1Current = adc6Values[1],
                      .panelYMinusMppt2Current = adc6Values[2],
                      .panelYPlusMppt1Current = adc6Values[3],
                      .panelYPlusMppt2Current = adc6Values[4],
                      .panelZMinusMppt1Current = adc6Values[5],
                      .panelZPlusMppt1Current = adc6Values[6],
                      .panelXPlusMppt1Current = adc6Values[7],
                      .panelXPlusMppt2Current = adc6Values[8],
                      .mpptBusVoltage = adc6Values[9]};
    // NOLINTEND(*magic-numbers)
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

namespace
{
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
}

#include <Sts1CobcSw/Sensors/Eps.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>

#include <algorithm>
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
auto ResetAdc(hal::GpioPin * adcCsPin, ResetType resetType) -> void;
}


// --- Public function definitions ---

auto InitializeAdcs() -> void
{
    if(not persistentVariables.template Load<"epsIsWorking">())
    {
        return;
    }
    adc4CsGpioPin.SetDirection(hal::PinDirection::out);
    adc4CsGpioPin.Set();
    adc5CsGpioPin.SetDirection(hal::PinDirection::out);
    adc5CsGpioPin.Set();
    adc6CsGpioPin.SetDirection(hal::PinDirection::out);
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


auto ReadAdcs() -> AdcData
{
    if(not persistentVariables.template Load<"epsIsWorking">())
    {
        return AdcData{};
    }
    auto adcData = AdcData{};
    adcData.adc4 = ReadAdc(&adc4CsGpioPin);
    auto adc5Data = ReadAdc(&adc5CsGpioPin);
    static_assert(adc5Data.size() >= adcData.adc5.size());
    std::copy_n(adc5Data.begin(), adcData.adc5.size(), adcData.adc5.begin());
    auto adc6Data = ReadAdc(&adc6CsGpioPin);
    static_assert(adc6Data.size() >= adcData.adc6.size());
    std::copy_n(adc6Data.begin(), adcData.adc6.size(), adcData.adc6.begin());
    return adcData;
}


auto ResetAdcRegisters() -> void
{
    if(not persistentVariables.template Load<"epsIsWorking">())
    {
        return;
    }
    ResetAdc(&adc4CsGpioPin, ResetType::registers);
    ResetAdc(&adc5CsGpioPin, ResetType::registers);
    ResetAdc(&adc6CsGpioPin, ResetType::registers);
}


auto ClearAdcFifos() -> void
{
    if(not persistentVariables.template Load<"epsIsWorking">())
    {
        return;
    }
    ResetAdc(&adc4CsGpioPin, ResetType::fifo);
    ResetAdc(&adc5CsGpioPin, ResetType::fifo);
    ResetAdc(&adc6CsGpioPin, ResetType::fifo);
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
auto ResetAdc(hal::GpioPin * adcCsPin, ResetType resetType) -> void
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

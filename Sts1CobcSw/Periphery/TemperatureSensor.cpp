//! @file
//! @brief "Driver" for the temperature sensor TMP36xS


#include <Sts1CobcSw/Periphery/TemperatureSensor.hpp>

#include <rodos_no_using_namespace.h>

/*
3.30 Temperature sensor
The temperature sensor is internally
connected to the ADC_IN18 input channel which is used to convert the sensor output
voltage into a digital value. Refer to the reference manual for additional information.

no ADC_IN18 only ADC_CH_018

*/

namespace sts1cobcsw::rftemperaturesensor
{



// RF_TMP is read on pin PC0 on internal ADC1
auto adc = RODOS::HAL_ADC(RODOS::ADC_IDX2);
constexpr auto channel = RODOS::ADC_CH_010;


auto Initialize() -> void
{
    adc.init(channel);
    const int32_t bitResolution = 12;  // 3.3 V / 2^12 bits / (10 mV/°C) = 0.0806 °C/bit
    adc.config(RODOS::ADC_PARAMETER_RESOLUTION, bitResolution);
}


auto Read() -> std::uint16_t
{
    auto rfTemperature = adc.read(channel);
    if(rfTemperature == static_cast<std::uint16_t>(RODOS::ADC_ERR_CONV_FAIL))
    {
        adc.reset();
        Initialize();
        rfTemperature = adc.read(channel);
    }
    return rfTemperature;
}



// copy code from RF_TMP Sensor as quick and dirty test
auto adcStm32Temperature = RODOS::HAL_ADC(RODOS::ADC_IDX1);
constexpr auto channelStm32Temperature = RODOS::ADC_CH_018;

auto InitializeStm32Temperature() -> void
{
    adcStm32Temperature.init(channelStm32Temperature);
    const int32_t bitResolution = 12;  // 3.3 V / 2^12 bits / (10 mV/°C) = 0.0806 °C/bit
    adcStm32Temperature.config(RODOS::ADC_PARAMETER_RESOLUTION, bitResolution);
}

/*
TODO: get correct temperature value, from raw value:
manual page 119:
TS_CAL1 TS ADC raw data acquired at temperature of 30 °C, VDDA= 3.3 V 0x1FFF 7A2C - 0x1FFF 7A2D
TS_CAL2 TS ADC raw data acquired at temperature of 110 °C, VDDA= 3.3 V 0x1FFF 7A2E - 0x1FFF 7A2F
*/
auto ReadStm32Temperature() -> std::uint16_t
{
    auto stm32Temperature = adcStm32Temperature.read(channelStm32Temperature);
    // if(rfTemperature == static_cast<std::uint16_t>(RODOS::ADC_ERR_CONV_FAIL))
    // {
    //     adc.reset();
    //     Initialize();
    //     rfTemperature = adc.read(channel);
    // }
    return stm32Temperature;
}



}

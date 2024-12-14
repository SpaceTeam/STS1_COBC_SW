//! @file
//! @brief "Driver" for the temperature sensor TMP36xS


#include <Sts1CobcSw/Periphery/TemperatureSensor.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::rftemperaturesensor
{
// RF_TMP is read on pin PC0 on internal ADC1
auto adc = RODOS::HAL_ADC(RODOS::ADC_IDX1);
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
}

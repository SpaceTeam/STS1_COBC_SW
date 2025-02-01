//! @file
//! @brief "Driver" for ADC sensors


#include <Sts1CobcSw/Periphery/AdcSensors.hpp>

#include <rodos/src/bare-metal/stm32f4/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_adc.h>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::adc
{
constexpr std::int32_t bitResolution = 12;  // 3.3 V / 2^12 bits / (10 mV/°C) = 0.0806 °C/bit
// RF_TMP is read on pin PC0 == ADC_CH_010 on internal ADC1
constexpr auto rfTemperatureChannel = RODOS::ADC_CH_010;
// MCU_TMP is read on ADC_CH_016 on internal ADC1 (and not on 18!)
constexpr auto mcuTemperatureChannel = RODOS::ADC_CH_016;


namespace
{
auto adc = RODOS::HAL_ADC(RODOS::ADC_IDX1);


auto Read(RODOS::ADC_CHANNEL channel) -> std::uint16_t;
}


auto Initialize() -> void
{
    ADC_TempSensorVrefintCmd(FunctionalState::ENABLE);
    adc.init(rfTemperatureChannel);
    adc.init(mcuTemperatureChannel);
    adc.config(RODOS::ADC_PARAMETER_RESOLUTION, bitResolution);
}


auto ReadRfTemperature() -> std::uint16_t
{
    return Read(rfTemperatureChannel);
}


auto ReadMcuTemperature() -> std::uint16_t
{
    return Read(mcuTemperatureChannel);
}


namespace
{
auto Read(RODOS::ADC_CHANNEL channel) -> std::uint16_t
{
    auto value = adc.read(channel);
    if(value == static_cast<std::uint16_t>(RODOS::ADC_ERR_CONV_FAIL))
    {
        adc.reset();
        Initialize();
        value = adc.read(channel);
    }
    return value;
}
}
}

//! @file
//! @brief "Driver" for ADC sensors


#include <Sts1CobcSw/Periphery/TemperatureSensors.hpp>

#include <rodos/src/bare-metal/stm32f4/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_adc.h>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::temperaturesensors
{
constexpr std::int32_t adcBitResolution = 12;  // 3.3 V / 2^12 bits / (10 mV/°C) = 0.0806 °C/bit
// RF_TMP is read on pin PC0 == ADC_CH_010 on internal ADC1
constexpr auto rfChannel = RODOS::ADC_CH_010;
// MCU_TMP is read on ADC_CH_016 on internal ADC1 (and not on 18!)
constexpr auto mcuChannel = RODOS::ADC_CH_016;


namespace
{
auto adc = RODOS::HAL_ADC(RODOS::ADC_IDX1);


auto Read(RODOS::ADC_CHANNEL channel) -> std::uint16_t;
}


auto Initialize() -> void
{
    ADC_TempSensorVrefintCmd(FunctionalState::ENABLE);
    adc.init(mcuChannel);
    adc.init(rfChannel);
    adc.config(RODOS::ADC_PARAMETER_RESOLUTION, adcBitResolution);
}


auto ReadRfTemperature() -> std::uint16_t
{
    return Read(rfChannel);
}


auto ReadMcuTemperature() -> std::uint16_t
{
    return Read(mcuChannel);
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

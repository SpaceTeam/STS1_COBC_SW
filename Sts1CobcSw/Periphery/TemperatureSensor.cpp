//! @file
//! @brief  temperature sensors readout
//!


#include <rodos_no_using_namespace.h>

#include <Sts1CobcSw/Hal/GpioPin.hpp>       // Todo: MR:07.12.2024: I think this is not needed, check if file can be reset to original state
#include <Sts1CobcSw/Hal/IoNames.hpp>       // Todo: MR:07.12.2024: I think this is not needed,
#include <cstdint>

// from ionames inline constexpr auto rfTmpPin = pc0;    //probably not used

// temperature Sensor: TMP36xS

// https://github.com/SpaceTeam/rodos/blob/st_develop/api/hal/hal_adc.h
// https://github.com/SpaceTeam/rodos/blob/st_develop/src/bare-metal/stm32f4/hal/hal_adc.cpp

// STM32F411
// RF_TMP is read on pin PC0 on internal ADC



RODOS::HAL_ADC temperatureAdc(RODOS::ADC_IDX1);             //this blocks adc index 1 !!!
constexpr RODOS::ADC_CHANNEL adcChannel = RODOS::ADC_CH_010;    // ADC_CH_010,     // PC0/PC0/PC0


auto InitRfTemperature() -> void
{
    temperatureAdc.init(adcChannel);

    const int32_t bitResolution = 12;                       // 12-bit: 0.0403°C // 10-bit: 0.161°C // 8-bit: 0.644°C // 6-bit: 2.578°C
    temperatureAdc.config(RODOS::ADC_PARAMETER_RESOLUTION, bitResolution);
}


auto ReadRfTemperature() -> std::uint16_t
{
    std::uint16_t rfTemp = 0;
    const std::uint16_t adcConvFail = 65533;                // ADC failed conversion
    rfTemp = temperatureAdc.read(adcChannel);               //errors are returned as negative values. type is uint16_t errors are ~65535
    if (rfTemp == adcConvFail)
    {
        temperatureAdc.reset();     // Todo: MR:07.12.2024: check what function actually does. It might reset too much.
        InitRfTemperature();
        rfTemp = temperatureAdc.read(adcChannel);
    }

    return rfTemp;
}

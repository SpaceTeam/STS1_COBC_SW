//! @file
//! @brief  temperature sensors
//!


#include <rodos_no_using_namespace.h>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <cstdint>

// from ionames inline constexpr auto rfTmpPin = pc0;    //probably not used

// temperature Sensor: TMP36xS

// https://github.com/SpaceTeam/rodos/blob/st_develop/api/hal/hal_adc.h
// https://github.com/SpaceTeam/rodos/blob/st_develop/src/bare-metal/stm32f4/hal/hal_adc.cpp

// STM32F411
// RF_TMP is read on pin PC0 on internal ADC



auto ReadRfTemperature() -> std::uint16_t
{

    //RODOS::ADC_ERROR errorValue;                                    //there seems to be not an option with 0 = no error witch is dumb.



    const RODOS::ADC_IDX adcChannelIndex = RODOS::ADC_IDX1;               // index 1, 2, 3are ok
    RODOS::HAL_ADC temperatureAdcObject(adcChannelIndex);

    // set channel:
    int32_t error = 0;
    const RODOS::ADC_CHANNEL adcChannelForRfTemp = RODOS::ADC_CH_010;     // ADC_CH_010,     // PC0/PC0/PC0
    error  = temperatureAdcObject.init(adcChannelForRfTemp);
    if ( error != 0 )
    {
        //do something with error
    }

    // set resolution:

    // 6-bit    2.578°C
    // 8-bit    0.644°C
    // 10-bit   0.161°C
    // 12-bit   0.0403°C
    const int32_t bitResolution = 10;
    const RODOS::ADC_PARAMETER_TYPE configParameter = RODOS::ADC_PARAMETER_RESOLUTION;
    error  = temperatureAdcObject.config(configParameter, bitResolution);
    if ( error != 0 )
    {
        //do something with error
    }

    std::uint16_t rfTemp = 0;
    rfTemp = temperatureAdcObject.read(adcChannelForRfTemp);    //errors are returned as negative values. type is uint16_t errors are ~65535

    temperatureAdcObject.reset();   // unsure if this is needed.

    return rfTemp;
}

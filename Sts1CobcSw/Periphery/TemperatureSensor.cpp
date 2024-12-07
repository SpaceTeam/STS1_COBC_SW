//! @file
//! @brief  temperature sensors
//!


#include <rodos_no_using_namespace.h>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>

//from ionames inline constexpr auto rfTmpPin = pc0;

// https://github.com/SpaceTeam/rodos/blob/st_develop/api/hal/hal_adc.h
// https://github.com/SpaceTeam/rodos/blob/st_develop/src/bare-metal/stm32f4/hal/hal_adc.cpp

// STM32F411
// RF_TMP is read on pin PC0 on internal ADC



auto ReadRfTemperature() -> std::uint16_t
{
    std::uint16_t rfTemp = 0;

        //code

    return rfTemp;
}

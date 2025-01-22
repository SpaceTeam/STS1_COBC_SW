//! @file
//! @brief "Driver" for the temperature sensor TMP36xS


#include <Sts1CobcSw/Periphery/TemperatureSensor.hpp>

#include <rodos_no_using_namespace.h>

// Includes     ADC_TempSensorVrefintCmd(FunctionalState::ENABLE); function
//C:\Coding_Stuff\Space_Team\rodos\src\bare-metal\stm32f4\STM32F4xx_StdPeriph_Driver\src\stm32f4xx_adc.c
#include <rodos/src/bare-metal/stm32f4/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_adc.h>

/*

Temperature sensor is connected to ADC_CH_016 NOT 18!!!!!!

for our STM32 there is only one ADC!!!: ADC_IDX1

to read th rf temp and the MCU temp we need to use the same ADC and switch the channels from 10 to 16!

hardware testfile compile folder:
C:\Coding_Stuff\Space_Team\STS1_COBC_SW\build\cobc\Tests\HardwareTests

*/

namespace sts1cobcsw::rftemperaturesensor
{
// RF_TMP is read on pin PC0 on internal ADC1
auto adc = RODOS::HAL_ADC(RODOS::ADC_IDX1);
constexpr auto channel = RODOS::ADC_CH_016;





auto Initialize() -> void
{
    ADC_TempSensorVrefintCmd(FunctionalState::ENABLE);


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

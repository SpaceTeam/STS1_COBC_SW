//! @file
//! @brief "Driver" for the temperature sensor TMP36xS


#include <Sts1CobcSw/Periphery/TemperatureSensors.hpp>

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

namespace sts1cobcsw::temperaturesensors
{

// RF_TMP is read on pin PC0 == ADC_CH_010 on internal ADC1
auto rfAdc = RODOS::HAL_ADC(RODOS::ADC_IDX1);
constexpr auto rfChannel = RODOS::ADC_CH_010;

// MCU_TMP is read on ADC_CH_016 on internal ADC1 (and not on 18!)
auto mcuAdc = RODOS::HAL_ADC(RODOS::ADC_IDX1);
constexpr auto mcuChannel = RODOS::ADC_CH_016;

//constexpr int32_t adcBitResolution = 12;  // 3.3 V / 2^12 bits / (10 mV/°C) = 0.0806 °C/bit
constexpr int32_t adcBitResolution = 8;  // 3.3 V / 2^12 bits / (10 mV/°C) = 0.0806 °C/bit


auto InitializeRf() -> void
{
    ADC_TempSensorVrefintCmd(FunctionalState::ENABLE);
    rfAdc.init(rfChannel);
    rfAdc.config(RODOS::ADC_PARAMETER_RESOLUTION, adcBitResolution);
}

auto InitializeMcu() -> void
{
    ADC_TempSensorVrefintCmd(FunctionalState::ENABLE);
    mcuAdc.init(mcuChannel);
    mcuAdc.config(RODOS::ADC_PARAMETER_RESOLUTION, adcBitResolution);
}

auto ReadRf() -> std::uint16_t
{
    auto rfTemperature = rfAdc.read(rfChannel);
    if(rfTemperature == static_cast<std::uint16_t>(RODOS::ADC_ERR_CONV_FAIL))
    {
        rfAdc.reset();
        InitializeRf();
        rfTemperature = rfAdc.read(rfChannel);
    }
    return rfTemperature;
}

auto ReadMcu() -> std::uint16_t
{
    auto rfTemperature = mcuAdc.read(mcuChannel);
    if(rfTemperature == static_cast<std::uint16_t>(RODOS::ADC_ERR_CONV_FAIL))
    {
        mcuAdc.reset();
        InitializeMcu();
        rfTemperature = mcuAdc.read(mcuChannel);
    }
    return rfTemperature;
}




}

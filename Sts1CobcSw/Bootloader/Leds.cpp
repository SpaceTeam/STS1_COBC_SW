#include <Sts1CobcSw/Bootloader/Leds.hpp>

#include <Sts1CobcSw/Bootloader/stm32f411xe.h>


namespace sts1cobcsw::leds
{
// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

auto Initialize() -> void
{
    // LEDs are connected to PB12 and PB15
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;                                // Enable GPIOB clock
    GPIOB->MODER |= (GPIO_MODER_MODER12_0 | GPIO_MODER_MODER15_0);      // Output mode
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED12 | GPIO_OSPEEDR_OSPEED15);  // High speed
}


auto Reset() -> void
{
    // Reset PB12 and PB15 to default state
    GPIOB->MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER15);          // Input mode
    GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED12 | GPIO_OSPEEDR_OSPEED15);  // Low speed
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;                                // Disable GPIOB clock
}


auto TurnOn() -> void
{
    GPIOB->ODR |= (GPIO_ODR_OD12 | GPIO_ODR_OD15);  // Set PB12 and PB15 high
}


auto TurnOff() -> void
{
    GPIOB->ODR &= ~(GPIO_ODR_OD12 | GPIO_ODR_OD15);  // Set PB12 and PB15 low
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}

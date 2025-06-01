#include <Sts1CobcSw/Bootloader/UciUart.hpp>

#include <Sts1CobcSw/Bootloader/stm32f411xe.h>


namespace sts1cobcsw::uciuart
{
// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

auto Initialize() -> void
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // Enable GPIOA clock
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;  // Enable USART2 clock

    // Configure PA2 and PA3 for USART2 (TX and RX)
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1);  // Alternate function mode
    static constexpr auto af7 = 7U;                               // AF7 for USART2
    GPIOA->AFR[0] |= (af7 << GPIO_AFRL_AFSEL2_Pos) | (af7 << GPIO_AFRL_AFSEL3_Pos);
    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);  // High speed

    // Configure USART2 (default settings are 8N1)
    static constexpr auto usartdiv = 0x008C;     // = 8.6875 → ~115200 bd @ 16 MHz & OVER8 = 0 acc.
    USART2->BRR = usartdiv;                      // Table 75 in the reference manual RM0383
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;  // Enable transmitter and receiver
    USART2->CR1 |= USART_CR1_UE;                 // Enable USART2
}


auto Reset() -> void
{
    // Reset USART2 to default state
    USART2->CR1 &= ~USART_CR1_UE;                   // Disable USART2
    USART2->CR1 &= ~(USART_CR1_TE | USART_CR1_RE);  // Disable transmitter and receiver
    USART2->BRR = 0x0000;                           // Reset baud rate register

    // Reset PA2 and PA3 to default state
    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);

    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;   // Disable GPIOA clock
    RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;  // Disable USART2 clock
}


auto Write(char character) -> void
{
    while((USART2->SR & USART_SR_TXE) == 0) {}           // Wait until TX data register is empty
    USART2->DR = static_cast<unsigned char>(character);  // Send character
}


auto Write(char const * string) -> void
{
    for(; *string != '\0'; ++string)  // NOLINT(*pointer-arithmetic)
    {
        // Convert \n to \r\n because RODOS::PRINTF() also does this
        if(*string == '\n')
        {
            Write('\r');
        }
        Write(*string);
    }
    while((USART2->SR & USART_SR_TC) == 0) {}  // Wait until transmission is complete
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}

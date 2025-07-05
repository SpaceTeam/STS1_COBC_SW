#include <Sts1CobcSw/Bootloader/Spi.hpp>

#include <Sts1CobcSw/Bootloader/stm32f411xe.h>

namespace sts1cobcsw::spi
{
// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

constexpr unsigned char spiDummyByte = 0xFF;

auto Initialize() -> void
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;  // Enable GPIOB clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;  // Enable GPIOC clock
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;   // Enable SPI3 clock

    // Configure GPIO for SPI3 - Sck, MISO, MOSI, CsPin
    GPIOC->MODER |= (GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1
                     | GPIO_MODER_MODER12_1);  // PC10, PC11, PC12 Alternate
    static constexpr auto af6 = 6U;            // AF6 for SPI3
    GPIOC->AFR[1] |= (af6 << GPIO_AFRH_AFSEL10_Pos) | (af6 << GPIO_AFRH_AFSEL11_Pos)
                   | (af6 << GPIO_AFRH_AFSEL12_Pos);
    GPIOC->OSPEEDR |= (GPIO_OSPEEDR_OSPEED10 | GPIO_OSPEEDR_OSPEED11
                       | GPIO_OSPEEDR_OSPEED12);  // High speed for SCK, MISO, MOSI

    GPIOB->MODER |= (GPIO_MODER_MODER13_0);     // Output mode functoin mode
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED13);  // High speed

    // Configure SPI3
    SPI3->CR1 &= ~SPI_CR1_SPE;                    // Disable SPI before configuration
    SPI3->CR1 |= SPI_CR1_MSTR;                    // Set master mode
    SPI3->CR1 &= ~SPI_CR1_BR;                     // Define the serial clock baud rate
    SPI3->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);  // Clock polarity and phase
    SPI3->CR1 &= ~SPI_CR1_DFF;                    // 8 bit data frame format
    SPI3->CR1 &= ~SPI_CR1_LSBFIRST;               // Most significant bit first
    SPI3->CR1 &= ~SPI_CR1_RXONLY;                 // Full-duplex
    SPI3->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;       // NSS software mode
    SPI3->CR1 |= SPI_CR1_SPE;                     // Enable SPI3
}


auto Reset() -> void
{
    // Reset SPI3 to default state
    SPI3->CR1 &= ~SPI_CR1_SPE;                  // Disable SPI3
    SPI3->CR1 &= ~SPI_CR1_MSTR;                 // Reset master mode
    SPI3->CR1 &= ~(SPI_CR1_SSM | SPI_CR1_SSI);  // Disable NSS software mode

    // Reset GPIO to default state
    GPIOC->MODER &= ~(GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1 | GPIO_MODER_MODER12_1);
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED10 | GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12);

    GPIOB->MODER &= ~(GPIO_MODER_MODER13_0);
    GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED13);

    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;  // Disable GPIOB clock
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN;  // Disable GPIOB clock
    RCC->APB1ENR &= ~RCC_APB1ENR_SPI3EN;   // Disable SPI3 clock
}


auto Write(unsigned char character) -> void
{
    while((SPI3->SR & SPI_SR_TXE) == 0) {}  // Wait until TX data register is empty
    SPI3->DR = character;                   // Send character
    while((SPI3->SR & SPI_SR_RXNE) == 0) {}   
    (void)static_cast<char>(SPI3->DR);
}


auto Read(char * character) -> void
{
    while((SPI3->SR & SPI_SR_TXE) == 0) {}  // Wait until TX data register is empty
    SPI3->DR = spiDummyByte;                // Send dummy byte
    while((SPI3->SR & SPI_SR_RXNE) == 0) {}    // Wait until RX data register is not empty
    *character = static_cast<char>(SPI3->DR);  // Read received data
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}

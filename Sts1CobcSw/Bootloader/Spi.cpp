#include <Sts1CobcSw/Bootloader/Spi.hpp>

#include <Sts1CobcSw/CmsisDevice/stm32f411xe.h>


namespace sts1cobcsw::spi
{
using sts1cobcsw::operator""_b;


// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

auto Initialize() -> void
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;  // Enable GPIOB clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;  // Enable GPIOC clock
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;   // Enable SPI3 clock

    // Configure PC10, PC11, PC12, and PB13 for SPI3 (SCK, MISO, MOSI, CS/NSS)
    GPIOC->MODER |= (GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1  // Alternate function mode
                     | GPIO_MODER_MODER12_1);
    static constexpr auto af6 = 6U;  // AF6 for SPI3
    GPIOC->AFR[1] |= (af6 << GPIO_AFRH_AFSEL10_Pos) | (af6 << GPIO_AFRH_AFSEL11_Pos)
                   | (af6 << GPIO_AFRH_AFSEL12_Pos);
    GPIOC->OSPEEDR |= (GPIO_OSPEEDR_OSPEED10 | GPIO_OSPEEDR_OSPEED11  // High speed
                       | GPIO_OSPEEDR_OSPEED12);

    // Configure SPI3 (default settings are full duplex, no CRC, 8-bit data frame, MSB first, baud
    // rate = fPCLK / 2 = 16 MHz / 2 = 8 MHz, CPOL = 0, CPHA = 0)
    SPI3->CR1 |= SPI_CR1_MSTR;               // Master mode
    SPI3->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;  // Enable NSS software mode
    SPI3->CR1 |= SPI_CR1_SPE;                // Enable SPI3
}


auto Reset() -> void
{
    // Reset SPI3 to default state
    SPI3->CR1 &= ~SPI_CR1_SPE;                  // Disable SPI3
    SPI3->CR1 &= ~SPI_CR1_MSTR;                 // Disable master mode
    SPI3->CR1 &= ~(SPI_CR1_SSM | SPI_CR1_SSI);  // Disable NSS software mode

    // Reset PC10, PC11, PC12, and PB13 to default state
    GPIOC->MODER &= ~(GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1 | GPIO_MODER_MODER12_1);
    GPIOC->AFR[1] &= ~(GPIO_AFRH_AFSEL10 | GPIO_AFRH_AFSEL11 | GPIO_AFRH_AFSEL12);
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED10 | GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12);

    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;  // Disable GPIOB clock
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN;  // Disable GPIOC clock
    RCC->APB1ENR &= ~RCC_APB1ENR_SPI3EN;   // Disable SPI3 clock
}


auto Write(Byte byte) -> void
{
    while((SPI3->SR & SPI_SR_TXE) == 0) {}       // Wait until TX buffer is empty
    SPI3->DR = static_cast<unsigned int>(byte);  // Write byte to TX buffer
}


auto Write(std::span<Byte const> data) -> void
{
    for(auto && byte : data)
    {
        Write(byte);
    }
}


auto WaitUntilTxComplete() -> void
{
    while((SPI3->SR & SPI_SR_TXE) == 0) {}  // Wait until TX buffer is empty
    while((SPI3->SR & SPI_SR_BSY) != 0) {}  // Wait until SPI is not busy
    (void)(SPI3->DR);                       // Read received data to clear RXNE flag
}


auto Read(std::span<Byte> data) -> void
{
    static constexpr auto dummyByte = 0xFF_b;
    for(auto && byte : data)
    {
        Write(dummyByte);
        while((SPI3->SR & SPI_SR_RXNE) == 0) {}  // Wait until RX buffer is not empty
        byte = static_cast<Byte>(SPI3->DR);      // Read received data
    }
}


#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}

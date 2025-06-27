#include <Sts1CobcSw/Bootloader/UciSpi.hpp>
#include <Sts1CobcSw/Bootloader/BusyWait.hpp>

#include <Sts1CobcSw/Bootloader/stm32f411xe.h>


namespace sts1cobcsw::ucispi
{
// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace
{
namespace opcode
{
constexpr auto writeData = 0x02U;
constexpr auto readData = 0x03U;
constexpr auto setWriteEnableLatch = 0x06U;
}
}

auto Initialize() -> void
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable GPIOB clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // Enable GPIOB clock
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;  // Enable SPI3 clock

    // Configure GPIO for SPI3 - Sck, MISO, MOSI, CsPin
    GPIOC->MODER |= (GPIO_MODER_MODER10_0 | GPIO_MODER_MODER12_0); // Output functoin mode
    GPIOC->OSPEEDR |= (GPIO_OSPEEDR_OSPEED10 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER12_0);
    
    GPIOB->MODER |= (GPIO_MODER_MODER13_0);    // Output functoin mode
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED13); // High speed
    
    // Configure SPI3 
    SPI3->CR1 |= SPI_CR1_BR;                  // Define the serial clock baud rate
    //SPI3->CR1 |= SPI_CR1_DFF;               // 16 bit data frame format
    SPI3->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;   //NSS software mode
    //SPI3->I2SPR |= SPI_I2SPR_ODD;           // Prescaler
    SPI3->CR1 |= SPI_CR1_MSTR;                // Set master mode
    SPI3->CR1 |= SPI_CR1_SPE;                 // Enable SPI3
}


auto Reset() -> void
{
    // Reset SPI3 to default state
    SPI3->CR1 &= ~SPI_CR1_SPE;                 // Disable SPI3
    SPI3->CR1 &= ~SPI_CR1_MSTR;                // Reset master mode
    SPI3->CR1 &= ~(SPI_CR1_SSM | SPI_CR1_SSI); // Disable NSS software mode
    SPI3->CR1 &= ~SPI_CR1_BR;                  // Reset baud rate register
    
    // Reset GPIO to default state
    GPIOC->MODER &= ~(GPIO_MODER_MODER10_0 | GPIO_MODER_MODER12_0);
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED10 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER12_0);
    
    GPIOB->MODER &= ~(GPIO_MODER_MODER13_0);
    GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED13);

    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN; // Disable GPIOB clock
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN; // Disable GPIOB clock
    RCC->APB1ENR &= ~RCC_APB1ENR_SPI3EN;  // Disable SPI3 clock
}


auto Write(unsigned char character) -> void
{
    while((SPI3->SR & SPI_SR_TXE) == 0) {}           // Wait until TX data register is empty
    SPI3->DR = character;  // Send character
}

auto Read(char *character) -> void
{
    while(SPI_SR_RXNE == 0) {}  // Wait until RX data register is not empty
    *character = static_cast<char>(SPI3->DR); // Read received data
}

auto SetCsPin() -> void
{
    static constexpr auto chipSelectDelay = 1;
    GPIOB->ODR |= (GPIO_ODR_OD13); //Set Cs Pin
    BusyWaitUs(chipSelectDelay);
}

auto ResetCsPin() -> void
{
    static constexpr auto chipSelectDelay = 1;
    GPIOB->ODR &= ~(GPIO_ODR_OD13); //Reset Cs Pin
    BusyWaitUs(chipSelectDelay);
}

auto FramWrite(char32_t address, char const * string, int size) -> void
{  
    ResetCsPin();
    Write(opcode::setWriteEnableLatch);    
    SetCsPin();
    
    ResetCsPin();
    Write(opcode::writeData);
    Write((address & 0x00FF0000)>>16);
    Write((address & 0x0000FF00)>>8);
    Write(address & 0x000000FF);
    for(int i=0;i<size;i++)  // NOLINT(*pointer-arithmetic)
    {
        Write(string[i]);
    }
    while((SPI3->SR & SPI_SR_TXE) == 0) {}   // Wait until transmission is complete
    
    SetCsPin();
}

auto FramRead(char32_t address, char *string, int size) -> void
{
    ResetCsPin();
    Write(opcode::readData);    
    Write((address & 0x00FF0000)>>16);
    Write((address & 0x0000FF00)>>8);
    Write(address & 0x000000FF);
    while((SPI3->SR & SPI_SR_TXE) == 0) {}
    for(int i=0;i<size;i++)
    {
       Read(&string[i]); 
    }
    while(SPI_SR_RXNE == 0) {}
    SetCsPin();
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}

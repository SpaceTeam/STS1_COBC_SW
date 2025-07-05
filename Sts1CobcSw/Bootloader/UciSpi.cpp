#include <Sts1CobcSw/Bootloader/UciSpi.hpp>
#include <Sts1CobcSw/Bootloader/BusyWait.hpp>

#include <Sts1CobcSw/Bootloader/stm32f411xe.h>
#include <Sts1CobcSw/Bootloader/UciUart.hpp> //test only

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
constexpr auto readDeviceId = 0x9FU;
constexpr auto setWriteEnableLatch = 0x06U;
}
}

auto Initialize() -> void
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable GPIOB clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // Enable GPIOC clock
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;  // Enable SPI3 clock

    // Configure GPIO for SPI3 - Sck, MISO, MOSI, CsPin
    // Configure PC10 (SCK) and PC12 (MOSI) as output, PC11 (MISO) as input
    GPIOC->MODER |= (GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1 | GPIO_MODER_MODER12_1); // PC10, PC11, PC12 Alternate
    static constexpr auto af6 = 6U;                               // AF6 for SPI3
    GPIOC->AFR[1] |= (af6 << GPIO_AFRH_AFSEL10_Pos) | (af6 << GPIO_AFRH_AFSEL11_Pos) | (af6 << GPIO_AFRH_AFSEL12_Pos);
    //GPIOC->MODER &= ~GPIO_MODER_MODER11_1; // PC11 input (reset both bits to 00)
    GPIOC->OSPEEDR |= (GPIO_OSPEEDR_OSPEED10 | GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12); // High speed for SCK, MISO, MOSI
    
    GPIOB->MODER |= (GPIO_MODER_MODER13_0);    // Output mode functoin mode
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED13); // High speed
    
    // Configure SPI3 
    SPI3->CR1 &= ~SPI_CR1_SPE;                      // Disable SPI before configuration
    SPI3->CR1 |= SPI_CR1_MSTR;                      // Set master mode
    SPI3->CR1 &= ~SPI_CR1_BR;                       // Define the serial clock baud rate
    SPI3->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);    // Clock polarity and phase
    SPI3->CR1 &= ~SPI_CR1_DFF;                      // 8 bit data frame format
    SPI3->CR1 &= ~SPI_CR1_LSBFIRST;                 //Most significant bit first
    SPI3->CR1 &= ~SPI_CR1_RXONLY;                   // Full-duplex
    SPI3->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;         //NSS software mode
    SPI3->CR1 |= SPI_CR1_SPE;                       // Enable SPI3
}


auto Reset() -> void
{
    // Reset SPI3 to default state
    SPI3->CR1 &= ~SPI_CR1_SPE;                 // Disable SPI3
    SPI3->CR1 &= ~SPI_CR1_MSTR;                // Reset master mode
    SPI3->CR1 &= ~(SPI_CR1_SSM | SPI_CR1_SSI); // Disable NSS software mode
    SPI3->CR1 &= ~SPI_CR1_BR;                  // Reset baud rate register
    
    // Reset GPIO to default state
    GPIOC->MODER &= ~(GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1 | GPIO_MODER_MODER12_1);
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED10 | GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12);
    
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
    Write(0xFF);
    while((SPI3->SR &SPI_SR_RXNE) == 0) {}  // Wait until RX data register is not empty
    *character = static_cast<char>(SPI3->DR); // Read received data
}

auto SetCsPin() -> void
{
    static constexpr auto chipSelectDelay = 50;
    GPIOB->ODR |= (GPIO_ODR_OD13); //Set Cs Pin
    BusyWaitUs(chipSelectDelay);
}

auto ResetCsPin() -> void
{
    static constexpr auto chipSelectDelay = 50;
    GPIOB->ODR &= ~(GPIO_ODR_OD13); //Reset Cs Pin
    BusyWaitUs(chipSelectDelay);
}

auto FramInitialize()->void
{
    GPIOB->ODR |= (GPIO_ODR_OD13);    
}

auto FramReset()->void
{
    GPIOB->ODR &= ~(GPIO_ODR_OD13);    
}

auto FramReadId() -> void
{
    char rx[9] = {0};

    sts1cobcsw::uciuart::Write("Fram ID: ");

    ResetCsPin();
    Write(opcode::readDeviceId);
    Read(&rx[0]);
        
    for (char & i : rx) 
    {
        Read(&i);
        //sts1cobcsw::uciuart::Write(rx[i]);
    }
    
    SetCsPin();
    
    // Convert rx to a printable hex string
    char idString[26];
    int len = 0;
    for(char i : rx) // skip the first byte (command echo)
    {
        auto byte = static_cast<unsigned char>(i);
        // High nibble
        idString[len++] = "0123456789ABCDEF"[byte >> 4];
        // Low nibble
        idString[len++] = "0123456789ABCDEF"[byte & 0x0F];
        if(len < 26)
        {
            idString[len++] = ' ';
        }
    }
    
    sts1cobcsw::uciuart::Write(static_cast<const char *>(&idString[0]));
}

auto FramWrite(unsigned long address, char const * string, int size) -> void
{  
    ResetCsPin();
    Write(opcode::setWriteEnableLatch);    
    SetCsPin();
    
    ResetCsPin();
    Write(opcode::writeData);
    Write((address & 0x00FF0000)>>16);
    Write((address & 0x0000FF00)>>8);
    Write(address & 0x000000FF);
    for(int i=0;i<size;i++)
    {
        Write(string[i]);
    }
    while((SPI3->SR & SPI_SR_TXE) == 0) {}   // Wait until transmission is complete
    SetCsPin();
}

auto FramRead(unsigned long address, char *string, int size) -> void
{
   unsigned int adressByte0 = (address & 0x00FF0000)>>16;
   unsigned int adressByte1 = (address & 0x0000FF00)>>8;
   unsigned int adressByte2 = address & 0x000000FF;
    
    ResetCsPin();
    Write(opcode::readData);    
    Write(static_cast<char>(adressByte0));
    Write(static_cast<char>(adressByte1));
    Write(static_cast<char>(adressByte2));
    while((SPI3->SR & SPI_SR_TXE) == 0) {}
    for(int i=0;i<size;i++)
    {
       Read(&string[i]);
       sts1cobcsw::uciuart::Write(string[i]);
       sts1cobcsw::uciuart::Write("\n");
    }
    while(SPI_SR_RXNE == 0) {}
    SetCsPin();
}

auto PersistentWariableRead(char32_t address, unsigned int blockSize) -> unsigned int
{
    auto value0Char = static_cast<char>(0);
    auto value1Char = static_cast<char>(0);
    auto value2Char = static_cast<char>(0); 
    FramRead(address, &value0Char, 1);
    FramRead(address + blockSize, &value1Char, 1);
    FramRead(address + 2*blockSize, &value2Char, 1);    
    auto value0 = static_cast<unsigned int>(value0Char);
    auto value1 = static_cast<unsigned int>(value1Char);
    auto value2 = static_cast<unsigned int>(value2Char);
    
    unsigned int value = value0;
    for(unsigned int bit = 1; bit <= 8; ++bit)
    {
        unsigned int mask = 0U << bit;
        if(((value0 & mask) != (value1 & mask)) && ((value1 & mask) == (value2 & mask)))
        {
            value = (value & ~mask) | (value1 & mask);
        }
    }
    return value;
}

auto PersistentWariableWrite(char32_t address, unsigned int data, unsigned int blockSize) -> void
{
    auto dataChar = static_cast<char>(data);
    FramWrite(address, &dataChar, 1);
    FramWrite(address+blockSize, &dataChar, 1);
    FramWrite(address+2*blockSize, &dataChar, 1);
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}
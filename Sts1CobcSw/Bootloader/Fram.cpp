#include <Sts1CobcSw/Bootloader/BusyWait.hpp>
#include <Sts1CobcSw/Bootloader/Spi.hpp>
#include <Sts1CobcSw/Bootloader/stm32f411xe.h>
#include <Sts1CobcSw/Bootloader/UciUart.hpp>
#include <Sts1CobcSw/Bootloader/Utilities.hpp>

namespace sts1cobcsw::bootloader::fram
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

auto SetCsPin() -> void
{
    static constexpr auto chipSelectDelay = 50;
    GPIOB->ODR |= (GPIO_ODR_OD13);  // Set Cs Pin
    BusyWaitUs(chipSelectDelay);
}


auto ResetCsPin() -> void
{
    static constexpr auto chipSelectDelay = 50;
    GPIOB->ODR &= ~(GPIO_ODR_OD13);  // Reset Cs Pin
    BusyWaitUs(chipSelectDelay);
}


auto Initialize() -> void
{
    spi::Initialize();
    GPIOB->ODR |= (GPIO_ODR_OD13);
}

auto Reset() -> void
{
    GPIOB->ODR &= ~(GPIO_ODR_OD13);
}


auto ReadId() -> void
{
    char rx[9] = {0};

    uciuart::Write("Fram ID: ");

    ResetCsPin();
    spi::Write(opcode::readDeviceId);
    //spi::Read(&rx[0]);

    for(char & i : rx)
    {
        spi::Read(&i);
    }

    SetCsPin();
    
    //char *idString = new char;
    utilities::PrintHexString(static_cast<char *>(rx), 9);
    //uciuart::Write(idString);
    uciuart::Write("\n");
}


auto Write(unsigned long address, char const * string, int size) -> void
{
    ResetCsPin();
    spi::Write(opcode::setWriteEnableLatch);
    SetCsPin();

    ResetCsPin();
    spi::Write(opcode::writeData);
    spi::Write((address & 0x00FF'0000) >> 16);
    spi::Write((address & 0x0000'FF00) >> 8);
    spi::Write(address & 0x0000'00FF);
    
    for(int i = 0; i < size; i++)
    {
        spi::Write(string[i]);
    }
    while((SPI3->SR & SPI_SR_TXE) == 0) {}  // Wait until transmission is complete
    SetCsPin();
}


auto Read(unsigned long address, char ** stringPass, int size) -> void
{
    unsigned int adressByte0 = (address & 0x00FF'0000) >> 16;
    unsigned int adressByte1 = (address & 0x0000'FF00) >> 8;
    unsigned int adressByte2 = address & 0x0000'00FF;
    char *character = 0;
    char string[size];
    
    ResetCsPin();
    
    spi::Write(opcode::readData);
    spi::Write(static_cast<char>(adressByte0));
    spi::Write(static_cast<char>(adressByte1));
    spi::Write(static_cast<char>(adressByte2));

    while((SPI3->SR & SPI_SR_TXE) == 0) {}
    for(int i = 0; i < size; i++)
    {
        spi::Read(&string[i]);
    }
    while(SPI_SR_RXNE == 0) {}
    SetCsPin();
    
    // Copy the local idString to the memory pointed to by idStringPass
    if(stringPass && *stringPass)
    {
        for(int i = 0; i < size; ++i)
        {
            (*stringPass)[i] = string[i];
        }
        //(*stringPass)[len] = '\0'; // Null-terminate the string
    }
    
}


auto PersistentWariableRead(char32_t address, unsigned long int blockSize) -> unsigned int
{
    auto *value0Char = static_cast<char*>(nullptr);
    auto *value1Char = static_cast<char*>(nullptr);
    auto *value2Char = static_cast<char*>(nullptr);
    Read(address, &value0Char, 1);
    Read(address + blockSize, &value1Char, 1);
    Read(address + 2 * blockSize, &value2Char, 1);
    auto value0 = static_cast<unsigned int>(*value0Char);
    auto value1 = static_cast<unsigned int>(*value1Char);
    auto value2 = static_cast<unsigned int>(*value2Char);

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


auto PersistentWariableWrite(char32_t address, unsigned int data, unsigned long int blockSize) -> void
{
    auto dataChar = static_cast<char>(data);
    Write(address, &dataChar, 1);
    Write(address + blockSize, &dataChar, 1);
    Write(address + 2 * blockSize, &dataChar, 1);
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}

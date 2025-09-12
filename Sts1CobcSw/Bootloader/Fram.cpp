#include <Sts1CobcSw/Bootloader/Fram.hpp>

#include <Sts1CobcSw/Bootloader/Spi.hpp>
#include <Sts1CobcSw/CmsisDevice/stm32f411xe.h>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <bit>


namespace sts1cobcsw::fram
{
using sts1cobcsw::operator""_b;


namespace
{
constexpr auto addressEndianness = std::endian::big;

namespace opcode
{
constexpr auto writeData = 0x02_b;
constexpr auto readData = 0x03_b;
constexpr auto setWriteEnableLatch = 0x06_b;
constexpr auto readDeviceId = 0x9F_b;
}


auto SelectChip() -> void;
auto DeselectChip() -> void;
auto SetWriteEnableLatch() -> void;
}


// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

auto Initialize() -> void
{
    spi::Initialize();
    // Configure PB13 as chip select (CS) pin
    GPIOB->MODER |= (GPIO_MODER_MODER13_0);     // Output mode
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED13);  // High speed
    GPIOB->ODR |= (GPIO_ODR_OD13);              // Pull CS pin high
}


auto Reset() -> void
{
    GPIOB->ODR &= ~(GPIO_ODR_OD13);  // Pull CS pin low
    // Reset PB13 to default state
    GPIOB->MODER &= ~(GPIO_MODER_MODER13_0);
    GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED13);
    sts1cobcsw::spi::Reset();
}


auto ReadId() -> std::array<Byte, framIdSize>
{
    auto framId = std::array<Byte, framIdSize>{};
    SelectChip();
    spi::Write(opcode::readDeviceId);
    spi::WaitUntilTxComplete();
    spi::Read(std::span(framId));
    DeselectChip();
    return framId;
}


auto Write(std::uint32_t address, std::span<Byte const> data) -> void
{
    SetWriteEnableLatch();
    SelectChip();
    spi::Write(opcode::writeData);
    // FRAM expects 3 address bytes
    auto serializedAddress = Serialize<addressEndianness>(address);
    spi::Write(std::span(serializedAddress).last<3>());
    spi::Write(data);
    spi::WaitUntilTxComplete();
    DeselectChip();
}


auto Read(std::uint32_t address, std::span<Byte> data) -> void
{
    SelectChip();
    spi::Write(opcode::readData);
    // FRAM expects 3 address bytes
    auto serializedAddress = Serialize<addressEndianness>(address);
    spi::Write(std::span(serializedAddress).last<3>());
    spi::WaitUntilTxComplete();
    spi::Read(data);
    DeselectChip();
}


namespace
{
auto SelectChip() -> void
{
    // At 16 MHz a single clock cycle is 62.5 ns, which is larger than any delay or hold duration
    // that the FRAM datasheet specifies for the CS pin. Therefore, we don't wait for anything here.
    GPIOB->ODR &= ~(GPIO_ODR_OD13);  // Pull CS pin low
}


auto DeselectChip() -> void
{
    // At 16 MHz a single clock cycle is 62.5 ns, which is larger than any delay or hold duration
    // that the FRAM datasheet specifies for the CS pin. Therefore, we don't wait for anything here.
    GPIOB->ODR |= (GPIO_ODR_OD13);  // Pull CS pin high
}


auto SetWriteEnableLatch() -> void
{
    SelectChip();
    spi::Write(opcode::setWriteEnableLatch);
    spi::WaitUntilTxComplete();
    DeselectChip();
}
}
#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}

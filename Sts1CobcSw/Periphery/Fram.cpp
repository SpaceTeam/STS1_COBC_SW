// FRAM chip: CY15B108QN-40SXI

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::periphery::fram
{
using serial::operator""_b;


// --- Private globals ---

// Command opcodes according to section 4.1 in CY15B108QN-40SXI datasheet
namespace opcode
{
constexpr auto writeData = 0x02_b;
constexpr auto readData = 0x03_b;
constexpr auto setWriteEnableLatch = 0x06_b;
constexpr auto readDeviceId = 0x9F_b;
}

auto csGpioPin = hal::GpioPin(hal::framCsPin);
auto spi = RODOS::HAL_SPI(
    hal::framEpsSpiIndex, hal::framEpsSpiSckPin, hal::framEpsSpiMisoPin, hal::framEpsSpiMosiPin);


// --- Private function declarations ---

auto SetWriteEnableLatch() -> void;


// --- Public function definitions

auto Initialize() -> std::int32_t
{
    csGpioPin.Direction(hal::PinDirection::out);
    csGpioPin.Set();

    constexpr auto baudrate = 1'000'000;
    return spi.init(baudrate, /*slave=*/false, /*tiMode=*/false);
}


auto ReadDeviceId() -> DeviceId
{
    csGpioPin.Reset();
    spi.write(&opcode::readDeviceId, sizeof(opcode::readDeviceId));
    auto deviceId = DeviceId{};
    spi.read(deviceId.data(), deviceId.size());
    csGpioPin.Set();
    return deviceId;
}


auto ReadFrom(Address address, void * data, std::size_t size) -> void
{
    auto addressBytes = serial::Serialize(address);
    // FRAM expects 3-byte address in big endian
    auto commandMessage =
        std::array{opcode::readData, addressBytes[2], addressBytes[1], addressBytes[0]};

    csGpioPin.Reset();
    spi.write(commandMessage.data(), commandMessage.size());
    spi.read(data, size);
    csGpioPin.Set();
}


auto WriteTo(Address address, void const * data, std::size_t size) -> void
{
    auto addressBytes = serial::Serialize(address);
    // FRAM expects 3-byte address in big endian
    auto commandMessage =
        std::array{opcode::writeData, addressBytes[2], addressBytes[1], addressBytes[0]};

    SetWriteEnableLatch();
    csGpioPin.Reset();
    spi.write(commandMessage.data(), commandMessage.size());
    spi.write(data, size);
    csGpioPin.Set();
}


// --- Private function definitions ---

auto SetWriteEnableLatch() -> void
{
    csGpioPin.Reset();
    spi.write(&opcode::setWriteEnableLatch, sizeof(opcode::setWriteEnableLatch));
    csGpioPin.Set();
}
}

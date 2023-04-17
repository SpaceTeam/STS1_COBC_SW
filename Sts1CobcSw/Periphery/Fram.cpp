// FRAM chip: CY15B108QN-40SXI

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::periphery::fram
{
using serial::operator""_b;


// Commands according to section 4.1 in CY15B108QN-40SXI datasheet
namespace command
{
constexpr auto readDeviceId = 0x9F_b;
constexpr auto readData = 0x03_b;
}

auto csGpioPin = hal::GpioPin(hal::framCsPin);
auto spi =
    RODOS::HAL_SPI(hal::framSpiIndex, hal::framSpiSckPin, hal::framSpiMisoPin, hal::framSpiMosiPin);


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
    spi.write(&command::readDeviceId, sizeof(command::readDeviceId));
    auto deviceId = DeviceId{};
    spi.read(deviceId.data(), deviceId.size());
    csGpioPin.Set();
    return deviceId;
}


namespace details
{
auto Read(std::uint32_t address, std::span<Byte> data) -> void
{
    auto addressBytes = serial::Serialize(address);
    // FRAM expects 3-byte address in big endian
    auto message = std::array{addressBytes[2], addressBytes[1], addressBytes[0]};

    csGpioPin.Reset();
    spi.write(&command::readData, sizeof(command::readData));
    spi.write(message.data(), message.size());
    spi.read(data.data(), data.size());
    csGpioPin.Set();
}
}
}

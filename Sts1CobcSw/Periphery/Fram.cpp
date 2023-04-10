// FRAM chip: CY15B108QN-40SXI

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>

#include <rodos_no_using_namespace.h>

#include <span>


namespace sts1cobcsw::periphery::fram
{
using serial::operator""_b;


// Instructions according to section 4.1 in CY15B108QN-40SXI datasheet
constexpr auto readDeviceID = 0x9F_b;

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
    auto message = std::array{readDeviceID};
    hal::WriteToReadFrom(&spi, std::span(message));
    auto dummyBytes = DeviceId{};
    auto deviceId =  hal::WriteToReadFrom(&spi, std::span(dummyBytes));
    csGpioPin.Set();
    return deviceId;
}
}

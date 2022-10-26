#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <span>
#include <string_view>


namespace sts1cobcsw
{
namespace serial
{
template<>
inline constexpr std::size_t serialSize<periphery::flash::JedecId> =
    totalSerialSize<decltype(periphery::flash::JedecId::manufacturerId),
                    decltype(periphery::flash::JedecId::deviceId)>;
}


namespace periphery::flash
{
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::operator""_b;


// Private globals
constexpr auto dummyByte = 0x00_b;

auto csGpioPin = hal::GpioPin(hal::flashCsPin);
auto writeProtectionGpioPin = hal::GpioPin(hal::flashWriteProtectionPin);
auto spi = RODOS::HAL_SPI(
    hal::flashSpiIndex, hal::flashSpiSckPin, hal::flashSpiMisoPin, hal::flashSpiMosiPin);


// Private function declarations
template<std::size_t nBytes>
[[nodiscard]] auto WriteRead(std::span<Byte, nBytes> data) -> std::array<Byte, nBytes>;

auto DeserializeFrom(Byte * source, JedecId * jedecId) -> Byte *;


// Public function definitions
[[nodiscard]] auto Initialize() -> std::int32_t
{
    csGpioPin.Direction(hal::PinDirection::out);
    writeProtectionGpioPin.Direction(hal::PinDirection::out);
    csGpioPin.Set();
    writeProtectionGpioPin.Set();

    constexpr auto baudrate = 1'000'000;
    return spi.init(baudrate, /*slave=*/false, /*tiMode=*/false);
}


[[nodiscard]] auto ReadJedecId() -> JedecId
{
    constexpr auto deviceIdRegister = 0x9F_b;
    auto message = std::array{deviceIdRegister, dummyByte, dummyByte, dummyByte};

    csGpioPin.Reset();
    auto answer = WriteRead(std::span(message));
    csGpioPin.Set();

    return serial::Deserialize<JedecId>(std::span(answer).last<serial::serialSize<JedecId>>());
}


// Private function definitions
template<std::size_t nBytes>
[[nodiscard]] inline auto WriteRead(std::span<Byte, nBytes> data) -> std::array<Byte, nBytes>
{
    return hal::WriteToReadFrom(&spi, data);
}


auto DeserializeFrom(Byte * source, JedecId * jedecId) -> Byte *
{
    source = serial::DeserializeFrom<std::uint8_t>(source, &(jedecId->manufacturerId));
    source = serial::DeserializeFrom<std::uint16_t>(source, &(jedecId->deviceId));
    return source;
}
}
}
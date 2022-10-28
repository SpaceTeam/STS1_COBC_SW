#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
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
using serial::operator""_b;


struct SimpleInstruction
{
    Byte id = 0x00_b;
    std::size_t answerLength = 0;
};


// --- Private globals ---

// Instructions according to section 7.2 in W25Q01JV datasheet
constexpr auto readJedecId = SimpleInstruction{.id = 0x9F_b, .answerLength = 3};
constexpr auto readStatusRegister1 = SimpleInstruction{.id = 0x05_b, .answerLength = 1};
constexpr auto readStatusRegister2 = SimpleInstruction{.id = 0x35_b, .answerLength = 1};
constexpr auto readStatusRegister3 = SimpleInstruction{.id = 0x15_b, .answerLength = 1};
constexpr auto enter4ByteAdressMode = SimpleInstruction{.id = 0xB7_b, .answerLength = 0};
constexpr auto readData4ByteAddress = 0x13_b;

auto csGpioPin = hal::GpioPin(hal::flashCsPin);
auto writeProtectionGpioPin = hal::GpioPin(hal::flashWriteProtectionPin);
auto spi = RODOS::HAL_SPI(
    hal::flashSpiIndex, hal::flashSpiSckPin, hal::flashSpiMisoPin, hal::flashSpiMosiPin);


// --- Private function declarations ---

auto Enter4ByteAdressMode() -> void;

template<std::size_t nBytes>
[[nodiscard]] auto WriteRead(std::span<Byte, nBytes> data) -> std::array<Byte, nBytes>;

template<std::size_t nReadBytes, std::size_t nWriteBytes>
[[nodiscard]] auto WriteRead(std::span<Byte, nWriteBytes> message) -> std::array<Byte, nReadBytes>;

template<SimpleInstruction const & instruction>
    requires(instruction.answerLength > 0)
[[nodiscard]] auto SendInstruction() -> std::array<Byte, instruction.answerLength>;

template<SimpleInstruction const & instruction>
    requires(instruction.answerLength == 0)
auto SendInstruction() -> void;

auto DeserializeFrom(Byte * source, JedecId * jedecId) -> Byte *;


// ---Public function definitions ---

[[nodiscard]] auto Initialize() -> std::int32_t
{
    csGpioPin.Direction(hal::PinDirection::out);
    writeProtectionGpioPin.Direction(hal::PinDirection::out);
    csGpioPin.Set();
    writeProtectionGpioPin.Set();

    constexpr auto baudrate = 1'000'000;
    return spi.init(baudrate, /*slave=*/false, /*tiMode=*/false);

    periphery::flash::Enter4ByteAdressMode();
}


[[nodiscard]] auto ReadJedecId() -> JedecId
{
    csGpioPin.Reset();
    auto answer = SendInstruction<readJedecId>();
    csGpioPin.Set();
    return serial::Deserialize<JedecId>(std::span(answer));
}


[[nodiscard]] auto ReadStatusRegister(int8_t registerNo) -> Byte
{
    auto statusRegister = 0xFF_b;  // NOLINT

    csGpioPin.Reset();
    if(registerNo == 1)
    {
        statusRegister = SendInstruction<readStatusRegister1>()[0];
    }
    else if(registerNo == 2)
    {
        statusRegister = SendInstruction<readStatusRegister2>()[0];
    }
    else if(registerNo == 3)
    {
        statusRegister = SendInstruction<readStatusRegister3>()[0];
    }
    csGpioPin.Set();

    return statusRegister;
}


[[nodiscard]] auto ReadPage(std::uint32_t address) -> Page
{
    auto addressBytes = serial::Serialize(address);
    auto message = std::array<Byte, 1 + size(addressBytes)>{readData4ByteAddress};
    std::copy(rbegin(addressBytes), rend(addressBytes), begin(message) + 1);

    csGpioPin.Reset();
    auto page = WriteRead<pageSize>(std::span(message));
    csGpioPin.Set();

    return page;
}


// --- Private function definitions ---

auto Enter4ByteAdressMode() -> void
{
    csGpioPin.Reset();
    SendInstruction<enter4ByteAdressMode>();
    csGpioPin.Set();
}


template<std::size_t nBytes>
[[nodiscard]] inline auto WriteRead(std::span<Byte, nBytes> data) -> std::array<Byte, nBytes>
{
    return hal::WriteToReadFrom(&spi, data);
}


template<std::size_t nReadBytes, std::size_t nWriteBytes>
[[nodiscard]] auto WriteRead(std::span<Byte, nWriteBytes> message) -> std::array<Byte, nReadBytes>
{
    hal::WriteToReadFrom(&spi, message);
    auto dummyBytes = std::array<Byte, nReadBytes>{};
    return hal::WriteToReadFrom(&spi, std::span(dummyBytes));
}


template<SimpleInstruction const & instruction>
    requires(instruction.answerLength > 0)
[[nodiscard]] auto SendInstruction() -> std::array<Byte, instruction.answerLength>
{
    auto message1 = std::array{instruction.id};
    hal::WriteToReadFrom(&spi, std::span(message1));
    auto message2 = std::array<Byte, instruction.answerLength>{};
    return hal::WriteToReadFrom(&spi, std::span(message2));
}


template<SimpleInstruction const & instruction>
    requires(instruction.answerLength == 0)
inline auto SendInstruction() -> void
{
    auto message1 = std::array{instruction.id};
    hal::WriteToReadFrom(&spi, std::span(message1));
}


// TODO: Replace this super ugyl hack with a proper big endian deserialization
auto DeserializeFrom(Byte * source, JedecId * jedecId) -> Byte *
{
    source = serial::DeserializeFrom<std::uint8_t>(source, &(jedecId->manufacturerId));
    std::uint16_t deviceId = 0;
    source = serial::DeserializeFrom<std::uint16_t>(source, &(deviceId));
    auto deviceIdBytes = serial::Serialize(deviceId);
    std::reverse(begin(deviceIdBytes), end(deviceIdBytes));
    jedecId->deviceId = serial::Deserialize<std::uint16_t>(std::span(deviceIdBytes));
    return source;
}
}
}
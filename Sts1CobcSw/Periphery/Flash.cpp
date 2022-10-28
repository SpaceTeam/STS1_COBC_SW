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

auto csGpioPin = hal::GpioPin(hal::flashCsPin);
auto writeProtectionGpioPin = hal::GpioPin(hal::flashWriteProtectionPin);
auto spi = RODOS::HAL_SPI(
    hal::flashSpiIndex, hal::flashSpiSckPin, hal::flashSpiMisoPin, hal::flashSpiMosiPin);


// --- Private function declarations ---

template<std::size_t nBytes>
[[nodiscard]] auto WriteRead(std::span<Byte, nBytes> data) -> std::array<Byte, nBytes>;

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
    switch(registerNo)
    {
        case 1:
        {
            csGpioPin.Reset();
            auto statusRegister = SendInstruction<readStatusRegister1>()[0];
            csGpioPin.Set();
            return statusRegister;
        }
        case 2:
        {
            csGpioPin.Reset();
            auto statusRegister = SendInstruction<readStatusRegister2>()[0];
            csGpioPin.Set();
            return statusRegister;
        }
        case 3:
        {
            csGpioPin.Reset();
            auto statusRegister = SendInstruction<readStatusRegister3>()[0];
            csGpioPin.Set();
            return statusRegister;
        }
        default:
        {
            // TODO: Proper error handling
            return 0xFF_b;  // NOLINT
        }
    }
}


// --- Private function definitions ---

template<std::size_t nBytes>
[[nodiscard]] inline auto WriteRead(std::span<Byte, nBytes> data) -> std::array<Byte, nBytes>
{
    return hal::WriteToReadFrom(&spi, data);
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


auto DeserializeFrom(Byte * source, JedecId * jedecId) -> Byte *
{
    source = serial::DeserializeFrom<std::uint8_t>(source, &(jedecId->manufacturerId));
    source = serial::DeserializeFrom<std::uint16_t>(source, &(jedecId->deviceId));
    return source;
}
}
}
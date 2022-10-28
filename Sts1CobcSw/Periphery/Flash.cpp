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
struct Instruction
{
    Byte id = 0x00_b;
    std::size_t answerLength = 0;
};


// --- Private globals ---

// Instructions according to section 7.2 in W25Q01JV datasheet
constexpr auto readJedecId = Instruction{.id = 0x9F_b, .answerLength = 3};
constexpr auto readStatusRegister1 = Instruction{.id = 0x05_b, .answerLength = 1};
constexpr auto readStatusRegister2 = Instruction{.id = 0x35_b, .answerLength = 1};
constexpr auto readStatusRegister3 = Instruction{.id = 0x15_b, .answerLength = 1};

auto csGpioPin = hal::GpioPin(hal::flashCsPin);
auto writeProtectionGpioPin = hal::GpioPin(hal::flashWriteProtectionPin);
auto spi = RODOS::HAL_SPI(
    hal::flashSpiIndex, hal::flashSpiSckPin, hal::flashSpiMisoPin, hal::flashSpiMosiPin);


// --- Private function declarations ---

template<std::size_t nBytes>
[[nodiscard]] auto WriteRead(std::span<Byte, nBytes> data) -> std::array<Byte, nBytes>;

template<Instruction const & instruction>
    requires(instruction.answerLength > 0)
[[nodiscard]] inline auto SendInstruction() -> std::array<Byte, instruction.answerLength>;

template<Instruction const & instruction>
    requires(instruction.answerLength == 0)
inline auto SendInstruction() -> void;

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


[[nodiscard]] auto ReadStatusRegisters() -> StatusRegisters
{
    auto statusRegisters = StatusRegisters();

    csGpioPin.Reset();
    statusRegisters.one = SendInstruction<readStatusRegister1>()[0];
    csGpioPin.Set();
    csGpioPin.Reset();
    statusRegisters.two = SendInstruction<readStatusRegister2>()[0];
    csGpioPin.Set();
    csGpioPin.Reset();
    statusRegisters.three = SendInstruction<readStatusRegister3>()[0];
    csGpioPin.Set();

    return statusRegisters;
}


// --- Private function definitions ---

template<std::size_t nBytes>
[[nodiscard]] inline auto WriteRead(std::span<Byte, nBytes> data) -> std::array<Byte, nBytes>
{
    return hal::WriteToReadFrom(&spi, data);
}


template<Instruction const & instruction>
    requires(instruction.answerLength > 0)
[[nodiscard]] inline auto SendInstruction() -> std::array<Byte, instruction.answerLength>
{
    auto message1 = std::array{instruction.id};
    hal::WriteToReadFrom(&spi, std::span(message1));
    auto message2 = std::array<Byte, instruction.answerLength>{};
    return hal::WriteToReadFrom(&spi, std::span(message2));
}


template<Instruction const & instruction>
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
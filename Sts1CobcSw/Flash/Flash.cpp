#include <Sts1CobcSw/Flash/Flash.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>

#include <array>
#include <bit>
#include <compare>


namespace sts1cobcsw
{
template<>
inline constexpr std::size_t serialSize<flash::JedecId> =
    totalSerialSize<decltype(flash::JedecId::manufacturerId), decltype(flash::JedecId::deviceId)>;


namespace flash
{
using sts1cobcsw::DeserializeFrom;


struct SimpleInstruction
{
    Byte id = 0x00_b;
    std::size_t answerLength = 0;
};


// --- Private globals ---

// Baud rate = 48 MHz, largest data transfer = 1 page = 256 bytes -> spiTimeout = 1 ms is enough for
// all transfers
constexpr auto spiTimeout = 1 * ms;
constexpr auto endianness = std::endian::big;

// Instructions according to section 7.3 in W25Q01JV datasheet
constexpr auto readJedecId = SimpleInstruction{.id = 0x9F_b, .answerLength = 3};
constexpr auto readStatusRegister1 = SimpleInstruction{.id = 0x05_b, .answerLength = 1};
constexpr auto readStatusRegister2 = SimpleInstruction{.id = 0x35_b, .answerLength = 1};
constexpr auto readStatusRegister3 = SimpleInstruction{.id = 0x15_b, .answerLength = 1};
constexpr auto writeEnable = SimpleInstruction{.id = 0x06_b, .answerLength = 0};
constexpr auto writeDisable = SimpleInstruction{.id = 0x04_b, .answerLength = 0};

constexpr auto readData4ByteAddress = 0x13_b;
constexpr auto pageProgram4ByteAddress = 0x12_b;
constexpr auto sectorErase4ByteAddress = 0x21_b;

auto csGpioPin = hal::GpioPin(hal::flashCsPin);
auto writeProtectionGpioPin = hal::GpioPin(hal::flashWriteProtectionPin);


// --- Private function declarations ---

auto EnableWriting() -> void;
auto DisableWriting() -> void;
auto IsBusy() -> bool;

template<std::size_t extent>
auto Write(std::span<Byte const, extent> data, Duration timeout) -> void;

template<std::size_t extent>
auto Read(std::span<Byte, extent> data, Duration timeout) -> void;

template<std::size_t size>
auto Read(Duration timeout) -> std::array<Byte, size>;

template<SimpleInstruction const & instruction>
    requires(instruction.answerLength > 0)
[[nodiscard]] auto SendInstruction() -> std::array<Byte, instruction.answerLength>;

template<SimpleInstruction const & instruction>
    requires(instruction.answerLength == 0)
auto SendInstruction() -> void;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, JedecId * jedecId) -> void const *;


// ---Public function definitions ---

auto Initialize() -> void
{
    csGpioPin.SetDirection(hal::PinDirection::out);
    csGpioPin.Set();
    writeProtectionGpioPin.SetDirection(hal::PinDirection::out);
    writeProtectionGpioPin.Set();
    auto const baudRate = 48'000'000;
    Initialize(&flashSpi, baudRate);
}


auto ReadJedecId() -> JedecId
{
    csGpioPin.Reset();
    auto answer = SendInstruction<readJedecId>();
    csGpioPin.Set();
    return Deserialize<endianness, JedecId>(Span(answer));
}


// TODO: Only read status register 1
auto ReadStatusRegister(int8_t registerNo) -> Byte
{
    auto statusRegister = 0xFF_b;  // NOLINT(*magic-numbers*)
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


auto ReadPage(std::uint32_t address) -> Page
{
    csGpioPin.Reset();
    Write(Span(readData4ByteAddress), spiTimeout);
    Write(Span(Serialize<endianness>(address)), spiTimeout);
    auto page = Read<pageSize>(spiTimeout);
    csGpioPin.Set();
    return page;
}


auto ProgramPage(std::uint32_t address, PageSpan data) -> void
{
    EnableWriting();
    csGpioPin.Reset();
    Write(Span(pageProgram4ByteAddress), spiTimeout);
    Write(Span(Serialize<endianness>(address)), spiTimeout);
    Write(data, spiTimeout);
    csGpioPin.Set();
    DisableWriting();
}


auto EraseSector(std::uint32_t address) -> void
{
    // Round address down to the nearest sector address.
    address = (address / sectorSize) * sectorSize;
    EnableWriting();
    csGpioPin.Reset();
    Write(Span(sectorErase4ByteAddress), spiTimeout);
    Write(Span(Serialize<endianness>(address)), spiTimeout);
    csGpioPin.Set();
    DisableWriting();
}


auto WaitWhileBusy(Duration timeout) -> Result<void>
{
    auto const pollingCycleTime = 1 * ms;
    auto const reactivationTime = CurrentRodosTime() + timeout;
    while(IsBusy())
    {
        if(CurrentRodosTime() >= reactivationTime)
        {
            return ErrorCode::timeout;
        }
        SuspendFor(pollingCycleTime);
    }
    return outcome_v2::success();
}


auto ActualBaudRate() -> std::int32_t
{
    return flashSpi.BaudRate();
}


// --- Private function definitions ---

auto EnableWriting() -> void
{
    csGpioPin.Reset();
    SendInstruction<writeEnable>();
    csGpioPin.Set();
}


auto DisableWriting() -> void
{
    csGpioPin.Reset();
    SendInstruction<writeDisable>();
    csGpioPin.Set();
}


auto IsBusy() -> bool
{
    auto const busyBitMask = 0x01_b;
    return (ReadStatusRegister(1) & busyBitMask) == busyBitMask;
};


template<std::size_t extent>
inline auto Write(std::span<Byte const, extent> data, Duration timeout) -> void
{
    hal::WriteTo(&flashSpi, data, timeout);
}


template<std::size_t extent>
inline auto Read(std::span<Byte, extent> data, Duration timeout) -> void
{
    hal::ReadFrom(&flashSpi, data, timeout);
}


template<std::size_t size>
inline auto Read(Duration timeout) -> std::array<Byte, size>
{
    auto answer = std::array<Byte, size>{};
    hal::ReadFrom(&flashSpi, Span(&answer), timeout);
    return answer;
}


template<SimpleInstruction const & instruction>
    requires(instruction.answerLength > 0)
auto SendInstruction() -> std::array<Byte, instruction.answerLength>
{
    Write(Span(instruction.id), spiTimeout);
    return Read<instruction.answerLength>(spiTimeout);
}


template<SimpleInstruction const & instruction>
    requires(instruction.answerLength == 0)
inline auto SendInstruction() -> void
{
    Write(Span(instruction.id), spiTimeout);
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, JedecId * jedecId) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(jedecId->manufacturerId));
    source = DeserializeFrom<endianness>(source, &(jedecId->deviceId));
    return source;
}
}
}

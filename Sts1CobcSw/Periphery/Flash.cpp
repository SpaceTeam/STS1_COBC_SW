#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <bit>
#include <string_view>


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

constexpr auto endianness = std::endian::big;

// Instructions according to section 7.3 in W25Q01JV datasheet
constexpr auto readJedecId = SimpleInstruction{.id = 0x9F_b, .answerLength = 3};
constexpr auto readStatusRegister1 = SimpleInstruction{.id = 0x05_b, .answerLength = 1};
constexpr auto readStatusRegister2 = SimpleInstruction{.id = 0x35_b, .answerLength = 1};
constexpr auto readStatusRegister3 = SimpleInstruction{.id = 0x15_b, .answerLength = 1};
constexpr auto writeEnable = SimpleInstruction{.id = 0x06_b, .answerLength = 0};
constexpr auto writeDisable = SimpleInstruction{.id = 0x04_b, .answerLength = 0};
constexpr auto enter4ByteAdressMode = SimpleInstruction{.id = 0xB7_b, .answerLength = 0};

constexpr auto readData4ByteAddress = 0x13_b;
constexpr auto pageProgram4ByteAddress = 0x12_b;
constexpr auto sectorErase4ByteAddress = 0x21_b;

auto csGpioPin = hal::GpioPin(hal::flashCsPin);
auto writeProtectionGpioPin = hal::GpioPin(hal::flashWriteProtectionPin);
auto spi = RODOS::HAL_SPI(hal::flashSpiIndex,
                          hal::flashSpiSckPin,
                          hal::flashSpiMisoPin,
                          hal::flashSpiMosiPin,
                          hal::spiNssDummyPin);


// --- Private function declarations ---

auto Enter4ByteAdressMode() -> void;
auto EnableWriting() -> void;
auto DisableWriting() -> void;

template<std::size_t extent>
auto Write(std::span<Byte const, extent> data) -> void;

template<std::size_t extent>
auto Read(std::span<Byte, extent> data) -> void;

template<std::size_t size>
auto Read() -> std::array<Byte, size>;

template<SimpleInstruction const & instruction>
    requires(instruction.answerLength > 0)
[[nodiscard]] auto SendInstruction() -> std::array<Byte, instruction.answerLength>;

template<SimpleInstruction const & instruction>
    requires(instruction.answerLength == 0)
auto SendInstruction() -> void;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, JedecId * jedecId) -> void const *;


// ---Public function definitions ---

auto Initialize() -> std::int32_t
{
    csGpioPin.Direction(hal::PinDirection::out);
    writeProtectionGpioPin.Direction(hal::PinDirection::out);
    csGpioPin.Set();
    writeProtectionGpioPin.Set();

    constexpr auto baudRate = 1'000'000;
    auto errorCode = hal::Initialize(&spi, baudRate);

    Enter4ByteAdressMode();

    return errorCode;
}


auto ReadJedecId() -> JedecId
{
    csGpioPin.Reset();
    auto answer = SendInstruction<readJedecId>();
    csGpioPin.Set();
    return Deserialize<endianness, JedecId>(Span(answer));
}


auto ReadStatusRegister(int8_t registerNo) -> Byte
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


auto ReadPage(std::uint32_t address) -> Page
{
    csGpioPin.Reset();
    Write(Span(readData4ByteAddress));
    Write(Span(Serialize<endianness>(address)));
    auto page = Read<pageSize>();
    csGpioPin.Set();

    return page;
}


// TODO: Maybe check BUSY flag before writing or something
auto ProgramPage(std::uint32_t address, PageSpan data) -> void
{
    EnableWriting();
    csGpioPin.Reset();

    Write(Span(pageProgram4ByteAddress));
    Write(Span(Serialize<endianness>(address)));
    Write(data);

    csGpioPin.Set();
    DisableWriting();
}


auto EraseSector(std::uint32_t address) -> void
{
    // Round address down to the nearest sector address
    address = (address / sectorSize) * sectorSize;

    EnableWriting();
    csGpioPin.Reset();

    Write(Span(sectorErase4ByteAddress));
    Write(Span(Serialize<endianness>(address)));

    csGpioPin.Set();
    DisableWriting();
}


auto WaitWhileBusy() -> void
{
    auto pollingCycleTime = 1 * RODOS::MILLISECONDS;
    auto busyBitMask = 0x01_b;
    auto isBusy = (ReadStatusRegister(1) & busyBitMask) == busyBitMask;
    while(isBusy)
    {
        RODOS::AT(RODOS::NOW() + pollingCycleTime);
        isBusy = (ReadStatusRegister(1) & busyBitMask) == busyBitMask;
    }
}


// --- Private function definitions ---

auto Enter4ByteAdressMode() -> void
{
    csGpioPin.Reset();
    SendInstruction<enter4ByteAdressMode>();
    csGpioPin.Set();
}


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


// TODO:: Maybe this is one level of indirection too much?
template<std::size_t nBytes>
inline auto Write(std::span<Byte const, nBytes> data) -> void
{
    hal::WriteTo(&spi, data);
}


template<std::size_t extent>
inline auto Read(std::span<Byte, extent> data) -> void
{
    hal::ReadFrom(&spi, data);
}


template<std::size_t size>
inline auto Read() -> std::array<Byte, size>
{
    return hal::ReadFrom<size>(&spi);
}


template<SimpleInstruction const & instruction>
    requires(instruction.answerLength > 0)
auto SendInstruction() -> std::array<Byte, instruction.answerLength>
{
    Write(Span(instruction.id));
    return Read<instruction.answerLength>();
}


template<SimpleInstruction const & instruction>
    requires(instruction.answerLength == 0)
inline auto SendInstruction() -> void
{
    Write(Span(instruction.id));
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

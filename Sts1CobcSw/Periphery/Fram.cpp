//! @file
//! @brief  Low-level driver for the FRAM chip CY15B108QN-40SXI

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramEspSpi.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

#include <bit>


namespace sts1cobcsw::fram
{
// --- Private globals ---

constexpr auto endianness = std::endian::big;

// Command opcodes according to section 4.1 in CY15B108QN-40SXI datasheet. I couldn't use an enum
// class because std::byte is already an enum class so it cannot be used as the underlying type of
// another enum.
namespace opcode
{
constexpr auto writeData = 0x02_b;
constexpr auto readData = 0x03_b;
constexpr auto setWriteEnableLatch = 0x06_b;
constexpr auto readDeviceId = 0x9F_b;
}

auto csGpioPin = hal::GpioPin(hal::framCsPin);

// --- Private function declarations ---

auto SetWriteEnableLatch(std::int64_t timeout = RODOS::END_OF_TIME) -> void;


// --- Public function definitions ---

auto Initialize() -> void
{
    csGpioPin.Direction(hal::PinDirection::out);
    csGpioPin.Set();

    auto const baudRate = 6'000'000;
    Initialize(&spi, baudRate);
}


auto ReadDeviceId(std::int64_t timeout) -> DeviceId
{
    csGpioPin.Reset();
    hal::WriteTo(&spi, Span(opcode::readDeviceId), timeout);
    auto deviceId = DeviceId{};
    hal::ReadFrom(&spi, Span(&deviceId), timeout);
    csGpioPin.Set();
    return deviceId;
}


auto ActualBaudRate() -> int32_t
{
    return spi.Status();
}


namespace internal
{
auto WriteTo(Address address, void const * data, std::size_t nBytes, std::int64_t timeout) -> void
{
    SetWriteEnableLatch();
    csGpioPin.Reset();
    hal::WriteTo(&spi, Span(opcode::writeData), timeout);
    // FRAM expects 3-byte address in big endian
    hal::WriteTo(&spi, Span(Serialize<endianness>(address)).subspan<1, 3>(), timeout);
    hal::WriteTo(&spi, std::span(static_cast<Byte const *>(data), nBytes), timeout);
    csGpioPin.Set();
}


auto ReadFrom(Address address, void * data, std::size_t nBytes, std::int64_t timeout) -> void
{
    csGpioPin.Reset();
    hal::WriteTo(&spi, Span(opcode::readData), timeout);
    // FRAM expects 3-byte address in big endian
    hal::WriteTo(&spi, Span(Serialize<endianness>(address)).subspan<1, 3>(), timeout);
    hal::ReadFrom(&spi, std::span(static_cast<Byte *>(data), nBytes), timeout);
    csGpioPin.Set();
}
}


// --- Private function definitions ---

auto SetWriteEnableLatch(std::int64_t timeout) -> void
{
    csGpioPin.Reset();
    hal::WriteTo(&spi, Span(opcode::setWriteEnableLatch), timeout);
    csGpioPin.Set();
}
}

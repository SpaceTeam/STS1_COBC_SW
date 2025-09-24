//! @file
//! @brief  Low-level driver for the FRAM chip CY15B108QN-40SXI

#include <Sts1CobcSw/Fram/Fram.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <bit>


namespace sts1cobcsw::fram
{
// --- Public globals ---

EdacVariable<bool> framIsWorking(true);
RODOS::Semaphore framEpsSemaphore{};


namespace
{
// --- Private globals ---

constexpr auto spiTimeout = 1 * ms;
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

auto SelectChip() -> void;
auto DeselectChip() -> void;
auto SetWriteEnableLatch() -> void;
}


// --- Public function definitions ---

auto Initialize() -> void
{
    auto protector = RODOS::ScopeProtector(&framEpsSemaphore);
    csGpioPin.SetDirection(hal::PinDirection::out);
    csGpioPin.Set();
    auto const baudRate = 6'000'000;
    Initialize(&framEpsSpi, baudRate);
}


auto ReadDeviceId() -> DeviceId
{
    auto protector = RODOS::ScopeProtector(&framEpsSemaphore);
    SelectChip();
    hal::WriteTo(&framEpsSpi, Span(opcode::readDeviceId), spiTimeout);
    auto deviceId = DeviceId{};
    hal::ReadFrom(&framEpsSpi, Span(&deviceId), spiTimeout);
    DeselectChip();
    return deviceId;
}


auto ActualBaudRate() -> std::int32_t
{
    auto protector = RODOS::ScopeProtector(&framEpsSemaphore);
    return framEpsSpi.BaudRate();
}


namespace internal
{
auto WriteTo(Address address, void const * data, std::size_t nBytes, Duration timeout) -> void
{
    auto protector = RODOS::ScopeProtector(&framEpsSemaphore);
    SetWriteEnableLatch();
    SelectChip();
    hal::WriteTo(&framEpsSpi, Span(opcode::writeData), spiTimeout);
    // FRAM expects 3-byte address in big endian
    hal::WriteTo(
        &framEpsSpi, Span(Serialize<endianness>(value_of(address))).subspan<1, 3>(), spiTimeout);
    hal::WriteTo(&framEpsSpi, std::span(static_cast<Byte const *>(data), nBytes), timeout);
    DeselectChip();
}


auto ReadFrom(Address address, void * data, std::size_t nBytes, Duration timeout) -> void
{
    auto protector = RODOS::ScopeProtector(&framEpsSemaphore);
    SelectChip();
    hal::WriteTo(&framEpsSpi, Span(opcode::readData), spiTimeout);
    // FRAM expects 3-byte address in big endian
    hal::WriteTo(
        &framEpsSpi, Span(Serialize<endianness>(value_of(address))).subspan<1, 3>(), spiTimeout);
    hal::ReadFrom(&framEpsSpi, std::span(static_cast<Byte *>(data), nBytes), timeout);
    DeselectChip();
}
}


// --- Private function definitions ---

namespace
{
auto SelectChip() -> void
{
    static constexpr auto chipSelectDelay = 10 * ns;
    csGpioPin.Reset();
    BusyWaitFor(chipSelectDelay);
}


auto DeselectChip() -> void
{
    static constexpr auto chipSelectHoldDuration = 10 * ns;
    static constexpr auto deselectDelay = 60 * ns;
    BusyWaitFor(chipSelectHoldDuration);
    csGpioPin.Set();
    BusyWaitFor(deselectDelay);
}


auto SetWriteEnableLatch() -> void
{
    SelectChip();
    hal::WriteTo(&framEpsSpi, Span(opcode::setWriteEnableLatch), spiTimeout);
    DeselectChip();
}
}
}

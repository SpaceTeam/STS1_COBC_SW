#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::periphery::rf
{
// --- Globals

auto spi = RODOS::HAL_SPI(hal::rfSpiIndex, hal::rfSpiSckPin, hal::rfSpiMisoPin, hal::rfSpiMosiPin);
// FIXME: Look up the correct hal::rfCsPin!
auto csGpioPin = hal::GpioPin(hal::rfCsPin);


// --- Private function declarations ---

auto SendCommand(std::uint8_t * data,
                 std::size_t length,
                 std::uint8_t * responseData,
                 std::size_t responseLength) -> void;
auto WriteFifo(std::uint8_t * data, std::size_t length) -> void;


// --- Public function definitions ---

auto Initialize() -> void
{
    csGpioPin.Direction(hal::PinDirection::out);
    // TODO: Check if CS pin is active low
    csGpioPin.Set();

    // TODO: Use a reasonable baudrate
    constexpr auto baudrate = 1'000'000;
    spi.init(baudrate, /*slave=*/false, /*tiMode=*/false);

    // Here comes the configuration of the RF module
}


auto Morse() -> void
{
}


template<std::size_t size>
auto Send(std::span<std::uint8_t, size> data) -> void
{
}


// --- Private function definitions ---

auto SendCommand(std::uint8_t * data,
                 std::size_t length,
                 std::uint8_t * responseData,
                 std::size_t responseLength) -> void
{
}


auto WriteFifo(std::uint8_t * data, std::size_t length) -> void
{
}
}
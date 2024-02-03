#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <climits>
#include <span>


namespace sts1cobcsw::periphery::rf
{
using RODOS::AT;
using RODOS::MICROSECONDS;
using RODOS::MILLISECONDS;
using RODOS::NOW;

enum class PowerUpBootOptions : std::uint8_t
{
    noPatch = 0x01,
    patch = 0x81
};

enum class PowerUpXtalOptions : std::uint8_t
{
    xtal = 0x00,  // Reference signal is derived from the internal crystal oscillator
    txco = 0x01   // Reference signal is derived from an external TCXO
};

// --- Private globals ---

auto spi = RODOS::HAL_SPI(
    hal::rfSpiIndex, hal::rfSpiSckPin, hal::rfSpiMisoPin, hal::rfSpiMosiPin, hal::spiNssDummyPin);
auto csGpioPin = hal::GpioPin(hal::rfCsPin);
auto nirqGpioPin = hal::GpioPin(hal::rfNirqPin);
auto sdnGpioPin = hal::GpioPin(hal::rfSdnPin);
auto gpio0GpioPin = hal::GpioPin(hal::rfGpio0Pin);
auto gpio1GpioPin = hal::GpioPin(hal::rfGpio1Pin);
auto paEnablePin = hal::GpioPin(hal::rfPaEnablePin);

// TODO: This should probably be somewhere else as it is not directly related to the RF module
auto watchdogResetGpioPin = hal::GpioPin(hal::watchdogClearPin);

constexpr std::uint32_t powerUpXoFrequency = 26'000'000;  // 26 MHz

// Si4463 command headers
constexpr auto cmdPartInfo = 0x01_b;
constexpr auto cmdPowerUp = 0x02_b;
constexpr auto cmdReadCmdBuff = 0x44_b;

// Command response lengths
constexpr auto partInfoResponseLength = 8U;

// Check for this value when waiting for the Si4463 (WaitOnCts())
constexpr auto readyCtsByte = 0xFF_b;

// --- Private function declarations ---

auto InitializeGpioAndSpi() -> void;

auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void;

auto SendCommandNoResponse(std::span<Byte const> commandBuffer) -> void;

template<std::size_t nResponseBytes>
auto SendCommandWithResponse(std::span<Byte const> commandBuffer)
    -> std::array<Byte, nResponseBytes>;

auto WaitOnCts() -> void;

// --- Public function definitions ---

// TODO: Get rid of all the magic numbers
// TODO: Replace all C-style arrays with std::array

auto Initialize() -> void
{
    InitializeGpioAndSpi();

    PowerUp(PowerUpBootOptions::noPatch, PowerUpXtalOptions::xtal, powerUpXoFrequency);

    // Here comes the initialization of the RF module
}


auto ReadPartInfo() -> std::uint16_t
{
    auto const sendBuffer = std::to_array<Byte>({cmdPartInfo});
    auto responseBuffer = SendCommandWithResponse<partInfoResponseLength>(sendBuffer);

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(responseBuffer[1]) << CHAR_BIT
                                      | static_cast<std::uint16_t>(responseBuffer[2]));
}


// --- Private function definitions ---

auto InitializeGpioAndSpi() -> void
{
    csGpioPin.Direction(hal::PinDirection::out);
    csGpioPin.Set();

    nirqGpioPin.Direction(hal::PinDirection::in);

    sdnGpioPin.Direction(hal::PinDirection::out);
    sdnGpioPin.Set();

    gpio0GpioPin.Direction(hal::PinDirection::out);
    gpio0GpioPin.Reset();

    watchdogResetGpioPin.Direction(hal::PinDirection::out);
    watchdogResetGpioPin.Reset();
    AT(NOW() + 1 * MILLISECONDS);
    watchdogResetGpioPin.Set();
    AT(NOW() + 1 * MILLISECONDS);
    watchdogResetGpioPin.Reset();

    constexpr auto baudrate = 10'000'000;
    auto spiError = spi.init(baudrate, /*slave=*/false, /*tiMode=*/false);
    if(spiError == -1)
    {
        RODOS::PRINTF("Error initializing RF SPI!\n");
        // TODO: proper error handling
        return;
    }

    // Enable Si4463 and wait for PoR to finish
    AT(NOW() + 100 * MILLISECONDS);
    sdnGpioPin.Reset();
    AT(NOW() + 20 * MILLISECONDS);
}


auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void
{
    auto const powerUpBuffer = std::to_array<Byte>(
        {cmdPowerUp,
         static_cast<Byte>(bootOptions),
         static_cast<Byte>(xtalOptions),
         static_cast<Byte>(xoFrequency >> (CHAR_BIT * 3)),  // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency >> (CHAR_BIT * 2)),  // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency >> (CHAR_BIT)),      // NOLINT(hicpp-signed-bitwise)
         static_cast<Byte>(xoFrequency)});

    SendCommandNoResponse(powerUpBuffer);
}


auto SendCommandNoResponse(std::span<Byte const> commandBuffer) -> void
{
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    hal::WriteTo(&spi, commandBuffer);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();
    WaitOnCts();
    // No response -> just set the CS pin again
    csGpioPin.Set();
}


template<std::size_t nResponseBytes>
auto SendCommandWithResponse(std::span<Byte const> commandBuffer)
    -> std::array<Byte, nResponseBytes>
{
    csGpioPin.Reset();
    AT(NOW() + 20 * MICROSECONDS);
    hal::WriteTo(&spi, commandBuffer);
    AT(NOW() + 2 * MICROSECONDS);
    csGpioPin.Set();

    auto responseBuffer = std::array<Byte, nResponseBytes>{};
    WaitOnCts();
    // WaitOnCts leaves CS pin low, read response afterwards
    hal::ReadFrom(&spi, std::span<Byte, nResponseBytes>(responseBuffer));
    csGpioPin.Set();

    return responseBuffer;
}


//! @brief Polls the CTS byte until 0xFF is received (i.e. Si4463 is ready for command).
auto WaitOnCts() -> void
{
    auto sendBuffer = std::to_array<Byte>({cmdReadCmdBuff});
    do
    {
        AT(NOW() + 20 * MICROSECONDS);
        csGpioPin.Reset();
        AT(NOW() + 20 * MICROSECONDS);

        hal::WriteTo(&spi, std::span<Byte const, std::size(sendBuffer)>(sendBuffer));
        auto ctsBuffer = std::array<Byte, 1>{};
        hal::ReadFrom(&spi, std::span<Byte, std::size(ctsBuffer)>(ctsBuffer));

        if(ctsBuffer[0] != readyCtsByte)
        {
            AT(NOW() + 2 * MICROSECONDS);
            csGpioPin.Set();
        }
        else
        {
            break;
        }
    } while(true);
}
}

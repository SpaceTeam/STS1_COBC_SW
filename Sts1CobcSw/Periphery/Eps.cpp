#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>

#include <rodos_no_using_namespace.h>

namespace sts1cobcsw::periphery::eps
{
// --- Private globals ---

auto epsSpi = HAL_SPI(framEpsSpiIndex, framEpsSpiSckPin, framEpsSpiMisoPin, framEpsSpiMosiPin);

enum class AdcRegister : std::uint8_t
{
    conversion = 0b1 << 7,
    setup = 0b01 << 6,
    averaging = 0b001 << 5,
    reset = 0b0001 << 4
};

enum class ScanMode : std::uint8_t
{
    zeroThroughN = 0b00,
    nThroughHighest = 0b01,
    repeatN = 0b10,
    noScan = 0b11
};

// --- Private function declarations ---

// --- Public function definitions ---

// --- Private function definitions ---
}
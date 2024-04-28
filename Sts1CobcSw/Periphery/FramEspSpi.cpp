#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/FramEspSpi.hpp>


namespace sts1cobcsw
{
hal::Spi spi = hal::Spi(
    hal::framEpsSpiIndex, hal::framEpsSpiSckPin, hal::framEpsSpiMisoPin, hal::framEpsSpiMosiPin);
}
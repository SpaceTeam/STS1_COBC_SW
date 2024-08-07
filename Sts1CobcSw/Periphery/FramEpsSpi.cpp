#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/FramEpsSpi.hpp>


namespace sts1cobcsw
{
hal::Spi framEpsSpi = hal::Spi(
    hal::framEpsSpiIndex, hal::framEpsSpiSckPin, hal::framEpsSpiMisoPin, hal::framEpsSpiMosiPin);
}

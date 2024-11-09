#include <Sts1CobcSw/Hal/HardwareSpi.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/FramEpsSpi.hpp>


namespace sts1cobcsw
{
auto hardwareSpi = hal::HardwareSpi(
    hal::framEpsSpiIndex, hal::framEpsSpiSckPin, hal::framEpsSpiMisoPin, hal::framEpsSpiMosiPin);
hal::Spi & framEpsSpi = hardwareSpi;
}

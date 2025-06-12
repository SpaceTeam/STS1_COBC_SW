#include <Sts1CobcSw/Hal/HardwareSpi.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>  // IWYU pragma: associated


namespace sts1cobcsw
{
namespace
{
auto hardwareFlashSpi = hal::HardwareSpi(
    hal::flashSpiIndex, hal::flashSpiSckPin, hal::flashSpiMisoPin, hal::flashSpiMosiPin);
auto hardwareFramEpsSpi = hal::HardwareSpi(
    hal::framEpsSpiIndex, hal::framEpsSpiSckPin, hal::framEpsSpiMisoPin, hal::framEpsSpiMosiPin);
auto hardwareRfSpi =
    hal::HardwareSpi(hal::rfSpiIndex, hal::rfSpiSckPin, hal::rfSpiMisoPin, hal::rfSpiMosiPin);
}

hal::Spi & flashSpi = hardwareFlashSpi;
hal::Spi & framEpsSpi = hardwareFramEpsSpi;
hal::Spi & rfSpi = hardwareRfSpi;
}

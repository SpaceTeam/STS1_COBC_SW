#include <Sts1CobcSw/Hal/SpiMock.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>  // IWYU pragma: associated


namespace sts1cobcsw
{
namespace
{
auto flashSpiMock = hal::SpiMock{};
auto framEpsSpiMock = hal::SpiMock{};
auto rfSpiMock = hal::SpiMock{};
}


hal::Spi & flashSpi = flashSpiMock;
hal::Spi & framEpsSpi = framEpsSpiMock;
hal::Spi & rfSpi = rfSpiMock;
}

#include <Sts1CobcSw/Hal/Spi.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::hal
{
auto Initialize(RODOS::HAL_SPI * spi, std::uint32_t baudRate) -> void
{
    // spi.init() only returns -1 if the SPI_IDX is out of range. Since we can check that statically
    // we do not need to report that error at runtime.
    spi->init(baudRate, /*slave=*/false, /*tiMode=*/false);
}
}

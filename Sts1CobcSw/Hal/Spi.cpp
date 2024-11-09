#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::hal
{
Spi::Spi(RODOS::SPI_IDX spiIndex,
         RODOS::GPIO_PIN sckPin,
         RODOS::GPIO_PIN misoPin,
         RODOS::GPIO_PIN mosiPin)
    : spi_(spiIndex, sckPin, misoPin, mosiPin, spiNssDummyPin)
{
    transferEnd_.put(RODOS::END_OF_TIME);
}


auto Initialize(Spi * spi, std::uint32_t baudRate, bool useOpenDrainOutputs) -> void
{
    // spi.init() only returns -1 if the SPI_IDX is out of range. Since we can check that statically
    // we do not need to report that error at runtime.
    spi->spi_.init(baudRate, /*slave=*/false, /*tiMode=*/false, useOpenDrainOutputs);
    spi->transferEnd_.put(RODOS::END_OF_TIME);
}


auto Spi::TransferEnd() const -> std::int64_t
{
    std::int64_t transferEnd = 0;
    this->transferEnd_.get(transferEnd);
    return transferEnd;
}


auto Spi::BaudRate() -> std::int32_t
{
    return this->spi_.status(RODOS::SPI_STATUS_BAUDRATE);
}


auto Initialize(RODOS::HAL_SPI * spi, std::uint32_t baudRate) -> void
{
    // spi.init() only returns -1 if the SPI_IDX is out of range. Since we can check that statically
    // we do not need to report that error at runtime.
    spi->init(baudRate, /*slave=*/false, /*tiMode=*/false);
}
}

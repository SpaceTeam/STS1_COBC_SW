#include <Sts1CobcSw/Hal/SpiMock.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::hal
{
namespace empty
{
auto DoInitialize([[maybe_unused]] std::uint32_t baudRate) -> void
{
}


auto DoRead([[maybe_unused]] void * data,
            [[maybe_unused]] std::size_t nBytes,
            [[maybe_unused]] Duration timeout) -> void
{
}


auto DoWrite([[maybe_unused]] void const * data,
             [[maybe_unused]] std::size_t nBytes,
             [[maybe_unused]] Duration timeout) -> void
{
}


auto DoTransferEnd() -> RodosTime
{
    return endOfTime;
}


auto DoBaudRate() -> std::int32_t
{
    return 0;
}
}


Spi::Spi([[maybe_unused]] RODOS::SPI_IDX spiIndex,
         [[maybe_unused]] RODOS::GPIO_PIN sckPin,  // NOLINT(*easily-swappable-parameters)
         [[maybe_unused]] RODOS::GPIO_PIN misoPin,
         [[maybe_unused]] RODOS::GPIO_PIN mosiPin)
    : pimpl_(new Spi::Impl())
{
}


auto Initialize(Spi * spi, std::uint32_t baudRate) -> void
{
    return spi->pimpl_->doInitialize(baudRate);
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Spi::Read(void * data, std::size_t nBytes, Duration timeout) -> void
{
    return pimpl_->doRead(data, nBytes, timeout);
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Spi::Write(void const * data, std::size_t nBytes, Duration timeout) -> void
{
    return pimpl_->doWrite(data, nBytes, timeout);
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Spi::TransferEnd() const -> RodosTime
{
    return pimpl_->doTransferEnd();
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Spi::BaudRate() const -> std::int32_t
{
    return pimpl_->doBaudRate();
}
}

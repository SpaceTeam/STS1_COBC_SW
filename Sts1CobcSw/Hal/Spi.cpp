#include <Sts1CobcSw/Hal/Spi.hpp>

#include <strong_type/type.hpp>

#include <utility>


namespace sts1cobcsw::hal
{
auto Initialize(Spi * spi, std::uint32_t baudRate, bool useOpenDrainOutputs) -> void
{
    spi->DoInitialize(baudRate, useOpenDrainOutputs);
}


[[nodiscard]] auto Spi::TransferEnd() const -> RodosTime
{
    return DoTransferEnd();
}


[[nodiscard]] auto Spi::BaudRate() const -> std::int32_t
{
    return DoBaudRate();
}
}

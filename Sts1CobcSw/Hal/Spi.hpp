#pragma once


#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::hal
{
class Spi
{
public:
    Spi() = delete;
    Spi(RODOS::SPI_IDX spiIndex,
        RODOS::GPIO_PIN sckPin,
        RODOS::GPIO_PIN misoPin,
        RODOS::GPIO_PIN mosiPin);

    friend auto Initialize(Spi * spi, std::uint32_t baudRate, bool useOpenDrainOutputs) -> void;

    template<typename T, std::size_t extent>
    friend auto WriteTo(Spi * spi, std::span<T const, extent> data, Duration timeout) -> void;

    template<typename T, std::size_t extent>
    friend auto ReadFrom(Spi * spi, std::span<T, extent> data, Duration timeout) -> void;

    auto TransferEnd() const -> RodosTime;
    auto BaudRate() -> std::int32_t;


private:
    RODOS::HAL_SPI spi_;
    mutable RODOS::CommBuffer<std::int64_t> transferEnd_;
};


auto Initialize(Spi * spi, std::uint32_t baudRate, bool useOpenDrainOutputs = false) -> void;
}


#include <Sts1CobcSw/Hal/Spi.ipp>  // IWYU pragma: keep

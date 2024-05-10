#pragma once


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

    friend auto Initialize(Spi * spi, std::uint32_t baudRate) -> void;

    template<typename T, std::size_t extent>
    friend auto WriteTo(Spi * spi, std::span<T const, extent> data, std::int64_t timeout) -> void;

    template<typename T, std::size_t extent>
    friend auto ReadFrom(Spi * spi, std::span<T, extent> data, std::int64_t timeout) -> void;

    auto TransferEnd() const -> std::int64_t;
    auto BaudRate() -> std::int32_t;


private:
    RODOS::HAL_SPI spi_;
    mutable RODOS::CommBuffer<std::int64_t> transferEnd_;
};


auto Initialize(RODOS::HAL_SPI * spi, std::uint32_t baudRate) -> void;

// TODO: Maybe remove extent to reduce code bloat, or probably build time since it is just a single
// call to the Rodos function, which is trivial to inline.
template<typename T, std::size_t extent>
auto WriteTo(RODOS::HAL_SPI * spi, std::span<T const, extent> data) -> void;

template<typename T, std::size_t extent>
auto ReadFrom(RODOS::HAL_SPI * spi, std::span<T, extent> data) -> void;
}


#include <Sts1CobcSw/Hal/Spi.ipp>  // IWYU pragma: keep

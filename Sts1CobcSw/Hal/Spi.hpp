#pragma once


#include <rodos_no_using_namespace.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::hal
{
class Spi
{
private:
    RODOS::HAL_SPI spi;
    RODOS::CommBuffer<std::int64_t> transferEnd;

public:
    friend auto Initialize(Spi * spiClass, std::int32_t baudRate) -> void;

    template<typename T, std::size_t extent>
    friend auto ReadFrom(Spi * spiClass, std::span<T const, extent> data, std::int64_t timeout)
        -> void;

    template<typename T, std::size_t extent>
    friend auto WriteTo(Spi * spiClass, std::span<T, extent> data, std::int64_t timeout) -> void;

    auto TransferEnd();
};

}
// TODO: Maybe remove extent to reduce code bloat, or probably build time since it is just a single
// call to the Rodos function, which is trivial to inline.


#include <Sts1CobcSw/Hal/Spi.ipp>  // IWYU pragma: keep

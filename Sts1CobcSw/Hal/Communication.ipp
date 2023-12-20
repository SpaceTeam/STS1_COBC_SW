#pragma once


#include <Sts1CobcSw/Hal/Communication.hpp>


namespace sts1cobcsw::hal
{
template<typename T, std::size_t extent>
inline auto WriteTo(auto * communicationInterface, std::span<T const, extent> data) -> void
{
    std::size_t nSentBytes = 0U;
    auto bytes = std::as_bytes(data);
    while(nSentBytes < bytes.size())
    {
        nSentBytes +=
            communicationInterface->write(bytes.data() + nSentBytes, bytes.size() - nSentBytes);
    }
}


template<std::size_t extent>
inline auto ReadFrom(auto * communicationInterface, std::span<Byte, extent> data) -> void
{
    std::size_t nReadBytes = 0U;
    while(nReadBytes < data.size())
    {
        nReadBytes +=
            communicationInterface->read(data.data() + nReadBytes, data.size() - nReadBytes);
    }
}


template<std::size_t size>
inline auto ReadFrom(RODOS::HAL_SPI * spi) -> std::array<Byte, size>
{
    auto buffer = std::array<Byte, size>{};
    ReadFrom(spi, std::span(buffer));
    return buffer;
}
}
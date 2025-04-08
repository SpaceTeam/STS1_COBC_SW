#pragma once


#include <Sts1CobcSw/Fram/Fram.hpp>


namespace sts1cobcsw::fram
{
template<std::size_t extent>
inline auto WriteTo(Address address, std::span<Byte const, extent> data, Duration timeout) -> void
{
    internal::WriteTo(address, data.data(), data.size(), timeout);
}


template<std::size_t extent>
inline auto ReadFrom(Address address, std::span<Byte, extent> data, Duration timeout) -> void
{
    internal::ReadFrom(address, data.data(), data.size(), timeout);
}


template<std::size_t size>
auto ReadFrom(Address address, Duration timeout) -> std::array<Byte, size>
{
    auto data = std::array<Byte, size>{};
    internal::ReadFrom(address, data.data(), data.size(), timeout);
    return data;
}
}

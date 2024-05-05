#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>


namespace sts1cobcsw::fram
{
template<std::size_t extent>
inline auto WriteTo(Address address, std::span<Byte const, extent> data, std::int64_t timeout)
    -> void
{
    internal::WriteTo(address, data.data(), data.size(), timeout);
}


template<std::size_t extent>
inline auto ReadFrom(Address address, std::span<Byte, extent> data, std::int64_t timeout) -> void
{
    internal::ReadFrom(address, data.data(), data.size(), timeout);
}


template<std::size_t size>
auto ReadFrom(Address address, std::int64_t timeout) -> std::array<Byte, size>
{
    auto data = std::array<Byte, size>{};
    internal::ReadFrom(address, data.data(), data.size(), timeout);
    return data;
}
}

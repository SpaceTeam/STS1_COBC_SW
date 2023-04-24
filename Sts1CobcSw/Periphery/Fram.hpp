#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::periphery::fram
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
using DeviceId = std::array<serial::Byte, 9>;
// TODO: Use a strong typedef or create class that ensures, that the address is only 20 bits long
using Address = std::uint32_t;


[[nodiscard]] auto Initialize() -> std::int32_t;
[[nodiscard]] auto ReadDeviceId() -> DeviceId;

// TODO: Rename to ReadFrom() and WriteTo()
auto Read(Address address, void * data, std::size_t size) -> void;
auto Write(Address address, void const * data, std::size_t size) -> void;

template<typename T>
[[nodiscard]] auto Read(Address address) -> T;
template<typename T>
auto Write(Address address, T const & t) -> void;


template<typename T>
auto Read(Address address) -> T
{
    auto t = T{};
    Read(address, &t, sizeof(T));
    return t;
}


template<typename T>
auto Write(Address address, T const & t) -> void
{
    Write(address, &t, sizeof(T));
}
}

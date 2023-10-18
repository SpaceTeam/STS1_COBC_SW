#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::fram
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
using DeviceId = std::array<Byte, 9>;
// TODO: Use a strong typedef or create class that ensures, that the address is only 20 bits long
using Address = std::uint32_t;


[[nodiscard]] auto Initialize() -> std::int32_t;
[[nodiscard]] auto ReadDeviceId() -> DeviceId;

// TODO: Rename to ReadFrom() and WriteTo()
auto ReadFrom(Address address, void * data, std::size_t size) -> void;
auto WriteTo(Address address, void const * data, std::size_t size) -> void;

template<typename T>
[[nodiscard]] auto Read(Address address) -> T;
template<typename T>
auto WriteTo(Address address, T const & t) -> void;
}


#include <Sts1CobcSw/Periphery/Fram.tpp>

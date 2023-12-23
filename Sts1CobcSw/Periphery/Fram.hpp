#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::fram
{
// NOLINTNEXTLINE(*magic-numbers)
using DeviceId = std::array<Byte, 9>;
// TODO: Use a strong typedef or create class that ensures, that the address is only 20 bits long
using Address = std::uint32_t;


auto Initialize() -> void;
[[nodiscard]] auto ReadDeviceId() -> DeviceId;

template<std::size_t extent>
auto WriteTo(Address address, std::span<Byte const, extent> data) -> void;

template<std::size_t extent>
auto ReadFrom(Address address, std::span<Byte, extent> data) -> void;

template<std::size_t size>
[[nodiscard]] auto ReadFrom(Address address) -> std::array<Byte, size>;


// Contents of namespace internal is only for internal use and not part of the public interface. The
// declarations here are necessary because of templates.
namespace internal
{
auto WriteTo(Address address, void const * data, std::size_t nBytes) -> void;
auto ReadFrom(Address address, void * data, std::size_t nBytes) -> void;
}
}


#include <Sts1CobcSw/Periphery/Fram.ipp>

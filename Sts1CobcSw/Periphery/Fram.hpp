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
// TODO: Use a strong typedef
using Address = std::uint32_t;
using Size = std::uint32_t;


inline constexpr Size memorySize = 1024 * 1024;
inline constexpr auto correctDeviceId =
    DeviceId{0x03_b, 0x2E_b, 0xC2_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b};


auto Initialize() -> void;
[[nodiscard]] auto ReadDeviceId() -> DeviceId;
auto ActualBaudRate() -> std::int32_t;

template<std::size_t extent>
auto WriteTo(Address address, std::span<Byte const, extent> data, std::int64_t timeout) -> void;

template<std::size_t extent>
auto ReadFrom(Address address, std::span<Byte, extent> data, std::int64_t timeout) -> void;

template<std::size_t size>
[[nodiscard]] auto ReadFrom(Address address, std::int64_t timeout) -> std::array<Byte, size>;


// Contents of namespace internal is only for internal use and not part of the public interface. The
// declarations here are necessary because of templates.
namespace internal
{
auto WriteTo(Address address, void const * data, std::size_t nBytes, std::int64_t timeout) -> void;
auto ReadFrom(Address address, void * data, std::size_t nBytes, std::int64_t timeout) -> void;
}
}


#include <Sts1CobcSw/Periphery/Fram.ipp>  // IWYU pragma: keep

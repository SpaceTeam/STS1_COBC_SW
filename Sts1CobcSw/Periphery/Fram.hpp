#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

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
[[nodiscard]] auto ReadDeviceId(std::int64_t timeout = RODOS::END_OF_TIME) -> DeviceId;
auto ActualBaudRate() -> int32_t;

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
auto WriteTo(Address address,
             void const * data,
             std::size_t nBytes,
             std::int64_t timeout = RODOS::END_OF_TIME) -> void;
auto ReadFrom(Address address,
              void * data,
              std::size_t nBytes,
              std::int64_t timeout = RODOS::END_OF_TIME) -> void;
}
}


#include <Sts1CobcSw/Periphery/Fram.ipp>  // IWYU pragma: keep

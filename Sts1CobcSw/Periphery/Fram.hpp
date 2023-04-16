#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::periphery::fram
{
using serial::Byte;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
using DeviceId = std::array<Byte, 9>;


[[nodiscard]] auto Initialize() -> std::int32_t;
[[nodiscard]] auto ReadDeviceId() -> DeviceId;
template<std::size_t nBytes>
[[nodiscard]] auto Read(std::uint32_t address) -> std::array<Byte, nBytes>;


// This namespace contains implementation details not meant for the user
namespace details
{
auto Read(std::uint32_t address, std::span<Byte> data) -> void;
}


// TODO: Maybe I don't want such a function at this abstraction level. Maybe details::Read() is
// fine and a "type safe" read function should only be implemented for the persistent state.
template<std::size_t nBytes>
inline auto Read(std::uint32_t address) -> std::array<Byte, nBytes>
{
    auto data = std::array<Byte, nBytes>{};
    details::Read(address, data);
    return data;
}
}

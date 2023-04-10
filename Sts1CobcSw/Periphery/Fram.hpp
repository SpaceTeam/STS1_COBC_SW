#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstdint>


namespace sts1cobcsw::periphery::fram
{
using serial::Byte;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
using DeviceId = std::array<Byte, 9>;


[[nodiscard]] auto Initialize() -> std::int32_t;
[[nodiscard]] auto ReadDeviceId() -> DeviceId;
}

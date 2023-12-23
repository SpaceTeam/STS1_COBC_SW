#pragma once
#include <cstdint>
#include <span>

namespace sts1cobcsw::periphery
{
auto EnableHardwareCrc() -> void;
auto HardwareCrc32(std::span<uint32_t> data) -> uint32_t;
}  // namespace sts1cobcsw::periphery

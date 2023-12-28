#pragma once
#include <cstdint>
#include <span>

namespace sts1cobcsw::periphery
{
auto EnableHardwareCrc() -> void;
auto EnableHardwareCrcAndDma() -> void;
auto DmaCrc32(std::span<uint32_t> data) -> void;
auto HardwareCrc32(std::span<uint32_t> data) -> uint32_t;
}  // namespace sts1cobcsw::periphery

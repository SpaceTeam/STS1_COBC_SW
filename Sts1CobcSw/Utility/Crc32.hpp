#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>
#include <span>


namespace sts1cobcsw::utility
{
auto InitializeCrc32Hardware() -> void;
[[nodiscard]] auto ComputeCrc32(std::span<Byte const> data) -> std::uint32_t;
[[nodiscard]] auto ComputeCrc32Blocking(std::span<std::uint32_t const> data) -> std::uint32_t;
[[nodiscard]] auto ComputeCrc32Sw(std::span<Byte const> data) -> std::uint32_t;
}

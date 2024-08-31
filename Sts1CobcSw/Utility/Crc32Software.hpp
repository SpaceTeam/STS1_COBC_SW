#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>
#include <span>


namespace sts1cobcsw::utility
{
[[nodiscard]] auto ComputeCrc32Sw(std::span<Byte const> data) -> std::uint32_t;
}

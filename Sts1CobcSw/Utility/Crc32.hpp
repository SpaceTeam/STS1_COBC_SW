#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>
#include <span>


namespace sts1cobcsw::utility
{
enum class DmaBurstType
{
    singleWord,
    fourBytes
};

[[nodiscard]] auto ComputeCrc32Sw(std::span<Byte const> data) -> std::uint32_t;
[[nodiscard]] auto ComputeCrc32(std::span<Byte const> data) -> std::uint32_t;
[[nodiscard]] auto ComputeCrc32Blocking(std::span<std::uint32_t> data) -> std::uint32_t;
auto InitializeCrc32Hardware(DmaBurstType dmaBurstType) -> void;
}

#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::fram
{
constexpr auto framIdSize = 9U;


auto Initialize() -> void;
auto Reset() -> void;
[[nodiscard]] auto ReadId() -> std::array<Byte, framIdSize>;
auto Write(std::uint32_t address, std::span<Byte const> data) -> void;
auto Read(std::uint32_t address, std::span<Byte> data) -> void;
template<std::size_t size>
auto Read(std::uint32_t address) -> std::array<Byte, size>;


template<std::size_t size>
auto Read(std::uint32_t address) -> std::array<Byte, size>
{
    auto data = std::array<Byte, size>{};
    Read(address, std::span(data));
    return data;
}
}

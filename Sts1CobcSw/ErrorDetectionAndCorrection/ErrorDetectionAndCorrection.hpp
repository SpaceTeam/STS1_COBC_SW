#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>
#include <span>


namespace sts1cobcsw
{
[[nodiscard]] auto ComputeCrc32(std::span<Byte const> data) -> std::uint32_t;
[[nodiscard]] auto ComputeCrc32(std::uint32_t previousCrc32, std::span<Byte const> data)
    -> std::uint32_t;


// I am too lazy to add an .ipp file just for this function
template<std::size_t size>
[[nodiscard]] constexpr auto ComputeBitwiseMajorityVote(std::span<Byte const, size> data0,
                                                        std::span<Byte const, size> data1,
                                                        std::span<Byte const, size> data2)
    -> std::array<Byte, size>
{
    auto result = std::array<Byte, size>{};
    for(auto i = 0U; i < size; ++i)
    {
        result[i] = (data0[i] & data1[i]) | (data0[i] & data2[i]) | (data1[i] & data2[i]);
    }
    return result;
}
}

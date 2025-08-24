#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>
#include <optional>
#include <span>


namespace sts1cobcsw
{
[[nodiscard]] auto ComputeCrc32(std::span<Byte const> data) -> std::uint32_t;
[[nodiscard]] auto ComputeCrc32(std::uint32_t previousCrc32, std::span<Byte const> data)
    -> std::uint32_t;


// I am too lazy to add an .ipp file just for this function
template<typename T>
[[nodiscard]] constexpr auto ComputeMajorityVote(T const & value0,
                                                 T const & value1,
                                                 T const & value2) -> std::optional<T>
{
    if(value0 == value1 or value0 == value2)
    {
        return value0;
    }
    if(value1 == value2)
    {
        return value1;
    }
    return std::nullopt;
}
}

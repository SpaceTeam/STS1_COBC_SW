#pragma once


#include <optional>


namespace sts1cobcsw
{
// I won't make a separate .ipp file just for this simple function
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

#pragma once


#include <Sts1CobcSw/Utility/StringLiteral.hpp>


namespace sts1cobcsw
{
template<std::size_t size>
constexpr StringLiteral<size>::StringLiteral(char const (&string)[size])
{
    std::copy_n(&string[0], size, value.begin());
}


template<std::size_t leftSize, std::size_t rightSize>
constexpr auto operator==(StringLiteral<leftSize> const & lhs, StringLiteral<rightSize> const & rhs)
    -> bool
{
    if constexpr(leftSize != rightSize)
    {
        return false;
    }
    else
    {
        return lhs.value == rhs.value;
    }
}
}

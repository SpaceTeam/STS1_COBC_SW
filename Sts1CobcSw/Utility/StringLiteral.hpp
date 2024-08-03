#pragma once


#include <algorithm>
#include <array>
#include <cstddef>


namespace sts1cobcsw
{
// Utility class for passing string literals as non-type template parameters
template<std::size_t size>
struct StringLiteral
{
    // NOLINTNEXTLINE(*explicit*): implicit conversion is for convenient usage (see test)
    constexpr StringLiteral(char const (&string)[size]);

    template<std::size_t leftSize, std::size_t rightSize>
    friend constexpr auto operator==(StringLiteral<leftSize> const & lhs,
                                     StringLiteral<rightSize> const & rhs) -> bool;

    std::array<char, size> value;
};
}


#include <Sts1CobcSw/Utility/StringLiteral.ipp>  // IWYU pragma: keep

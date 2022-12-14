#pragma once


#include <cstddef>


namespace sts1cobcsw::serial
{
// Allegedly std::byte is quite heavyweight. This type alias allows us to easily replace std::byte
// with std::uint8_t or whatever type we want to use for low-level memory access and some such.
using Byte = std::byte;


inline constexpr auto operator"" _b(unsigned long long number)  // NOLINT(google-runtime-int)
    -> Byte
{
    return Byte(number);
}
}
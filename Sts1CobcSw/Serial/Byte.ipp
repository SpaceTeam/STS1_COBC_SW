#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

namespace sts1cobcsw::serial
{
inline constexpr auto operator"" _b(unsigned long long number)  // NOLINT(google-runtime-int)
    -> Byte
{
    return static_cast<Byte>(number);
}
}
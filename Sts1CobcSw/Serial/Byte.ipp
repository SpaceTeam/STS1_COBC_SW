#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>


namespace sts1cobcsw
{
inline constexpr auto operator"" _b(unsigned long long number) -> Byte
{
    return static_cast<Byte>(number);
}
}

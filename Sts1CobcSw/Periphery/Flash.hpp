#pragma once


#include <cstdint>


namespace sts1cobcsw::periphery::flash
{
// TODO: Proper error handling/return type
[[nodiscard]] auto Initialize() -> std::int32_t;
auto Initialize() -> std::int32_t;
}
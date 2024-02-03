#pragma once


#include <cstdint>


namespace sts1cobcsw::periphery::rf
{
auto Initialize() -> void;
auto ReadPartInfo() -> std::uint16_t;
}
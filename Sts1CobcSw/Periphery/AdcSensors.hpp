#pragma once


#include <cstdint>


namespace sts1cobcsw::adc
{
auto Initialize() -> void;
auto ReadRfTemperature() -> std::uint16_t;
auto ReadMcuTemperature() -> std::uint16_t;
}

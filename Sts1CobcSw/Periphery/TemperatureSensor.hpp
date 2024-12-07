#pragma once

#include <cstdint>

auto InitRfTemperature() -> void;
auto ReadRfTemperature() -> std::uint16_t;

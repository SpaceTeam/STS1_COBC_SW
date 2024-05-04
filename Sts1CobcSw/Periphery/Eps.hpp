#pragma once

#include <array>
#include <cstdint>


namespace sts1cobcsw::eps
{
inline constexpr auto nAdcs = 3U;
inline constexpr auto nChannels = 16U;
using AdcValue = std::uint16_t;
using SensorValues = std::array<AdcValue, nChannels * nAdcs>;

auto Initialize() -> void;
auto Read() -> SensorValues;
auto ResetAdcRegisters() -> void;
auto ClearFifos() -> void;
}

#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>


namespace sts1cobcsw::eps
{
constexpr auto nChannels = 16U;
constexpr auto adcDataLength = 2 * nChannels;
constexpr auto nAdcs = 3U;
constexpr auto sensorDataLength = nAdcs * adcDataLength;

auto Initialize() -> void;
auto Read() -> std::array<Byte, sensorDataLength>;
auto ResetAdcRegisters() -> void;
auto ClearFifos() -> void;
}

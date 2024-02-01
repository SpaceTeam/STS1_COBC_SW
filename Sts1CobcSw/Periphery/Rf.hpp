#pragma once

#include <Sts1CobcSw/Periphery/RfNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string_view>


namespace sts1cobcsw::periphery::rf
{
auto Initialize() -> void;
auto InitializeGpioAndSpi() -> void;
auto GetPartInfo() -> std::uint16_t;
}
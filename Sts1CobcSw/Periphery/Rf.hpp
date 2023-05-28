#pragma once

#include <Sts1CobcSw/Periphery/RfNames.hpp>

#include <cstdint>

namespace sts1cobcsw::periphery::rf
{
// Must be called once in a thread's init() function
auto Initialize() -> void;

auto InitializeGpioAndSpi() -> void;

auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void;

auto PartInfoIsCorrect() -> bool;

auto GetPartInfo() -> std::uint16_t;

auto Morse() -> void;

auto ClearInterrupts() -> void;
}
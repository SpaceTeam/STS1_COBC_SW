#pragma once

#include <Sts1CobcSw/Periphery/RfNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>
#include <span>

namespace sts1cobcsw::periphery::rf
{

using sts1cobcsw::serial::Byte;

// Must be called once in a thread's init() function
auto Initialize() -> void;

auto InitializeGpioAndSpi() -> void;

auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void;

auto PartInfoIsCorrect() -> bool;

auto GetPartInfo() -> std::uint16_t;

auto Morse() -> void;

auto SendPacket() -> void;

auto StartTx(std::uint16_t length) -> void;

auto EnterPowerMode(PowerMode powerMode) -> void;

auto TransmitData(std::span<Byte> data) -> void;

auto ClearInterrupts() -> void;
}
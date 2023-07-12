#pragma once

#include <Sts1CobcSw/Periphery/RfNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>
#include <span>

namespace sts1cobcsw::periphery::rf
{

using sts1cobcsw::serial::Byte;

// Must be called once in a thread's init() function
auto Initialize(TxType txType) -> void;

auto InitializeGpioAndSpi() -> void;

auto PowerUp(PowerUpBootOptions bootOptions,
             PowerUpXtalOptions xtalOptions,
             std::uint32_t xoFrequency) -> void;

auto PartInfoIsCorrect() -> bool;

auto GetPartInfo() -> std::uint16_t;

auto Morse() -> void;

auto StartTx(std::uint16_t length) -> void;

auto EnterPowerMode(PowerMode powerMode) -> void;

auto TransmitTestData() -> void;

auto TransmitData(std::uint8_t * data, std::size_t length) -> void;

// TODO: This is a test implementation, receives 2 * 48 bytes
auto ReceiveTestData() -> std::array<std::uint8_t, maxRxBytes>;

auto ClearInterrupts() -> void;

auto ClearFifos() -> void;

auto SetTxType(TxType txType) -> void;
}
#pragma once
#include <Sts1CobcSw/Serial/Byte.hpp>

namespace sts1cobcsw::periphery::rf
{
using sts1cobcsw::serial::operator""_b;
// Si4463 Command IDs
inline constexpr auto cmdPowerUp = 0x02_b;
inline constexpr auto cmdNop = 0x00_b;
inline constexpr auto cmdPartInfo = 0x01_b;
inline constexpr auto cmdFuncInfo = 0x10_b;
inline constexpr auto cmdSetProperty = 0x11_b;
inline constexpr auto cmdReadyCmdBuff = 0x44_b;

// Response to READY_CMD_BUFF if ready for command
inline constexpr auto readyCtsByte = 0xFF_b;
}
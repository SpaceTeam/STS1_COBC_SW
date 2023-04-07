#pragma once
#include <Sts1CobcSw/Serial/Byte.hpp>

namespace sts1cobcsw::periphery::rf
{
using sts1cobcsw::serial::operator""_b;

// In the following, abbreviations are used to adhere to the API documentation

// Si4463 Command IDs
inline constexpr auto cmdPowerUp = 0x02_b;
inline constexpr auto cmdNop = 0x00_b;
inline constexpr auto cmdPartInfo = 0x01_b;
inline constexpr auto cmdFuncInfo = 0x10_b;
inline constexpr auto cmdSetProperty = 0x11_b;
inline constexpr auto cmdReadyCmdBuff = 0x44_b;

// Response to READY_CMD_BUFF if ready for command
inline constexpr auto readyCtsByte = 0xFF_b;

// PowerUp constants
inline constexpr auto noPatch = 0x01_b;
inline constexpr auto noTxco = 0x00_b;

// Property groups
inline constexpr auto groupGlobal = 0x00_b;       // Global
inline constexpr auto groupIntCtl = 0x01_b;       // Interrupt control
inline constexpr auto groupFrrCtl = 0x02_b;       // Fast response register control
inline constexpr auto groupPreamble = 0x10_b;     // Preamble
inline constexpr auto groupSync = 0x11_b;         // Sync word
inline constexpr auto groupPkt = 0x12_b;          // Packet
inline constexpr auto groupModem = 0x20_b;        //
inline constexpr auto groupModemChflt = 0x21_b;   //
inline constexpr auto groupPa = 0x22_b;           //
inline constexpr auto groupSynth = 0x23_b;        //
inline constexpr auto groupMatch = 0x30_b;        //
inline constexpr auto groupFreqControl = 0x40_b;  //
inline constexpr auto groupRxHop = 0x50_b;        //
inline constexpr auto groupPti = 0xF0_b;          //

inline constexpr auto setPropertyHeaderSize = 4;  // SetProperty starts with Cmd ID, Group, # of properties, Start property 
}
#pragma once
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>

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
inline constexpr auto cmdGetIntStatus = 0x20_b;
inline constexpr auto cmdReadyCmdBuff = 0x44_b;

// GetIntStatus constants
inline constexpr auto getIntStatusResponseLength = 8;
inline constexpr auto partInfoResponseLength = 9;

// Response to READY_CMD_BUFF if ready for command
inline constexpr auto readyCtsByte = 0xFF_b;

// Property groups
enum class PropertyGroup : std::uint8_t
{
    groupGlobal = 0x00,       // Global
    groupIntCtl = 0x01,       // Interrupt control
    groupFrrCtl = 0x02,       // Fast response register control
    groupPreamble = 0x10,     // Preamble
    groupSync = 0x11,         // Sync word
    groupPkt = 0x12,          // Packet
    groupModem = 0x20,        //
    groupModemChflt = 0x21,   //
    groupPa = 0x22,           //
    groupSynth = 0x23,        //
    groupMatch = 0x30,        //
    groupFreqControl = 0x40,  //
    groupRxHop = 0x50,        //
    groupPti = 0xF0           //
};

// SetProperty command constants
// SetProperty starts with Cmd ID, Group, # of properties, Start property
inline constexpr auto setPropertyHeaderSize = 4;
inline constexpr auto maxNProperties = 12;

// PowerUp command constants
enum class PowerUpBootOptions : std::uint8_t
{
    noPatch = 0x01,
    patch = 0x81
};
enum class PowerUpXtalOptions : std::uint8_t
{
    xtal = 0x00,  // Reference signal is derived from the internal crystal oscillator
    txco = 0x01   // Reference signal is derived from an external TCXO
};
// inline constexpr auto noPatch = 0x01_b;
// inline constexpr auto noTxco = 0x00_b;
}
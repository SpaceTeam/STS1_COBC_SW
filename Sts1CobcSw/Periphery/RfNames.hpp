#pragma once
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>

namespace sts1cobcsw::periphery::rf
{
using sts1cobcsw::serial::operator""_b;

// In the following, abbreviations are used to adhere to the API documentation

enum class TxType
{
    morse,  // From GPIO pin
    packet  // From FIFO
};

// TODO: How will transmitting large amounts of data work?
inline constexpr auto maxRxBytes = 128;

// Si4463 Command IDs
// TODO: change to enum (class)?
inline constexpr auto cmdNop = 0x00_b;
inline constexpr auto cmdPartInfo = 0x01_b;
inline constexpr auto cmdPowerUp = 0x02_b;
inline constexpr auto cmdFuncInfo = 0x10_b;
inline constexpr auto cmdSetProperty = 0x11_b;
inline constexpr auto cmdFifoInfo = 0x15_b;
inline constexpr auto cmdGetIntStatus = 0x20_b;
inline constexpr auto cmdStartTx = 0x31_b;
inline constexpr auto cmdChangeState = 0x34_b;
inline constexpr auto cmdReadCmdBuff = 0x44_b;

// GetIntStatus constants
inline constexpr auto getIntStatusResponseLength = 8;

inline constexpr auto partInfoResponseLength = 8;

// Response to READY_CMD_BUFF if ready for command
inline constexpr auto readyCtsByte = 0xFF_b;

// Property groups
enum class PropertyGroup : std::uint8_t
{
    global = 0x00,       // Global
    intCtl = 0x01,       // Interrupt control
    frrCtl = 0x02,       // Fast response register control
    preamble = 0x10,     // Preamble
    sync = 0x11,         // Sync word
    pkt = 0x12,          // Packet
    modem = 0x20,        //
    modemChflt = 0x21,   //
    pa = 0x22,           //
    synth = 0x23,        //
    match = 0x30,        //
    freqControl = 0x40,  //
    rxHop = 0x50,        //
    pti = 0xF0           //
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

enum class PowerMode : std::uint8_t
{
    standby = 0x01
};
}
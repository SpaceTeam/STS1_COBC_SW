#pragma once


#include <Sts1CobcSw/ChannelCoding/External/ConvolutionalCoding.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::rf
{
enum class TxType : std::uint8_t
{
    morse,  // From GPIO pin
    packet  // From FIFO
};


enum class PropertyGroup : std::uint8_t
{
    global = 0x00,       //
    intCtl = 0x01,       // Interrupt control
    frrCtl = 0x02,       // Fast response register control
    preamble = 0x10,     //
    sync = 0x11,         // Sync word
    pkt = 0x12,          // Packet
    modem = 0x20,        // Selects type of modulation. TX: also selects source of modulation.
    modemChflt = 0x21,   // Filter coefficients
    pa = 0x22,           // Power amplifier
    synth = 0x23,        //
    match = 0x30,        //
    freqControl = 0x40,  //
    rxHop = 0x50,        //
    pti = 0xF0           // Packet trace interface
};


inline constexpr auto correctPartNumber = 0x4463;
inline constexpr auto maxTxDataLength = cc::ViterbiCodec::UnencodedSize((1U << 13U) - 1U, true);


auto Initialize(TxType txType) -> Result<void>;
auto EnableTx() -> void;
auto DisableTx() -> void;
auto ReadPartNumber() -> std::uint16_t;
auto EnterStandbyMode() -> void;
auto SetTxType(TxType txType) -> void;
auto SetTxDataLength(std::uint16_t length) -> void;
auto SetTxDataRate(std::uint32_t dataRate) -> void;
auto SetRxDataRate(std::uint32_t dataRate) -> void;
[[nodiscard]] auto GetTxDataRate() -> std::uint32_t;
[[nodiscard]] auto GetRxDataRate() -> std::uint32_t;
auto SendAndWait(std::span<Byte const> data) -> void;
auto SendAndContinue(std::span<Byte const> data) -> void;
auto SuspendUntilDataSent(Duration timeout) -> void;
// Return the number of received bytes
auto Receive(std::span<Byte> data, Duration timeout) -> std::size_t;
}

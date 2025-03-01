#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstdint>
#include <span>


namespace sts1cobcsw::rf
{
enum class TxType
{
    morse,  // From GPIO pin
    packet  // From FIFO
};


inline constexpr auto correctPartNumber = 0x4463;
inline constexpr auto maxRxBytes = 128;

extern bool rfIsWorking;


auto Initialize(TxType txType) -> void;
auto ReadPartNumber() -> std::uint16_t;
auto ReadPartInfo() -> std::array<Byte, 8>;
auto ReadFunctionInfo() -> std::array<Byte, 6>;
auto ReadModemStatus() -> std::array<Byte, 8>;
auto ReadDeviceState() -> std::array<Byte, 2>;
auto SetTxType(TxType txType) -> void;
auto Send(void const * data, std::size_t nBytes) -> void;
auto ReceiveTestData() -> std::array<Byte, maxRxBytes>;
}

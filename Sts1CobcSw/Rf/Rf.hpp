#pragma once


#include <cstdint>


namespace sts1cobcsw::rf
{
enum class TxType
{
    morse,  // From GPIO pin
    packet  // From FIFO
};


inline constexpr auto correctPartNumber = 0x4463;


auto Initialize(TxType txType) -> void;
auto ReadPartNumber() -> std::uint16_t;
auto SetTxType(TxType txType) -> void;
}

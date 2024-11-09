#pragma once


#include <Sts1CobcSw/Hal/Spi.hpp>

#include <cstdint>


namespace sts1cobcsw::rf
{
enum class TxType
{
    morse,  // From GPIO pin
    packet  // From FIFO
};


inline constexpr auto correctPartNumber = 0x4463;

extern hal::Spi & spi;
extern bool rfIsWorking;


auto Initialize(TxType txType) -> void;
auto ReadPartNumber() -> std::uint16_t;
auto SetTxType(TxType txType) -> void;
}

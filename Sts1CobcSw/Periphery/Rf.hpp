#pragma once


#include <cstdint>


namespace sts1cobcsw::rf
{
enum class TxType
{
    morse,  // From GPIO pin
    packet  // From FIFO
};


auto Initialize(TxType txType) -> void;
auto ReadPartNumber() -> std::uint16_t;
}
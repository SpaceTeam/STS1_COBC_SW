#pragma once


#include <cstdint>


namespace sts1cobcsw::periphery::rf
{
enum class TxType
{
    morse,  // From GPIO pin
    packet  // From FIFO
};


auto Initialize(TxType txType) -> void;
auto ReadPartInfo() -> std::uint16_t;
}
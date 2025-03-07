#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstdint>


namespace sts1cobcsw::rf
{
enum class TxType
{
    morse,  // From GPIO pin
    packet  // From FIFO
};


inline constexpr auto correctPartNumber = 0x4463;
inline constexpr auto maxRxSize = 1024;


auto Initialize(TxType txType) -> void;
auto ReadPartNumber() -> std::uint16_t;

auto SetTxType(TxType txType) -> void;
// TODO: Return a Result<void, E> instead
auto Send(void const * data, std::uint16_t size) -> bool;
// TODO: Replace this by Receive(void * data, std::size_t size) -> void;
auto ReceiveTestData() -> std::array<Byte, maxRxSize>;
}

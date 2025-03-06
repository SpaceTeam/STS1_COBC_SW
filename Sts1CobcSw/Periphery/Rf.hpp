#pragma once


#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Spis.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

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
inline constexpr auto maxRxBytes = 1024;

// TODO: I am pretty sure that we should use persistentVariables.template
// Get/Store<"flashIsWorking">(true) instead
extern bool rfIsWorking;


auto Initialize(TxType txType) -> void;
auto ReadPartNumber() -> std::uint16_t;

auto SetTxType(TxType txType) -> void;
// TODO: Return type should be void;
auto Send(void const * data, std::size_t nBytes) -> bool;
// TODO: Replace this by Receive(void * data, std::size_t nBytes) -> void;
auto ReceiveTestData() -> std::array<Byte, maxRxBytes>;
}

#pragma once


#include <Sts1CobcSw/Hal/Spi.hpp>
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
inline constexpr auto maxRxBytes = 128;

extern hal::Spi spi;
extern bool rfIsWorking;


auto Initialize(TxType txType) -> void;
auto ReadPartNumber() -> std::uint16_t;
auto ReadPartInfo() -> std::array<Byte, 8>;
auto ReadFunctionInfo() -> std::array<Byte, 6>;
auto ReadDeviceState() -> std::array<Byte, 2>;
auto SetTxType(TxType txType) -> void;
auto Send(void const * data, std::size_t nBytes) -> void;
auto RecieveTestData() -> std::array<Byte, maxRxBytes>;
}

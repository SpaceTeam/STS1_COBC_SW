#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
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


enum class ErrorCode
{
    timeout = 1
};


template<typename T>
using Result = outcome_v2::experimental::status_result<T, ErrorCode, RebootPolicy>;


inline constexpr auto correctPartNumber = 0x4463;
inline constexpr auto maxRxSize = 1024;


auto Initialize(TxType txType) -> void;
auto ReadPartNumber() -> std::uint16_t;

auto SetTxType(TxType txType) -> void;
auto Send(void const * data, std::uint16_t size) -> Result<void>;
// TODO: Replace this by Receive(void * data, std::size_t size) -> void;
auto ReceiveTestData() -> Result<std::array<Byte, maxRxSize>>;
}

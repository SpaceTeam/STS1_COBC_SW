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
inline constexpr auto correctPartNumber = 0x4463;
inline constexpr auto maxTxDataLength = cc::ViterbiCodec::UnencodedSize((1U << 13U) - 1U, true);


auto Initialize() -> Result<void>;
auto EnableTx() -> void;
auto DisableTx() -> void;
auto ReadPartNumber() -> std::uint16_t;
auto EnterStandbyMode() -> void;
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

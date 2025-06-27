#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <cstdint>
#include <span>


namespace sts1cobcsw::rf
{
enum class TxType : std::uint8_t
{
    morse,  // From GPIO pin
    packet  // From FIFO
};


inline constexpr auto correctPartNumber = 0x4463;
inline constexpr auto maxTxDataLength = (1U << 13U) - 1U;


auto Initialize(TxType txType) -> Result<void>;
auto EnableTx() -> void;
auto DisableTx() -> void;
auto ReadPartNumber() -> std::uint16_t;
auto EnterStandbyMode() -> void;
auto SetTxType(TxType txType) -> void;
auto SetTxDataLength(std::uint16_t length) -> void;
auto SetTxDataRate(std::uint32_t dataRate) -> void;
auto SetRxDataRate(std::uint32_t dataRate) -> void;
[[nodiscard]] auto GetTxDataRate() -> std::uint32_t;
[[nodiscard]] auto GetRxDataRate() -> std::uint32_t;
auto SendAndWait(std::span<Byte const> data) -> void;
auto SendAndContinue(std::span<Byte const> data) -> void;
auto SuspendUntilDataSent(Duration timeout) -> void;
auto Receive(std::span<Byte> data, Duration timeout) -> void;
}

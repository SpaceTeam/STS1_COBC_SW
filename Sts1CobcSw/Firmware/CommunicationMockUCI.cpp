#include "DynamicCommunicationManager.hpp"
#include <Sts1CobcSw/Rf/Rf.hpp>

#include <alogrithm>

namespace sts1cobcsw::rf
{

auto Initialize() -> Result<void>
{
   return uci::Initialize();
}

auto EnableTx() -> void
{
    uci::EnableTx();
}

auto DisableTx() -> void
{
    uci::DisableTx();
}

auto ReadPartNumber() -> std::uint16_t
{
    return uci::ReadPartNumber();
}

auto EnterStandbyMode() -> void
{
    uci::EnterStandbyMode();
}

auto SetTxDataLength(std::uint16_t length) -> void
{
    uci::SetTxDataRate(dataRate);
}

auto SetTxDataRate(std::uint32_t dataRate) -> void
{
    uci::SetTxDataRate(dataRate);
}

auto SetRxDataRate(std::uint32_t dataRate) -> void
{
    uci::SetRxDataRate(dataRate);
}

auto GetTxDataRate() -> std::uint32_t
{
    return uci::GetTxDataRate();
}

auto GetRxDataRate() -> std::uint32_t
{
    return uci::GetRxDataRate();
}

auto SendAndWait(std::span<Byte const> data) -> void
{
    uci::SendAndWait(data);
}

auto SendAndContinue(std::span<Byte const> data) -> void
{
    uci::SendAndContinue(data);
}

auto SuspendUntilDataSent(Duration timeout) -> void
{
    uci::SuspendUntilDataSent(timeout);
}

auto Receive(std::span<Byte> data, Duration timeout) -> std::size_t
{
    return uci::Receive(data, pollSlice);
}

}
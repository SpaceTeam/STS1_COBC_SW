#include "DynamicCommunicationManager.hpp"
#include <Sts1CobcSw/Rf/Rf.hpp>

#include <alogrithm>

namespace sts1cobcsw::rf
{

auto Initialize() -> Result<void>
{
   return rf::Initialize();
}

auto EnableTx() -> void
{
    rf::EnableTx();
}

auto DisableTx() -> void
{
    rf::DisableTx();
}

auto ReadPartNumber() -> std::uint16_t
{
    return rf::ReadPartNumber();
}

auto EnterStandbyMode() -> void
{
    rf::EnterStandbyMode();
}

auto SetTxDataLength(std::uint16_t length) -> void
{
    rf::SetTxDataRate(dataRate);
}

auto SetTxDataRate(std::uint32_t dataRate) -> void
{
    rf::SetTxDataRate(dataRate);
}

auto SetRxDataRate(std::uint32_t dataRate) -> void
{
    rf::SetRxDataRate(dataRate);
}

auto GetTxDataRate() -> std::uint32_t
{
    return rf::GetTxDataRate();
}

auto GetRxDataRate() -> std::uint32_t
{
    return rf::GetRxDataRate();
}

auto SendAndWait(std::span<Byte const> data) -> void
{
    rf::SendAndWait(data);
}

auto SendAndContinue(std::span<Byte const> data) -> void
{
    rf::SendAndContinue(data);
}

auto SuspendUntilDataSent(Duration timeout) -> void
{
    rf::SuspendUntilDataSent(timeout);
}

auto Receive(std::span<Byte> data, Duration timeout) -> std::size_t
{
    return rf::Receive(data, pollSlice);
}

}
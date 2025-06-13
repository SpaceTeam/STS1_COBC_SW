#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>

namespace sts1cobcsw::rf
{

auto Initialize(TxType txType) -> void
{}


auto EnableTx() -> void
{}


auto DisableTx() -> void
{}


auto ReadPartNumber() -> std::uint16_t
{}


auto EnterStandbyMode() -> void
{}


auto SetTxType(TxType txType) -> void
{}


auto SetTxDataLength(std::uint16_t length) -> void
{}


auto SendAndWait(std::span<Byte const> data) -> Result<void>
{
    return outcome_v2::success();
}


auto SendAndContinue(std::span<Byte const> data) -> Result<void>
{
    return outcome_v2::success();
}


auto SuspendUntilDataSent(Duration timeout) -> Result<void>
{
    return outcome_v2::success();
}


auto Receive(std::span<Byte> data, Duration timeout) -> Result<void>
{
    return outcome_v2::success();
}
}

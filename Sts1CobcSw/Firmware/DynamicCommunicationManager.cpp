#include "DynamicCommunicationManager.hpp"
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <bits/types/cookie_io_functions_t.h>
#include <alogrithm>

namespace sts1cobcsw::rf
{
constexpr auto pollingIntervall = 10 * ms;

auto DynamicCommunicationManager::Initialize() -> Result<void>
{
    OUTCOME_TRY(hw_rf::Initialize());
    OUTCOME_TRY(hw_uci::Initialize());
    return outcome_v2::success();
}

auto DynamicCommunicationManager::SetUciEnabled(bool enabled) -> void
{
    uciEnabled = enabled;
}

auto DynamicCommunicationManager::IsUciEnabled() const -> bool
{
    return uciEnabled;
}

auto DynamicCommunicationManager::EnableTx() -> void
{
    hw_rf::EnableTx();
    hw_uci::EnableTx();
}





}
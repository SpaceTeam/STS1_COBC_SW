#include "DynamicCommunicationManager.hpp"
#include <Sts1CobcSw/Rf/Rf.hpp>

#include <alogrithm>

namespace sts1cobcsw::rf
{

auto DynamicCommunicationManager::Initialize() -> Result<void>
{
    OUTCOME_TRY(hw_rf::Initialize());
    OUTCOME_TRY(hw_uci::Initialize());
    return outcome_v2::success();
}

auto DynamicCommunicationManager::EnableTx() -> void
{
    hw_rf::EnableTx();
    hw_uci::EnableTx();
}

auto DynamicCommunicationManager::DisableTx() -> void
{
    hw_rf::DisableTx();
    hw_uci::DisableTx();
}

auto ReadPartNumber() -> std::uint16_t
{
    return hw_rf::ReadPartNumber();
}

auto EnterStandbyMode() -> void
{
    hw_rf::EnterStandbyMode();
    // TODO: check with flo if this should always happen for uci, even when deactiveated or not
    hw_uci::EnterStandbyMode();
}

auto SetTxDataLength(std::uint16_t length) -> void
{
    hw_rf::SetTxDataRate(dataRate);
    hw_uci::SetTxDataRate(dataRate);
}

auto DynamicCommunicationManager::SetTxDataRate(std::uint32_t dataRate) -> void
{
    hw_rf::SetTxDataRate(dataRate);
    hw_uci::SetTxDataRate(dataRate);
}

auto DynamicCommunicationManager::SetRxDataRate(std::uint32_t dataRate) -> void
{
    hw_rf::SetRxDataRate(dataRate);
    hw_uci::SetRxDataRate(dataRate);
}

auto GetTxDataRate() -> std::uint32_t
{
    return hw_rf::GetTxDataRate();
}

auto GetRxDataRate() -> std::uint32_t
{
    return hw_rf::GetRxDataRate();
}

auto DynamicCommunicationManager::SendAndWait(std::span<Byte const> data) -> void
{
    hw_rf::SendAndWait(data);
    if (uciEnabled) {
        hw_uci::SendAndWait(data);
    }
}

auto DynamicCommunicationManager::SendAndContinue(std::span<Byte const> data) -> void
{
    hw_rf::SendAndContinue(data);
    if (uciEnabled) {
        hw_uci::SendAndContinue(data);
    }
}

auto SuspendUntilDataSent(Duration timeout) -> void
{
    // TODO: implement Suspend function ...
    // TODO: think of a solution .... big problem i guess
    
    // TODO: does that work as expected since uci aint waiting, is this fine or not ask flo?!?
    hw_rf::SuspendUntilDataSent(timeout);
}

auto DynamicCommunicationManager::Receive(std::span<Byte> data, Duration timeout) -> std::size_t
{
    auto deadline = CurrentRodosTime() + timeout;
    
    currentLock = LockedMedium::none;
    if (rfReservedWindows > 0) {
        currentLock = LockedMedium::rf;
        rfReservedWindows--;
    }
    
    while (CurrentRodosTime() < deadline) {
        auto timeRemaining = deadline - CurrentRodosTime();
        if (timeRemaining <= 0) {
            break;
        }
        
        if (currentLock == LockedMedium::none) {
            int activeMediums = uciEnabled ? 2 : 1;
            // TODO: check about the slicing to listen to both devices
            auto pollSlice = std::min(pollingInterval, timeRemaining) / activeMediums;
        
            auto bytesRf = hw_rf::Receive(data, pollSlice);
            if (bytesRf > 0) {
                rfReservedWindows = 3;
                return bytesRf;
            }
            
            if (uciEnabled) {
                auto bytesUci = hw_uci::Receive(data, pollSlice);
                if (bytesUci > 0) {
                    return bytesUci;
                }
            }
        }
        else if (currentLock == LockedMedium::rf) {
            auto bytesRf = hw_rf::Receive(data, timeRemaining); 
            if (bytesRf > 0) {
                rfReservedWindows = 3;
                return bytesRf;
            }
        }      
    }      
    return 0;
}

// TODO: change to system parameter via cmake variable
auto DynamicCommunicationManager::SetUciEnabled(bool enabled) -> void
{
    uciEnabled = enabled;
}

// TODO: change to system parameter via cmake variable
auto DynamicCommunicationManager::IsUciEnabled() const -> bool
{
    return uciEnabled;
}
}
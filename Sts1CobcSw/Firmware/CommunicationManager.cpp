#include "DynamicCommunicationManager.hpp"
#include <Sts1CobcSw/Rf/Rf.hpp>

#include <alogrithm>

namespace sts1cobcsw::rf
{
    
namespace
{
    // --- Private globals ---
    // TODO: change to system parameter via cmake variable
    bool uciEnabled = true;
    
    LockedMedium currentLock = LockedMedium::none;
    int rfReservedWindows = 0;
    constexpr Duration pollingInterval = 10 * ms;
}


auto Initialize() -> Result<void>
{
    OUTCOME_TRY(rf::Initialize());
    OUTCOME_TRY(uci::Initialize());
    return outcome_v2::success();
}

auto EnableTx() -> void
{
    rf::EnableTx();
    uci::EnableTx();
}

auto DisableTx() -> void
{
    rf::DisableTx();
    uci::DisableTx();
}

auto ReadPartNumber() -> std::uint16_t
{
    return rf::ReadPartNumber();
}

auto EnterStandbyMode() -> void
{
    rf::EnterStandbyMode();
    // TODO: check if that works as expected
    uci::EnterStandbyMode();
}

auto SetTxDataLength(std::uint16_t length) -> void
{
    rf::SetTxDataRate(dataRate);
    uci::SetTxDataRate(dataRate);
}

auto SetTxDataRate(std::uint32_t dataRate) -> void
{
    rf::SetTxDataRate(dataRate);
    uci::SetTxDataRate(dataRate);
}

auto SetRxDataRate(std::uint32_t dataRate) -> void
{
    rf::SetRxDataRate(dataRate);
    uci::SetRxDataRate(dataRate);
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
    if (uciEnabled) {
        uci::SendAndWait(data);
    }
}

auto SendAndContinue(std::span<Byte const> data) -> void
{
    rf::SendAndContinue(data);
    if (uciEnabled) {
        uci::SendAndContinue(data);
    }
}

auto SuspendUntilDataSent(Duration timeout) -> void
{
    // TODO: does that work as expected since uci aint waiting, is this fine or not?!?
    rf::SuspendUntilDataSent(timeout);
}

auto Receive(std::span<Byte> data, Duration timeout) -> std::size_t
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
        
            auto bytesRf = rf::Receive(data, pollSlice);
            if (bytesRf > 0) {
                rfReservedWindows = 3;
                return bytesRf;
            }
            
            if (uciEnabled) {
                auto bytesUci = uci::Receive(data, pollSlice);
                if (bytesUci > 0) {
                    return bytesUci;
                }
            }
        }
        else if (currentLock == LockedMedium::rf) {
            auto bytesRf = rf::Receive(data, timeRemaining); 
            if (bytesRf > 0) {
                rfReservedWindows = 3;
                return bytesRf;
            }
        }      
    }      
    return 0;
}

// TODO: change to system parameter
auto SetUciEnabled(bool enabled) -> void
{
    uciEnabled = enabled;
}

// TODO: change to system parameter
auto IsUciEnabled() -> bool
{
    return uciEnabled;
}
}
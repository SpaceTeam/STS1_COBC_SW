#pragma once

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace sts1cobcsw::rf
{

enum class LockedMedium
{
    none,
    rf,
    uci
};

class DynamicCommunicationManager
{
public:
    DynamicCommunicationManager() = default;

    [[nodiscard]] auto Initialize() -> Result<void>;
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
    auto Receive(std::span<Byte> data, Duration timeout) -> std::size_t;
    
    auto SetUciEnabled(bool enabled) -> void;
    [[nodiscard]] auto IsUciEnabled() const -> bool;

private:
    // TODO: change to system parameter via cmake variable
    static inline bool uciEnabled = true;
    
    static inline LockedMedium currentLock = LockedMedium::none;
    static inline int rfReservedWindows = 0;
    static constexpr Duration pollingInterval = 10 * ms;
};

}

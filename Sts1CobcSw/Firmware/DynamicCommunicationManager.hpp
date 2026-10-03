#pragma once

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <cstddef>
#include <cstdint>
#include <random>
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

    auto SetUciEnabled(bool enabled) -> void;
    [[nodiscard]] auto IsUciEnabled() const -> bool;

    auto EnableTx() -> void;
    auto DisableTx() -> void;

    auto SetTxDataRate(std::uint32_t dataRate) -> void;
    auto SetRxDataRate(std::uint32_t dataRate) -> void;

    auto SendAndWait(std::span<Byte const> data) -> void;
    auto SendAndContinue(std::span<Byte const> data) -> void;

    auto Receive(std::span<Byte> data, Duration timeout) -> std::size_t;

private:
    bool uciEnabled = true;
    LockedMedium currentLock = LockedMedium::none;
    int rfReservedWindows = 0;
};

}

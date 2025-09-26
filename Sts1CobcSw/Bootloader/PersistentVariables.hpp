#pragma once

#include <Sts1CobcSw/Bootloader/Fram.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>

#include <cstdint>


namespace sts1cobcsw
{
template<typename T>
struct PersistentVariable
{
    using ValueType = T;
    std::uint32_t offset;
};


constexpr auto nTotalResets = PersistentVariable<std::uint32_t>{0};
constexpr auto nResetsSinceRf = PersistentVariable<std::uint8_t>{4};
constexpr auto activeSecondaryFwPartitionId = PersistentVariable<PartitionId>{5};
constexpr auto backupSecondaryFwPartitionId = PersistentVariable<PartitionId>{6};


template<typename T>
[[nodiscard]] auto Load(PersistentVariable<T> variable) -> T;
template<typename T>
auto Store(PersistentVariable<T> variable, T value) -> void;
}


#include <Sts1CobcSw/Bootloader/PersistentVariables.ipp>  // IWYU pragma: export

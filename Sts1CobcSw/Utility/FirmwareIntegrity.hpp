#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <cstdint>


// TODO: Move this to a new folder FirmwareManagement and add ProgramFlash()
namespace sts1cobcsw
{
inline constexpr std::uintptr_t primaryPartitionStartAddress = 0x0802'0000U;
inline constexpr std::uintptr_t secondaryPartition1StartAddress = 0x0804'0000U;
inline constexpr std::uintptr_t secondaryPartition2StartAddress = 0x0806'0000U;


[[nodiscard]] auto CheckFirmwareIntegrity(std::uintptr_t startAddress) -> Result<void>;
}

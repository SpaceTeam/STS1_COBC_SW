#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>

#include <strong_type/type.hpp>

#include <cstdint>
#include <span>


namespace sts1cobcsw::fw
{
struct Partition
{
    std::uintptr_t startAddress = 0;
    std::uint16_t flashSector = 0;
};


using DestinationPartition = strong::type<Partition, struct DestinationPartitionTag>;
using SourcePartition = strong::type<Partition, struct SourcePartitionTag>;


struct FirmwareChecksums
{
    std::uint32_t computed = 0;
    std::uint32_t stored = 0;
};


#ifdef BUILD_BOOTLOADER
using EraseResult = bool;
using ProgramResult = std::uintptr_t;
#else
using EraseResult = Result<void>;
using ProgramResult = Result<std::uintptr_t>;
#endif


inline constexpr auto partitionSize = 128 * 1024U;

extern Partition const primaryPartition;
extern Partition const secondaryPartition1;
extern Partition const secondaryPartition2;


[[nodiscard]] auto GetPartition(PartitionId partitionId) -> Partition;

#ifndef BUILD_BOOTLOADER
[[nodiscard]] auto CheckFirmwareIntegrity(std::uintptr_t startAddress) -> Result<void>;
#endif
auto ComputeAndReadFirmwareChecksums(std::uintptr_t startAddress, ErrorCode * errorCode)
    -> FirmwareChecksums;

[[nodiscard]] auto Erase(std::uint16_t flashSector) -> EraseResult;
[[nodiscard]] auto Program(std::uintptr_t address, std::span<Byte const> data) -> ProgramResult;
#ifdef BUILD_BOOTLOADER
[[nodiscard]] auto Overwrite(DestinationPartition destination, SourcePartition source) -> bool;
#endif
auto Read(std::uintptr_t address, std::span<Byte> data) -> void;
}

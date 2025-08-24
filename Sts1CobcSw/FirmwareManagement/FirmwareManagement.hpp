#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>

#include <cstdint>
#include <span>


namespace sts1cobcsw::fw
{
struct Partition
{
    std::uintptr_t startAddress = 0;
    std::uint16_t flashSector = 0;
};

struct Crcs
{
    bool validLength = true;
    bool aligned = true;
    std::uint32_t newCheckSum = 0;
    std::uint32_t oldCheckSum = 0;
};

extern Partition const primaryPartition;
extern Partition const secondaryPartition1;
extern Partition const secondaryPartition2;


[[nodiscard]] auto GetPartition(PartitionId partitionId) -> Result<Partition>;
auto GetCrcs(std::uintptr_t startAddress) -> Crcs;
[[nodiscard]] auto CheckFirmwareIntegrity(std::uintptr_t startAddress) -> Result<void>;
[[nodiscard]] auto Erase(std::uint16_t flashSector) -> Result<void>;
[[nodiscard]] auto Program(std::uintptr_t address, std::span<Byte const> data)
    -> Result<std::uintptr_t>;
auto Read(std::uintptr_t address, std::span<Byte> data) -> void;
}

#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <cstdint>
#include <span>


namespace sts1cobcsw::flash
{
[[maybe_unused]] constexpr std::size_t pageSize = 256;                 // bytes
[[maybe_unused]] constexpr std::size_t sectorSize = 4 * 1024;          // bytes
[[maybe_unused]] constexpr std::size_t smallBlockSize = 32 * 1024;     // bytes
[[maybe_unused]] constexpr std::size_t largeBlockSize = 64 * 1024;     // bytes
[[maybe_unused]] constexpr std::size_t flashSize = 128 * 1024 * 1024;  // bytes
[[maybe_unused]] constexpr std::size_t nSectors = flashSize / sectorSize;
[[maybe_unused]] constexpr std::size_t nSmallBlocks = flashSize / smallBlockSize;
[[maybe_unused]] constexpr std::size_t nLargeBlocks = flashSize / largeBlockSize;


using Page = std::array<Byte, pageSize>;
using PageSpan = std::span<Byte const, pageSize>;


struct JedecId
{
    std::uint8_t manufacturerId = 0U;
    std::uint16_t deviceId = 0U;
};


// TODO: Proper error handling/return type
auto Initialize() -> void;
[[nodiscard]] auto ReadJedecId() -> JedecId;
[[nodiscard]] auto ReadStatusRegister(std::int8_t registerNo) -> Byte;

[[nodiscard]] auto ReadPage(std::uint32_t address) -> Page;
auto ProgramPage(std::uint32_t address, PageSpan data) -> void;
auto EraseSector(std::uint32_t address) -> void;
auto WaitWhileBusy() -> void;
}

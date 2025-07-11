//! @file
//! @brief  Driver for the flash memory W25Q01JV.

#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
namespace flash
{
struct JedecId
{
    std::uint8_t manufacturerId = 0;
    std::uint16_t deviceId = 0;
};


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


inline constexpr auto correctJedecId = JedecId{.manufacturerId = 0xEF, .deviceId = 0x4021};


// TODO: Proper error handling/return type
auto Initialize() -> void;
[[nodiscard]] auto ReadJedecId() -> JedecId;
[[nodiscard]] auto ReadStatusRegister(std::int8_t registerNo) -> Byte;

[[nodiscard]] auto ReadPage(std::uint32_t address) -> Page;
auto ProgramPage(std::uint32_t address, PageSpan data) -> void;
auto EraseSector(std::uint32_t address) -> void;
[[nodiscard]] auto WaitWhileBusy(Duration timeout) -> Result<void>;
auto ActualBaudRate() -> std::int32_t;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, JedecId * jedecId) -> void const *;
}


template<>
inline constexpr std::size_t serialSize<flash::JedecId> =
    totalSerialSize<decltype(flash::JedecId::manufacturerId), decltype(flash::JedecId::deviceId)>;
}

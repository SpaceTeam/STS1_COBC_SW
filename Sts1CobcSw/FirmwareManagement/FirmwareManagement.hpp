#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#ifndef __linux__
    #include <rodos/src/bare-metal/stm32f4/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_flash.h>
#endif

#include <cstdint>
#include <span>


namespace sts1cobcsw::fw
{
struct Partition
{
    std::uintptr_t startAddress = 0;
    std::uint16_t flashSector = 0;
};


extern const Partition primaryPartition;
extern const Partition secondaryPartition1;
extern const Partition secondaryPartition2;


[[nodiscard]] auto CheckFirmwareIntegrity(std::uintptr_t startAddress) -> Result<void>;
[[nodiscard]] auto Erase(std::uint16_t flashSector) -> Result<void>;
[[nodiscard]] auto Program(std::uintptr_t address, std::span<Byte const> data)
    -> Result<std::uintptr_t>;
auto Read(std::uintptr_t address, std::span<Byte> data) -> void;
}

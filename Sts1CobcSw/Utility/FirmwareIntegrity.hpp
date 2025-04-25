#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>
#include <span>


namespace sts1cobcsw::utility
{
enum class Partition : std::uintptr_t
{
    primary = 0x8020000ULL,
    secondary1 = 0x8040000ULL,
    secondary2 = 0x8060000ULL
};


[[nodiscard]] auto CheckFirmwareIntegrity(Partition partition) -> Result<void>;
}

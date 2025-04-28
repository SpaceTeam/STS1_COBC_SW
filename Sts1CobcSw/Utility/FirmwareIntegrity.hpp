#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <cstdint>


namespace sts1cobcsw::utility
{
enum class Partition : std::uintptr_t
{
    primary = 0x802'0000ULL,
    secondary1 = 0x804'0000ULL,
    secondary2 = 0x806'0000ULL
};


[[nodiscard]] auto CheckFirmwareIntegrity(Partition partition) -> Result<void>;
}

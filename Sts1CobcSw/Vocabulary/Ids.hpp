#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <strong_type/equality.hpp>
#include <strong_type/invocable.hpp>
#include <strong_type/type.hpp>

#include <cstdint>


namespace sts1cobcsw
{
using ProgramId = strong::type<std::uint16_t,
                               struct ProgramIdTag,
                               strong::default_constructible,
                               strong::invocable,
                               strong::equality>;


enum class PartitionId : std::uint8_t
{
    primary = 0b0000'1111,
    secondary1 = 0b0000'0000,
    secondary2 = 0b1111'1111
};


[[nodiscard]] auto ToCZString(PartitionId partitionId) -> char const *;
[[nodiscard]] auto ToClosestSecondaryPartitionId(SerialBuffer<PartitionId> id) -> Byte;
}

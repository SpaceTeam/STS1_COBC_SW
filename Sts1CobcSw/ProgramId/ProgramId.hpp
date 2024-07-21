#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>

#include <strong_type/strong_type.hpp>

#include <cstdint>


namespace sts1cobcsw
{
using ProgramId =
    strong::type<std::uint16_t, struct ProgramIdTag, strong::invocable, strong::equality>;


template<>
inline constexpr std::size_t serialSize<ProgramId> =
    totalSerialSize<strong::underlying_type_t<ProgramId>>;


template<std::endian endianness>
[[nodiscard]] inline auto SerializeTo(void * destination, ProgramId const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] inline auto DeserializeFrom(void const * source, ProgramId * data) -> void const *;
}


#include <Sts1CobcSw/ProgramId/ProgramId.ipp>

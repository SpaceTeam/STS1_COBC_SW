#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>

#include <NamedType/named_type.hpp>

#include <cstdint>


namespace sts1cobcsw
{
using ProgramId = fluent::NamedType<std::uint16_t, struct ProgramIdTag, fluent::Callable>;

// TODO: Maybe make ProgramId completely serializable, i.e., overload (De-)Serialize() as well
template<>
inline constexpr std::size_t serialSize<ProgramId> = totalSerialSize<ProgramId::UnderlyingType>;
}

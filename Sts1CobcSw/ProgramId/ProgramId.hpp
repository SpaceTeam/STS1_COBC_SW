#pragma once


#include <NamedType/named_type.hpp>

#include <cstdint>


namespace sts1cobcsw
{
using ProgramId = fluent::NamedType<std::uint16_t, struct ProgramIdTag, fluent::Callable>;
}

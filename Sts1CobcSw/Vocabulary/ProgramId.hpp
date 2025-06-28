#pragma once


#include <strong_type/strong_type.hpp>

#include <cstdint>


namespace sts1cobcsw
{
using ProgramId = strong::type<std::uint16_t,
                               struct ProgramIdTag,
                               strong::default_constructible,
                               strong::invocable,
                               strong::equality>;
}

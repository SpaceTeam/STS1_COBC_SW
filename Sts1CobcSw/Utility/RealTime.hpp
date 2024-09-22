#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
[[nodiscard]] auto CurrentRealTime() -> RealTime;
[[nodiscard]] auto ToRodosTime(RealTime realTime) -> RodosTime;
[[nodiscard]] auto ToRealTime(RodosTime rodosTime) -> RealTime;
}


#include <Sts1CobcSw/Utility/RealTime.ipp>  // IWYU pragma: keep

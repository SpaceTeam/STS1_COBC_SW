#pragma once


#include <Sts1CobcSw/Vocabulary/TimeTypes.hpp>


namespace sts1cobcsw
{
[[nodiscard]] auto CurrentRealTime() -> RealTime;
[[nodiscard]] auto ToRodosTime(RealTime realTime) -> RodosTime;
[[nodiscard]] auto ToRealTime(RodosTime rodosTime) -> RealTime;
}


#include <Sts1CobcSw/Utility/RealTime.ipp>  // IWYU pragma: keep

#pragma once


#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <cinttypes>


#define DEBUG_PRINT_REAL_TIME() DEBUG_PRINT("Real time: %" PRIi32 "\n", value_of(CurrentRealTime()))


namespace sts1cobcsw
{
[[nodiscard]] auto CurrentRealTime() -> RealTime;
[[nodiscard]] auto ToRodosTime(RealTime realTime) -> RodosTime;
[[nodiscard]] auto ToRealTime(RodosTime rodosTime) -> RealTime;
}


#include <Sts1CobcSw/Time/RealTime.ipp>  // IWYU pragma: keep

#pragma once


#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>


namespace sts1cobcsw
{
// TODO: Measure how long the startup tests really take to determine the correct timeouts
inline constexpr auto startupTestTimeout = 100 * ms;
inline constexpr auto totalStartupTestTimeout = 3 * startupTestTimeout + 50 * ms;
auto ResumeSpiStartupTestAndSupervisorThread() -> void;
}

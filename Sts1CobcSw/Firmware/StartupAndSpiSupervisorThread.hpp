#pragma once


#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <utility>


namespace sts1cobcsw
{
// TODO: Measure how long the startup tests really take to determine the correct timeouts
inline constexpr auto startupTestTimeout = 200 * ms;
inline constexpr auto totalStartupTestTimeout = 3 * startupTestTimeout + 50 * ms;
auto ResumeStartupAndSpiSupervisorThread() -> void;
}

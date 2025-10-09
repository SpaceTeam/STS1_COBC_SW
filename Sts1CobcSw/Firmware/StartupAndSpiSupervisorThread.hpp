#pragma once


#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <utility>


namespace sts1cobcsw
{
// TODO: Measure how long the startup tests really take to determine the correct timeouts
inline constexpr auto flashStartupTestTimeout = 50 * ms;
inline constexpr auto framEpsStartupTestTimeout = 50 * ms;
inline constexpr auto rfStartupTestTimeout = 250 * ms;
inline constexpr auto totalStartupTestTimeout =
    flashStartupTestTimeout + framEpsStartupTestTimeout + rfStartupTestTimeout + 50 * ms;
auto ResumeStartupAndSpiSupervisorThread() -> void;
}

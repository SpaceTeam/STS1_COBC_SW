#pragma once


#include <Sts1CobcSw/Periphery/Section.hpp>


namespace sts1cobcsw::fram
{
// clang-format off
inline constexpr auto persistentVariablesSize = 100;
inline constexpr auto persistentVariables0 =    FirstSection<persistentVariablesSize>();
inline constexpr auto persistentVariables1 =
    NextSection<persistentVariablesSize>(persistentVariables0);
inline constexpr auto persistentVariables2 =
    NextSection<persistentVariablesSize>(persistentVariables1);
inline constexpr auto eduProgramQueue =         NextSection<20 * 10>(persistentVariables2);
inline constexpr auto eduProgramStatusHistory = NextSection<50 * 7>(eduProgramQueue);
inline constexpr auto testMemory =              NextSection<1000>(eduProgramStatusHistory);
inline constexpr auto telemetry =               LastSection(testMemory);
// clang-format on
}

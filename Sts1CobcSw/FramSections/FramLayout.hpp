#pragma once


#include <Sts1CobcSw/FramSections/Section.hpp>


namespace sts1cobcsw::fram
{
// clang-format off
inline constexpr auto persistentVariablesSize = Size(100);
inline constexpr auto persistentVariables0 =    FirstSection<persistentVariablesSize>();
inline constexpr auto persistentVariables1 =
    NextSection<persistentVariablesSize>(persistentVariables0);
inline constexpr auto persistentVariables2 =
    NextSection<persistentVariablesSize>(persistentVariables1);
inline constexpr auto eduProgramQueue =         NextSection<Size(20 * 10)>(persistentVariables2);
inline constexpr auto eduProgramStatusHistory = NextSection<Size(50 * 7)>(eduProgramQueue);
inline constexpr auto testMemory =              NextSection<Size(1000)>(eduProgramStatusHistory);
inline constexpr auto telemetry =               LastSection(testMemory);
// clang-format on

// Convenient type aliases for easy access to static members of Section<> (begin, end, size)
using PersistentVariables0 = decltype(persistentVariables0);
using PersistentVariables1 = decltype(persistentVariables1);
using PersistentVariables2 = decltype(persistentVariables2);
using EduProgramQueue = decltype(eduProgramQueue);
using EduProgramStatusHistory = decltype(eduProgramStatusHistory);
using TestMemory = decltype(testMemory);
using Telemetry = decltype(telemetry);
}

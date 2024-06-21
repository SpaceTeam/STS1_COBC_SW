#pragma once


#include <Sts1CobcSw/Periphery/Section.hpp>


namespace sts1cobcsw::fram
{
inline constexpr auto persistentVariablesSize = 100;
inline constexpr auto persistentVariables0 = CreateFirstSection<persistentVariablesSize>();
inline constexpr auto persistentVariables1 =
    CreateNextSection<persistentVariablesSize>(persistentVariables0);
inline constexpr auto persistentVariables2 =
    CreateNextSection<persistentVariablesSize>(persistentVariables1);
inline constexpr auto eduProgramQueue = CreateNextSection<20 * 10>(persistentVariables2);
inline constexpr auto eduProgramStatusHistory = CreateNextSection<50 * 7>(eduProgramQueue);
inline constexpr auto testMemory = CreateNextSection<1000>(eduProgramStatusHistory);
inline constexpr auto telemetry = CreateLastSection(testMemory);
}

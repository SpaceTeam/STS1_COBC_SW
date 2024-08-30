#pragma once


#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>


namespace sts1cobcsw
{
inline constexpr auto framMemory = Section<fram::Address(0), fram::memorySize>{};
inline constexpr auto persistentVariablesSize = fram::Size(100);
inline constexpr auto framSections =
    Subsections<framMemory,
                SubsectionInfo<"persistentVariables0", persistentVariablesSize>,
                SubsectionInfo<"persistentVariables1", persistentVariablesSize>,
                SubsectionInfo<"persistentVariables2", persistentVariablesSize>,
                SubsectionInfo<"eduProgramQueue", fram::Size(20 * 8)>,
                SubsectionInfo<"eduProgramStatusHistory", fram::Size(50 * 7)>,
                SubsectionInfo<"testMemory", fram::Size(1000)>,
                SubsectionInfo<"telemetry", fram::Size(26'168 * 40)>>{};
inline constexpr auto persistentVariables =
    PersistentVariables<framSections.Get<"persistentVariables0">(),
                        framSections.Get<"persistentVariables1">(),
                        framSections.Get<"persistentVariables2">(),
                        PersistentVariableInfo<"nResetsSinceRf", std::uint16_t>,
                        PersistentVariableInfo<"activeSecondaryFwPartition", std::int8_t>,
                        PersistentVariableInfo<"backupSecondaryFwPartition", std::int8_t>,
                        PersistentVariableInfo<"txIsOn", bool>,
                        PersistentVariableInfo<"antennasShouldBeDeployed", bool>,
                        PersistentVariableInfo<"nTotalResets", std::uint32_t>,
                        PersistentVariableInfo<"realTime", RealTime>,
                        PersistentVariableInfo<"realTimeOffset", Duration>,
                        PersistentVariableInfo<"epsIsWorking", bool>,
                        PersistentVariableInfo<"flashIsWorking", bool>,
                        PersistentVariableInfo<"nFlashErrors", std::uint16_t>,
                        PersistentVariableInfo<"rfIsWorking", bool>,
                        PersistentVariableInfo<"nRfErrors", std::uint16_t>,
                        PersistentVariableInfo<"nFileSystemErrors", std::uint16_t>,
                        PersistentVariableInfo<"eduShouldBePowered", bool>>{};
}

#pragma once


#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>


namespace sts1cobcsw
{
inline constexpr auto persistentVariablesSize = fram::Size(300);
inline constexpr auto eduProgramQueueSize = fram::Size(20 * 8);
inline constexpr auto eduProgramStatusHistorySize = fram::Size(50 * 7);
inline constexpr auto testMemorySize = fram::Size(1000);
inline constexpr auto telemetrySize = fram::memorySize - persistentVariablesSize
                                    - eduProgramQueueSize - eduProgramStatusHistorySize
                                    - testMemorySize;

inline constexpr auto framMemory = Section<fram::Address(0), fram::memorySize>{};
inline constexpr auto framSections =
    Subsections<framMemory,
                SubsectionInfo<"persistentVariables", persistentVariablesSize>,
                SubsectionInfo<"eduProgramQueue", eduProgramQueueSize>,
                SubsectionInfo<"eduProgramStatusHistory", eduProgramStatusHistorySize>,
                SubsectionInfo<"testMemory", testMemorySize>,
                SubsectionInfo<"telemetry", telemetrySize>>{};
inline constexpr auto persistentVariables =
    PersistentVariables<framSections.Get<"persistentVariables">(),
                        // Bootloader
                        PersistentVariableInfo<"nResetsSinceRf", std::uint8_t>,
                        PersistentVariableInfo<"activeSecondaryFwPartition", std::int8_t>,
                        PersistentVariableInfo<"backupSecondaryFwPartition", std::int8_t>,
                        // COBC state
                        PersistentVariableInfo<"txIsOn", bool>,
                        PersistentVariableInfo<"antennasShouldBeDeployed", bool>,
                        PersistentVariableInfo<"eduProgramQueueIndex", std::uint8_t>,
                        // Housekeeping
                        PersistentVariableInfo<"nTotalResets", std::uint32_t>,
                        PersistentVariableInfo<"realTime", RealTime>,
                        PersistentVariableInfo<"realTimeOffset", Duration>,
                        PersistentVariableInfo<"epsIsWorking", bool>,
                        PersistentVariableInfo<"flashIsWorking", bool>,
                        PersistentVariableInfo<"rfIsWorking", bool>,
                        PersistentVariableInfo<"nFlashErrors", std::uint8_t>,
                        PersistentVariableInfo<"nRfErrors", std::uint8_t>,
                        PersistentVariableInfo<"nFileSystemErrors", std::uint8_t>,
                        PersistentVariableInfo<"eduShouldBePowered", bool>,
                        PersistentVariableInfo<"nEduCommunicationErrors", std::uint8_t>>{};
}

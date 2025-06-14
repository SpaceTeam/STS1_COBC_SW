#pragma once


#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>


namespace sts1cobcsw
{
inline constexpr auto persistentVariablesSize = fram::Size(300);
inline constexpr auto eduProgramQueueSize = fram::Size(24 * 8 + 12);
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
                        PersistentVariableInfo<"nTotalResets", std::uint32_t>,
                        PersistentVariableInfo<"nResetsSinceRf", std::uint8_t>,
                        PersistentVariableInfo<"activeSecondaryFwPartition", std::int8_t>,
                        PersistentVariableInfo<"backupSecondaryFwPartition", std::int8_t>,
                        // Housekeeping
                        PersistentVariableInfo<"txIsOn", bool>,
                        PersistentVariableInfo<"antennasShouldBeDeployed", bool>,
                        PersistentVariableInfo<"realTime", RealTime>,
                        PersistentVariableInfo<"realTimeOffset", Duration>,
                        PersistentVariableInfo<"realTimeOffsetCorrection", Duration>,
                        PersistentVariableInfo<"nFirmwareChecksumErrors", std::uint8_t>,
                        PersistentVariableInfo<"epsIsWorking", bool>,
                        PersistentVariableInfo<"flashIsWorking", bool>,
                        PersistentVariableInfo<"rfIsWorking", bool>,
                        PersistentVariableInfo<"nFlashErrors", std::uint8_t>,
                        PersistentVariableInfo<"nRfErrors", std::uint8_t>,
                        PersistentVariableInfo<"nFileSystemErrors", std::uint8_t>,
                        // EDU
                        PersistentVariableInfo<"eduShouldBePowered", bool>,
                        PersistentVariableInfo<"newEduResultIsAvailable", bool>,
                        PersistentVariableInfo<"eduProgramQueueIndex", std::uint8_t>,
                        PersistentVariableInfo<"nEduCommunicationErrors", std::uint8_t>,
                        // Communication
                        PersistentVariableInfo<"nCorrectableUplinkErrors", std::uint16_t>,
                        PersistentVariableInfo<"nUncorrectableUplinkErrors", std::uint16_t>,
                        PersistentVariableInfo<"nGoodTransferFrames", std::uint16_t>,
                        PersistentVariableInfo<"nBadTransferFrames", std::uint16_t>,
                        PersistentVariableInfo<"lastFrameSequenceNumber", std::uint8_t>,
                        PersistentVariableInfo<"lastMessageTypeId", MessageTypeIdFields>,
                        PersistentVariableInfo<"lastMessageTypeIdWasInvalid", bool>,
                        PersistentVariableInfo<"lastTelecommandArgumentsWereInvalid", bool>>{};
}

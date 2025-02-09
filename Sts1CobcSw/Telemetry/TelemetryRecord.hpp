#pragma once


#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
// TODO: Merge the bools into bitfields
struct TelemetryRecord
{
    // Booleans: byte 1: bootloader and EDU
    bool fwChecksumsAreOk = false;
    bool eduShouldBePowered = false;
    bool eduHeartBeats = false;
    bool newResultIsAvailable = false;
    // Booleans: byte 2: housekeeping and communication
    bool antennasShouldBeDeployed = false;
    bool framIsWorking = false;
    bool epsIsWorking = false;
    bool flashIsWorking = false;
    bool rfIsWorking = false;
    bool lastTeleCommandIdWasInvalid = false;
    bool lastTeleCommandArgumentsWereInvalid = false;

    // BootLoader
    std::uint8_t nResetsSinceRf = 0U;
    std::int8_t activeSecondaryFwPartition = 0;
    std::int8_t backupSecondaryFwPartition = 0;

    // EDU
    std::int8_t eduProgramQueueIndex = 0;
    ProgramId programIdOfCurrentEduProgramQueueEntry;
    std::uint8_t nEduCommunicationErrors = 0U;

    // Housekeeping
    std::uint32_t nTotalResets = 0U;
    std::uint8_t lastResetReason = 0U;
    std::int32_t rodosTimeInSeconds = 0;
    RealTime realTime;
    std::uint8_t nFlashErrors = 0U;
    std::uint8_t nRfErrors = 0U;
    std::uint8_t nFileSystemErrors = 0U;

    // Sensor data
    std::uint8_t batteryPackVoltage = 0U;
    std::uint8_t batteryCenterTapVoltage = 0U;
    std::uint8_t batteryTemperature = 0U;
    std::uint8_t cobcTemperature = 0U;
    std::uint8_t cubeSatBusVoltage = 0U;
    std::uint8_t sidepanelXPlusTemperature = 0U;
    std::uint8_t sidepanelYPlusTemperature = 0U;
    std::uint8_t sidepanelYMinusTemperature = 0U;
    std::uint8_t sidepanelZPlusTemperature = 0U;
    std::uint8_t sidepanelZMinusTemperature = 0U;

    // Communication
    std::uint8_t rfBaudRate = 0U;
    std::uint16_t nCorrectableUplinkErrors = 0U;
    std::uint16_t nUncorrectableUplinkErrors = 0U;
    std::uint16_t nBadRfpackets = 0U;
    std::uint16_t nGoodRfpackets = 0U;
    std::uint8_t lastReceivedCommandId = 0U;

    friend auto operator==(TelemetryRecord const &, TelemetryRecord const &) -> bool = default;
};


template<>
inline constexpr std::size_t serialSize<TelemetryRecord> =
    2
    + totalSerialSize<
        // Bootloader
        decltype(TelemetryRecord::nResetsSinceRf),
        decltype(TelemetryRecord::activeSecondaryFwPartition),
        decltype(TelemetryRecord::backupSecondaryFwPartition),
        // EDU
        decltype(TelemetryRecord::eduProgramQueueIndex),
        decltype(TelemetryRecord::programIdOfCurrentEduProgramQueueEntry),
        decltype(TelemetryRecord::nEduCommunicationErrors),
        // Housekeeping
        decltype(TelemetryRecord::nTotalResets),
        decltype(TelemetryRecord::lastResetReason),
        decltype(TelemetryRecord::rodosTimeInSeconds),
        decltype(TelemetryRecord::realTime),
        decltype(TelemetryRecord::nFlashErrors),
        decltype(TelemetryRecord::nRfErrors),
        decltype(TelemetryRecord::nFileSystemErrors),
        // Sensor data
        decltype(TelemetryRecord::batteryPackVoltage),
        decltype(TelemetryRecord::batteryCenterTapVoltage),
        decltype(TelemetryRecord::batteryTemperature),
        decltype(TelemetryRecord::cobcTemperature),
        decltype(TelemetryRecord::cubeSatBusVoltage),
        decltype(TelemetryRecord::sidepanelXPlusTemperature),
        decltype(TelemetryRecord::sidepanelYPlusTemperature),
        decltype(TelemetryRecord::sidepanelYMinusTemperature),
        decltype(TelemetryRecord::sidepanelZPlusTemperature),
        decltype(TelemetryRecord::sidepanelZMinusTemperature),
        // Communication
        decltype(TelemetryRecord::rfBaudRate),
        decltype(TelemetryRecord::nCorrectableUplinkErrors),
        decltype(TelemetryRecord::nUncorrectableUplinkErrors),
        decltype(TelemetryRecord::nBadRfpackets),
        decltype(TelemetryRecord::nGoodRfpackets),
        decltype(TelemetryRecord::lastReceivedCommandId)>;


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, TelemetryRecord * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, TelemetryRecord const & data) -> void *;
}

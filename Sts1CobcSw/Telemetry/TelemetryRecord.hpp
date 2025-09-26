#pragma once


#include <Sts1CobcSw/Sensors/Eps.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>


namespace sts1cobcsw
{
struct TelemetryRecord
{
    // Booleans: byte 1
    UInt<1> eduShouldBePowered = 0;
    UInt<1> eduIsAlive = 0;
    UInt<1> newEduResultIsAvailable = 0;
    UInt<1> dosimeterIsPowered = 0;
    UInt<1> antennasShouldBeDeployed = 0;
    UInt<1> epsIsCharging = 0;
    UInt<1> epsDetectedFault = 0;
    UInt<1> framIsWorking = 0;
    // Booleans: byte 2
    UInt<1> epsIsWorking = 0;
    UInt<1> flashIsWorking = 0;
    UInt<1> rfIsWorking = 0;
    UInt<1> lastMessageTypeIdWasInvalid = 0;
    UInt<1> lastApplicationDataWasInvalid = 0;
    UInt<3> padding = 0;  // NOLINT(*magic-numbers)

    // BootLoader
    std::uint32_t nTotalResets = 0U;
    std::uint8_t nResetsSinceRf = 0U;
    PartitionId activeSecondaryFwPartition = PartitionId::secondary1;
    PartitionId backupSecondaryFwPartition = PartitionId::secondary1;

    // EDU
    std::uint8_t eduProgramQueueIndex = 0;
    ProgramId programIdOfCurrentEduProgramQueueEntry;
    std::uint8_t nEduCommunicationErrors = 0U;

    // Housekeeping
    std::uint8_t lastResetReason = 0U;
    std::int32_t rodosTimeInSeconds = 0;
    RealTime realTime;
    std::uint8_t nFirmwareChecksumErrors = 0U;
    std::uint8_t nFlashErrors = 0U;
    std::uint8_t nRfErrors = 0U;
    std::uint8_t nFileSystemErrors = 0U;

    // Sensor data
    std::uint16_t cobcTemperature = 0U;
    std::uint16_t rfTemperature = 0U;
    eps::AdcData epsAdcData = {};

    // Communication
    std::uint32_t rxDataRate = 0;
    std::uint32_t txDataRate = 0;
    std::uint16_t nCorrectableUplinkErrors = 0U;
    std::uint16_t nUncorrectableUplinkErrors = 0U;
    std::uint16_t nGoodTransferFrames = 0U;
    std::uint16_t nBadTransferFrames = 0U;
    std::uint8_t lastFrameSequenceNumber = 0U;
    MessageTypeIdFields lastMessageTypeId;

    friend auto operator==(TelemetryRecord const &, TelemetryRecord const &) -> bool = default;
};


template<>
inline constexpr std::size_t serialSize<TelemetryRecord> =
    totalSerialSize<decltype(TelemetryRecord::eduShouldBePowered),
                    decltype(TelemetryRecord::eduIsAlive),
                    decltype(TelemetryRecord::newEduResultIsAvailable),
                    decltype(TelemetryRecord::dosimeterIsPowered),
                    decltype(TelemetryRecord::antennasShouldBeDeployed),
                    decltype(TelemetryRecord::epsIsCharging),
                    decltype(TelemetryRecord::epsDetectedFault),
                    decltype(TelemetryRecord::framIsWorking),
                    decltype(TelemetryRecord::epsIsWorking),
                    decltype(TelemetryRecord::flashIsWorking),
                    decltype(TelemetryRecord::rfIsWorking),
                    decltype(TelemetryRecord::lastMessageTypeIdWasInvalid),
                    decltype(TelemetryRecord::lastApplicationDataWasInvalid),
                    decltype(TelemetryRecord::padding)>
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
        decltype(TelemetryRecord::nFirmwareChecksumErrors),
        decltype(TelemetryRecord::nFlashErrors),
        decltype(TelemetryRecord::nRfErrors),
        decltype(TelemetryRecord::nFileSystemErrors),
        // Sensor data
        decltype(TelemetryRecord::cobcTemperature),
        decltype(TelemetryRecord::rfTemperature),
        decltype(TelemetryRecord::epsAdcData),
        // Communication
        decltype(TelemetryRecord::rxDataRate),
        decltype(TelemetryRecord::txDataRate),
        decltype(TelemetryRecord::nCorrectableUplinkErrors),
        decltype(TelemetryRecord::nUncorrectableUplinkErrors),
        decltype(TelemetryRecord::nGoodTransferFrames),
        decltype(TelemetryRecord::nBadTransferFrames),
        decltype(TelemetryRecord::lastFrameSequenceNumber),
        decltype(TelemetryRecord::lastMessageTypeId)>;


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, TelemetryRecord * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, TelemetryRecord const & data) -> void *;
}

#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>


namespace sts1cobcsw
{
template<std::endian endianness>
auto DeserializeFrom(void const * source, TelemetryRecord * data) -> void const *
{
    // NOLINTBEGIN(*init-variables, *magic-numbers)
    std::uint8_t booleans1;
    source = DeserializeFrom<endianness>(source, &booleans1);
    data->fwChecksumsAreOk = booleans1 & 1U;
    data->eduShouldBePowered = booleans1 & (1U << 1U);
    data->eduHeartBeats = booleans1 & (1U << 2U);
    data->newResultIsAvailable = booleans1 & (1U << 3U);
    std::uint8_t booleans2;
    source = DeserializeFrom<endianness>(source, &booleans2);
    data->antennasShouldBeDeployed = booleans2 & 1U;
    data->framIsWorking = booleans2 & (1U << 1U);
    data->epsIsWorking = booleans2 & (1U << 2U);
    data->flashIsWorking = booleans2 & (1U << 3U);
    data->rfIsWorking = booleans2 & (1U << 4U);
    data->lastTeleCommandIdWasInvalid = booleans2 & (1U << 5U);
    data->lastTeleCommandArgumentsWereInvalid = booleans2 & (1U << 6U);
    // NOLINTEND(*init-variables, *magic-numbers)
    source = DeserializeFrom<endianness>(source, &(data->nResetsSinceRf));
    source = DeserializeFrom<endianness>(source, &(data->activeSecondaryFwPartition));
    source = DeserializeFrom<endianness>(source, &(data->backupSecondaryFwPartition));
    source = DeserializeFrom<endianness>(source, &(data->eduProgramQueueIndex));
    source = DeserializeFrom<endianness>(source, &(data->programIdOfCurrentEduProgramQueueEntry));
    source = DeserializeFrom<endianness>(source, &(data->nEduCommunicationErrors));
    source = DeserializeFrom<endianness>(source, &(data->nTotalResets));
    source = DeserializeFrom<endianness>(source, &(data->lastResetReason));
    source = DeserializeFrom<endianness>(source, &(data->rodosTimeInSeconds));
    source = DeserializeFrom<endianness>(source, &(data->realTime));
    source = DeserializeFrom<endianness>(source, &(data->nFlashErrors));
    source = DeserializeFrom<endianness>(source, &(data->nRfErrors));
    source = DeserializeFrom<endianness>(source, &(data->nFileSystemErrors));
    source = DeserializeFrom<endianness>(source, &(data->batteryPackVoltage));
    source = DeserializeFrom<endianness>(source, &(data->batteryCenterTapVoltage));
    source = DeserializeFrom<endianness>(source, &(data->batteryTemperature));
    source = DeserializeFrom<endianness>(source, &(data->cobcTemperature));
    source = DeserializeFrom<endianness>(source, &(data->cubeSatBusVoltage));
    source = DeserializeFrom<endianness>(source, &(data->sidepanelXPlusTemperature));
    source = DeserializeFrom<endianness>(source, &(data->sidepanelYPlusTemperature));
    source = DeserializeFrom<endianness>(source, &(data->sidepanelYMinusTemperature));
    source = DeserializeFrom<endianness>(source, &(data->sidepanelZPlusTemperature));
    source = DeserializeFrom<endianness>(source, &(data->sidepanelZMinusTemperature));
    source = DeserializeFrom<endianness>(source, &(data->rfBaudRate));
    source = DeserializeFrom<endianness>(source, &(data->nCorrectableUplinkErrors));
    source = DeserializeFrom<endianness>(source, &(data->nUncorrectableUplinkErrors));
    source = DeserializeFrom<endianness>(source, &(data->nBadRfpackets));
    source = DeserializeFrom<endianness>(source, &(data->nGoodRfpackets));
    source = DeserializeFrom<endianness>(source, &(data->lastReceivedCommandId));
    return source;
}


template<std::endian endianness>
auto SerializeTo(void * destination, TelemetryRecord const & data) -> void *
{
    // NOLINTBEGIN(*signed-bitwise, *magic-numbers)
    std::uint8_t booleans1 = data.fwChecksumsAreOk;
    booleans1 |= data.eduShouldBePowered << 1;
    booleans1 |= data.eduHeartBeats << 2;
    booleans1 |= data.newResultIsAvailable << 3;
    std::uint8_t booleans2 = data.antennasShouldBeDeployed;
    booleans2 |= data.framIsWorking << 1;
    booleans2 |= data.epsIsWorking << 2;
    booleans2 |= data.flashIsWorking << 3;
    booleans2 |= data.rfIsWorking << 4;
    booleans2 |= data.lastTeleCommandIdWasInvalid << 5;
    booleans2 |= data.lastTeleCommandArgumentsWereInvalid << 6;
    // NOLINTEND(*signed-bitwise, *magic-numbers)
    destination = SerializeTo<endianness>(destination, booleans1);
    destination = SerializeTo<endianness>(destination, booleans2);
    destination = SerializeTo<endianness>(destination, data.nResetsSinceRf);
    destination = SerializeTo<endianness>(destination, data.activeSecondaryFwPartition);
    destination = SerializeTo<endianness>(destination, data.backupSecondaryFwPartition);
    destination = SerializeTo<endianness>(destination, data.eduProgramQueueIndex);
    destination = SerializeTo<endianness>(destination, data.programIdOfCurrentEduProgramQueueEntry);
    destination = SerializeTo<endianness>(destination, data.nEduCommunicationErrors);
    destination = SerializeTo<endianness>(destination, data.nTotalResets);
    destination = SerializeTo<endianness>(destination, data.lastResetReason);
    destination = SerializeTo<endianness>(destination, data.rodosTimeInSeconds);
    destination = SerializeTo<endianness>(destination, data.realTime);
    destination = SerializeTo<endianness>(destination, data.nFlashErrors);
    destination = SerializeTo<endianness>(destination, data.nRfErrors);
    destination = SerializeTo<endianness>(destination, data.nFileSystemErrors);
    destination = SerializeTo<endianness>(destination, data.batteryPackVoltage);
    destination = SerializeTo<endianness>(destination, data.batteryCenterTapVoltage);
    destination = SerializeTo<endianness>(destination, data.batteryTemperature);
    destination = SerializeTo<endianness>(destination, data.cobcTemperature);
    destination = SerializeTo<endianness>(destination, data.cubeSatBusVoltage);
    destination = SerializeTo<endianness>(destination, data.sidepanelXPlusTemperature);
    destination = SerializeTo<endianness>(destination, data.sidepanelYPlusTemperature);
    destination = SerializeTo<endianness>(destination, data.sidepanelYMinusTemperature);
    destination = SerializeTo<endianness>(destination, data.sidepanelZPlusTemperature);
    destination = SerializeTo<endianness>(destination, data.sidepanelZMinusTemperature);
    destination = SerializeTo<endianness>(destination, data.rfBaudRate);
    destination = SerializeTo<endianness>(destination, data.nCorrectableUplinkErrors);
    destination = SerializeTo<endianness>(destination, data.nUncorrectableUplinkErrors);
    destination = SerializeTo<endianness>(destination, data.nBadRfpackets);
    destination = SerializeTo<endianness>(destination, data.nGoodRfpackets);
    destination = SerializeTo<endianness>(destination, data.lastReceivedCommandId);
    return destination;
}


template auto DeserializeFrom<std::endian::big>(void const *, TelemetryRecord *) -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, TelemetryRecord *) -> void const *;
template auto SerializeTo<std::endian::big>(void *, TelemetryRecord const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, TelemetryRecord const &) -> void *;
}

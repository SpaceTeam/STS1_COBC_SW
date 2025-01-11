#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>


namespace sts1cobcsw
{
template<std::endian endianness>
auto DeserializeFrom(void const * source, TelemetryRecord * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->nResetsSinceRf));
    source = DeserializeFrom<endianness>(source, &(data->activeSecondaryFwPartition));
    source = DeserializeFrom<endianness>(source, &(data->backupSecondaryFwPartition));
    source = DeserializeFrom<endianness>(source, &(data->fwChecksumsAreOk));
    source = DeserializeFrom<endianness>(source, &(data->eduShouldBePowered));
    source = DeserializeFrom<endianness>(source, &(data->eduHeartBeats));
    source = DeserializeFrom<endianness>(source, &(data->eduProgramQueueIndex));
    source = DeserializeFrom<endianness>(source, &(data->programIdOfCurrentProgramQueueEntry));
    source = DeserializeFrom<endianness>(source, &(data->newResultIsAvailable));
    source = DeserializeFrom<endianness>(source, &(data->nEduCommunicationErrors));
    source = DeserializeFrom<endianness>(source, &(data->nTotalResets));
    source = DeserializeFrom<endianness>(source, &(data->lastResetReason));
    source = DeserializeFrom<endianness>(source, &(data->rodosTimeInSeconds));
    source = DeserializeFrom<endianness>(source, &(data->realTime));
    source = DeserializeFrom<endianness>(source, &(data->antennasShouldBeDeployed));
    source = DeserializeFrom<endianness>(source, &(data->framIsWorking));
    source = DeserializeFrom<endianness>(source, &(data->epsIsWorking));
    source = DeserializeFrom<endianness>(source, &(data->flashIsWorking));
    source = DeserializeFrom<endianness>(source, &(data->rfIsWorking));
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
    source = DeserializeFrom<endianness>(source, &(data->lastReceivedCommandIdWasInvalid));
    source = DeserializeFrom<endianness>(source, &(data->lastReceivedCommandArgumentsWereInvalid));
    return source;
}


template<std::endian endianness>
auto SerializeTo(void * destination, TelemetryRecord const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, data.nResetsSinceRf);
    destination = SerializeTo<endianness>(destination, data.activeSecondaryFwPartition);
    destination = SerializeTo<endianness>(destination, data.backupSecondaryFwPartition);
    destination = SerializeTo<endianness>(destination, data.fwChecksumsAreOk);
    destination = SerializeTo<endianness>(destination, data.eduShouldBePowered);
    destination = SerializeTo<endianness>(destination, data.eduHeartBeats);
    destination = SerializeTo<endianness>(destination, data.eduProgramQueueIndex);
    destination = SerializeTo<endianness>(destination, data.programIdOfCurrentProgramQueueEntry);
    destination = SerializeTo<endianness>(destination, data.newResultIsAvailable);
    destination = SerializeTo<endianness>(destination, data.nEduCommunicationErrors);
    destination = SerializeTo<endianness>(destination, data.nTotalResets);
    destination = SerializeTo<endianness>(destination, data.lastResetReason);
    destination = SerializeTo<endianness>(destination, data.rodosTimeInSeconds);
    destination = SerializeTo<endianness>(destination, data.realTime);
    destination = SerializeTo<endianness>(destination, data.antennasShouldBeDeployed);
    destination = SerializeTo<endianness>(destination, data.framIsWorking);
    destination = SerializeTo<endianness>(destination, data.epsIsWorking);
    destination = SerializeTo<endianness>(destination, data.flashIsWorking);
    destination = SerializeTo<endianness>(destination, data.rfIsWorking);
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
    destination = SerializeTo<endianness>(destination, data.lastReceivedCommandIdWasInvalid);
    destination =
        SerializeTo<endianness>(destination, data.lastReceivedCommandArgumentsWereInvalid);
    return destination;
}


template auto DeserializeFrom<std::endian::big>(void const *, TelemetryRecord *) -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, TelemetryRecord *) -> void const *;
template auto SerializeTo<std::endian::big>(void *, TelemetryRecord const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, TelemetryRecord const &) -> void *;
}

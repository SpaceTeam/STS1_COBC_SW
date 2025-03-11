#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>


namespace sts1cobcsw
{
template<std::endian endianness>
auto DeserializeFrom(void const * source, TelemetryRecord * data) -> void const *
{
    // NOLINTBEGIN(*init-variables, *magic-numbers)
    std::uint8_t booleans1;
    source = DeserializeFrom<endianness>(source, &booleans1);
    data->eduShouldBePowered = booleans1 & (1U << 0U);
    data->eduIsAlive = booleans1 & (1U << 1U);
    data->newEduResultIsAvailable = booleans1 & (1U << 2U);
    data->antennasShouldBeDeployed = booleans1 & (1U << 3U);
    data->framIsWorking = booleans1 & (1U << 4U);
    data->epsIsWorking = booleans1 & (1U << 5U);
    data->flashIsWorking = booleans1 & (1U << 6U);
    data->rfIsWorking = booleans1 & (1U << 7U);
    std::uint8_t booleans2;
    source = DeserializeFrom<endianness>(source, &booleans2);
    data->lastTelecommandIdWasInvalid = booleans2 & (1U << 0U);
    data->lastTelecommandArgumentsWereInvalid = booleans2 & (1U << 1U);
    // NOLINTEND(*init-variables, *magic-numbers)
    source = DeserializeFrom<endianness>(source, &(data->nTotalResets));
    source = DeserializeFrom<endianness>(source, &(data->nResetsSinceRf));
    source = DeserializeFrom<endianness>(source, &(data->activeSecondaryFwPartition));
    source = DeserializeFrom<endianness>(source, &(data->backupSecondaryFwPartition));
    source = DeserializeFrom<endianness>(source, &(data->eduProgramQueueIndex));
    source = DeserializeFrom<endianness>(source, &(data->programIdOfCurrentEduProgramQueueEntry));
    source = DeserializeFrom<endianness>(source, &(data->nEduCommunicationErrors));
    source = DeserializeFrom<endianness>(source, &(data->lastResetReason));
    source = DeserializeFrom<endianness>(source, &(data->rodosTimeInSeconds));
    source = DeserializeFrom<endianness>(source, &(data->realTime));
    source = DeserializeFrom<endianness>(source, &(data->nFirmwareChecksumErrors));
    source = DeserializeFrom<endianness>(source, &(data->nFlashErrors));
    source = DeserializeFrom<endianness>(source, &(data->nRfErrors));
    source = DeserializeFrom<endianness>(source, &(data->nFileSystemErrors));
    source = DeserializeFrom<endianness>(source, &(data->cobcTemperature));
    source = DeserializeFrom<endianness>(source, &(data->rfTemperature));
    source = DeserializeFrom<endianness>(source, &(data->epsAdcData));
    source = DeserializeFrom<endianness>(source, &(data->rxBaudRate));
    source = DeserializeFrom<endianness>(source, &(data->txBaudRate));
    source = DeserializeFrom<endianness>(source, &(data->nCorrectableUplinkErrors));
    source = DeserializeFrom<endianness>(source, &(data->nUncorrectableUplinkErrors));
    source = DeserializeFrom<endianness>(source, &(data->nGoodTransferFrames));
    source = DeserializeFrom<endianness>(source, &(data->nBadTransferFrames));
    source = DeserializeFrom<endianness>(source, &(data->lastTelecommandId));
    return source;
}


template<std::endian endianness>
auto SerializeTo(void * destination, TelemetryRecord const & data) -> void *
{
    // NOLINTBEGIN(*signed-bitwise, *magic-numbers)
    std::uint8_t booleans1 = 0U;
    booleans1 |= data.eduShouldBePowered;
    booleans1 |= data.eduIsAlive << 1U;
    booleans1 |= data.newEduResultIsAvailable << 2U;
    booleans1 |= data.antennasShouldBeDeployed << 3U;
    booleans1 |= data.framIsWorking << 4U;
    booleans1 |= data.epsIsWorking << 5U;
    booleans1 |= data.flashIsWorking << 6U;
    booleans1 |= data.rfIsWorking << 7U;
    std::uint8_t booleans2 = 0U;
    booleans2 |= data.lastTelecommandIdWasInvalid;
    booleans2 |= data.lastTelecommandArgumentsWereInvalid << 1U;
    // NOLINTEND(*signed-bitwise, *magic-numbers)
    destination = SerializeTo<endianness>(destination, booleans1);
    destination = SerializeTo<endianness>(destination, booleans2);
    destination = SerializeTo<endianness>(destination, data.nTotalResets);
    destination = SerializeTo<endianness>(destination, data.nResetsSinceRf);
    destination = SerializeTo<endianness>(destination, data.activeSecondaryFwPartition);
    destination = SerializeTo<endianness>(destination, data.backupSecondaryFwPartition);
    destination = SerializeTo<endianness>(destination, data.eduProgramQueueIndex);
    destination = SerializeTo<endianness>(destination, data.programIdOfCurrentEduProgramQueueEntry);
    destination = SerializeTo<endianness>(destination, data.nEduCommunicationErrors);
    destination = SerializeTo<endianness>(destination, data.lastResetReason);
    destination = SerializeTo<endianness>(destination, data.rodosTimeInSeconds);
    destination = SerializeTo<endianness>(destination, data.realTime);
    destination = SerializeTo<endianness>(destination, data.nFirmwareChecksumErrors);
    destination = SerializeTo<endianness>(destination, data.nFlashErrors);
    destination = SerializeTo<endianness>(destination, data.nRfErrors);
    destination = SerializeTo<endianness>(destination, data.nFileSystemErrors);
    destination = SerializeTo<endianness>(destination, data.cobcTemperature);
    destination = SerializeTo<endianness>(destination, data.rfTemperature);
    destination = SerializeTo<endianness>(destination, data.epsAdcData);
    destination = SerializeTo<endianness>(destination, data.rxBaudRate);
    destination = SerializeTo<endianness>(destination, data.txBaudRate);
    destination = SerializeTo<endianness>(destination, data.nCorrectableUplinkErrors);
    destination = SerializeTo<endianness>(destination, data.nUncorrectableUplinkErrors);
    destination = SerializeTo<endianness>(destination, data.nGoodTransferFrames);
    destination = SerializeTo<endianness>(destination, data.nBadTransferFrames);
    destination = SerializeTo<endianness>(destination, data.lastTelecommandId);
    return destination;
}


template auto DeserializeFrom<std::endian::big>(void const *, TelemetryRecord *) -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, TelemetryRecord *) -> void const *;
template auto SerializeTo<std::endian::big>(void *, TelemetryRecord const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, TelemetryRecord const &) -> void *;
}

#pragma once

#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>

namespace sts1cobcsw::fw
{
template<std::endian endianness>
auto SerializeTo(void * destination, PartitionInfo const & data) -> void *
{
    using sts1cobcsw::SerializeTo;

    destination = SerializeTo<endianness>(destination, data.checksum);
    destination = SerializeTo<endianness>(destination, data.firmwareLength);
    destination = SerializeTo<endianness>(destination, data.commitHash);
    return destination;
}


template<std::endian endianness>
auto SerializeTo(void * destination, FirmwareChecksums const & data) -> void *
{
    using sts1cobcsw::SerializeTo;

    destination = SerializeTo<endianness>(destination, data.computed);
    destination = SerializeTo<endianness>(destination, data.stored);
    return destination;
}
}

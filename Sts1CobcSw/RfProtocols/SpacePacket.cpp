#include <Sts1CobcSw/RfProtocols/SpacePacket.hpp>

#include <Sts1CobcSw/RfProtocols/IdCounters.hpp>


namespace sts1cobcsw
{
namespace
{
auto packetSequenceCounters = IdCounters<std::uint16_t, Apid>{};
}


auto AddSpacePacketTo(etl::ivector<Byte> * dataField, Apid apid, Payload const & payload)
    -> Result<void>
{
    if(payload.Size() == 0)
    {
        // TODO: Think about what to return here
        return ErrorCode::emptyPayload;
    }
    if(dataField->available()
       < packetPrimaryHeaderLength + static_cast<std::size_t>(payload.Size()))
    {
        return ErrorCode::tooLarge;
    }
    auto * packetBegin = dataField->data() + dataField->size();
    dataField->resize(dataField->size() + packetPrimaryHeaderLength);
    auto primaryHeader = SpacePacketPrimaryHeader{
        .versionNumber = packetVersionNumber,
        .packetType = packettype::telemetry,
        .secondaryHeaderFlag = 1,
        .apid = apid,
        .sequenceFlags = 0b11,
        .packetSequenceCount = packetSequenceCounters.PostIncrement(apid),
        .packetDataLength = static_cast<std::uint16_t>(payload.Size() - 1U)};
    (void)SerializeTo<std::endian::big>(packetBegin, primaryHeader);
    return payload.AddTo(dataField);
}


auto ParseAsSpacePacket(std::span<Byte const> buffer) -> Result<SpacePacket>
{
    if(buffer.size() < packetPrimaryHeaderLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto packet = SpacePacket{};
    (void)DeserializeFrom<ccsdsEndianness>(buffer.data(), &packet.primaryHeader);
    auto packetIsValid = packet.primaryHeader.versionNumber == packetVersionNumber
                     and packet.primaryHeader.packetType == packettype::telecommand
                     and packet.primaryHeader.secondaryHeaderFlag == 1
                     and packet.primaryHeader.sequenceFlags == 0b11;
    if(not packetIsValid)
    {
        return ErrorCode::invalidSpacePacket;
    }
    if(not IsValid(packet.primaryHeader.apid))
    {
        return ErrorCode::invalidApid;
    }
    if(packet.primaryHeader.packetDataLength + 1U > tc::maxPacketDataLength)
    {
        return ErrorCode::invalidPacketDataLength;
    }
    if(buffer.size() < packetPrimaryHeaderLength
                           + static_cast<std::size_t>(packet.primaryHeader.packetDataLength) + 1)
    {
        return ErrorCode::bufferTooSmall;
    }
    packet.dataField =
        buffer.subspan(packetPrimaryHeaderLength, packet.primaryHeader.packetDataLength + 1);
    return packet;
}


template<std::endian endianness>
auto SerializeTo(void * destination, SpacePacketPrimaryHeader const & header) -> void *
{
    destination = SerializeTo<endianness>(destination,
                                          header.versionNumber,
                                          header.packetType,
                                          header.secondaryHeaderFlag,
                                          header.apid.Value(),
                                          header.sequenceFlags,
                                          header.packetSequenceCount);
    destination = SerializeTo<endianness>(destination, header.packetDataLength);
    return destination;
}


template auto SerializeTo<std::endian::big>(void * destination,
                                            SpacePacketPrimaryHeader const & header) -> void *;


template<std::endian endianness>
auto DeserializeFrom(void const * source, SpacePacketPrimaryHeader * header) -> void const *
{
    auto apidValue = Apid::ValueType{};
    source = DeserializeFrom<endianness>(source,
                                         &header->versionNumber,
                                         &header->packetType,
                                         &header->secondaryHeaderFlag,
                                         &apidValue,
                                         &header->sequenceFlags,
                                         &header->packetSequenceCount);
    header->apid = Apid(apidValue);
    source = DeserializeFrom<endianness>(source, &header->packetDataLength);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const * source,
                                                SpacePacketPrimaryHeader * header) -> void const *;
}

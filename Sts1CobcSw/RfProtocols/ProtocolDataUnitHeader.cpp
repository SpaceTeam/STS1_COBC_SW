#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>


namespace sts1cobcsw
{
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ProtocolDataUnitHeader const & header) -> void *
{
    destination = SerializeTo<endianness>(destination,
                                          header.version,
                                          header.pduType,
                                          header.direction,
                                          header.transmissionMode,
                                          header.crcFlag,
                                          header.largeFileFlag);
    destination = SerializeTo<endianness>(destination, header.pduDataFieldLength);
    destination = SerializeTo<endianness>(destination,
                                          header.segmentationControl,
                                          header.lengthOfEntityIds,
                                          header.segmentMetadataFlag,
                                          header.lengthOfTransactionSequenceNumber);
    destination = SerializeTo<endianness>(destination, header.sourceEntityId);
    destination = SerializeTo<endianness>(destination, header.transactionSequenceNumber);
    destination = SerializeTo<endianness>(destination, header.destinationEntityId);
    return destination;
}


template auto SerializeTo<std::endian::big>(void * destination,
                                            ProtocolDataUnitHeader const & header) -> void *;


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProtocolDataUnitHeader * header)
    -> void const *
{
    source = DeserializeFrom<endianness>(source,
                                         &header->version,
                                         &header->pduType,
                                         &header->direction,
                                         &header->transmissionMode,
                                         &header->crcFlag,
                                         &header->largeFileFlag);
    source = DeserializeFrom<endianness>(source, &header->pduDataFieldLength);
    source = DeserializeFrom<endianness>(source,
                                         &header->segmentationControl,
                                         &header->lengthOfEntityIds,
                                         &header->segmentMetadataFlag,
                                         &header->lengthOfTransactionSequenceNumber);
    source = DeserializeFrom<endianness>(source, &header->sourceEntityId);
    source = DeserializeFrom<endianness>(source, &header->transactionSequenceNumber);
    source = DeserializeFrom<endianness>(source, &header->destinationEntityId);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const * source,
                                                ProtocolDataUnitHeader * header) -> void const *;
}

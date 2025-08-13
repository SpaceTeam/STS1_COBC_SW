#include <Sts1CobcSw/RfProtocols/ProtocolDataUnits.hpp>

#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/RfProtocols/Utility.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <strong_type/equality.hpp>

#include <algorithm>
#include <utility>


namespace sts1cobcsw
{
auto FileDataPdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, offset);
    std::ranges::copy(fileData, static_cast<Byte *>(cursor));
}


auto FileDataPdu::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(totalSerialSize<decltype(offset)> + fileData.size());
}


auto EndOfFilePdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor =
        SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, value_of(conditionCode), spare);
    cursor = SerializeTo<ccsdsEndianness>(cursor, fileChecksum);
    cursor = SerializeTo<ccsdsEndianness>(cursor, fileSize);
    if(conditionCode != noErrorConditionCode)
    {
        (void)SerializeTo<ccsdsEndianness>(cursor, faultLocation);
    }
}


auto EndOfFilePdu::DoSize() const -> std::uint16_t
{
    return conditionCode == noErrorConditionCode
             ? minParameterFieldLength
             : minParameterFieldLength + totalSerialSize<FaultLocation>;
}


auto FinishedPdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize,
                                                 value_of(conditionCode),
                                                 spare,
                                                 value_of(deliveryCode),
                                                 value_of(fileStatus));
    if(conditionCode != noErrorConditionCode
       and conditionCode != unsupportedChecksumTypeConditionCode)
    {
        (void)SerializeTo<ccsdsEndianness>(cursor, faultLocation);
    }
}


auto FinishedPdu::DoSize() const -> std::uint16_t
{
    return (conditionCode == noErrorConditionCode
            or conditionCode == unsupportedChecksumTypeConditionCode)
             ? minParameterFieldLength
             : minParameterFieldLength + totalSerialSize<FaultLocation>;
}


auto AckPdu::DoSize() const -> std::uint16_t
{
    return minParameterFieldLength;
}


auto AckPdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize,
                                                 acknowledgedPduDirectiveCode,
                                                 directiveSubtypeCode,
                                                 value_of(conditionCode),
                                                 spare,
                                                 transactionStatus);
}


auto ParseAsProtocolDataUnit(std::span<Byte const> buffer) -> Result<tc::ProtocolDataUnit>
{
    if(buffer.size() < tc::pduHeaderLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto pdu = tc::ProtocolDataUnit{};
    (void)DeserializeFrom<ccsdsEndianness>(buffer.data(), &pdu.header);
    auto pduIsValid = pdu.header.version == pduVersion
                   && pdu.header.transmissionMode == acknowledgedTransmissionMode
                   && pdu.header.crcFlag == 0 && pdu.header.largeFileFlag == 0
                   && pdu.header.segmentationControl == 0
                   && pdu.header.lengthOfEntityIds == totalSerialSize<EntityId> - 1
                   && pdu.header.segmentMetadataFlag == 0
                   && pdu.header.lengthOfTransactionSequenceNumber
                          == totalSerialSize<decltype(pdu.header.transactionSequenceNumber)> - 1;
    if(not pduIsValid)
    {
        return ErrorCode::invalidProtocolDataUnit;
    }
    if(pdu.header.pduDataFieldLength > tc::maxPduDataLength)
    {
        return ErrorCode::invalidPduDataLength;
    }
    auto entityIdsAreValid = IsValid(pdu.header.sourceEntityId)
                          && IsValid(pdu.header.destinationEntityId)
                          && pdu.header.sourceEntityId != pdu.header.destinationEntityId;
    if(not entityIdsAreValid)
    {
        return ErrorCode::invalidEntityId;
    }
    if(buffer.size() < tc::pduHeaderLength + pdu.header.pduDataFieldLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    pdu.dataField.uninitialized_resize(pdu.header.pduDataFieldLength);
    std::ranges::copy(buffer.subspan(tc::pduHeaderLength, pdu.header.pduDataFieldLength),
                      pdu.dataField.begin());
    return pdu;
}


auto ParseAsFileDataPdu(std::span<Byte const> buffer) -> Result<FileDataPdu>
{
    if(buffer.size() < totalSerialSize<decltype(FileDataPdu::offset)>)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto fileDataPdu = FileDataPdu{};
    (void)DeserializeFrom<ccsdsEndianness>(buffer.data(), &fileDataPdu.offset);
    fileDataPdu.fileData = buffer.subspan<totalSerialSize<decltype(FileDataPdu::offset)>>();
    return fileDataPdu;
}


auto ParseAsFileDirectivePdu(std::span<Byte const> buffer) -> Result<FileDirectivePdu>
{
    if(buffer.size() < totalSerialSize<decltype(FileDirectivePdu::directiveCode)>)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto fileDirectivePdu = FileDirectivePdu{};
    (void)DeserializeFrom<ccsdsEndianness>(buffer.data(), &fileDirectivePdu.directiveCode);
    fileDirectivePdu.parameterField =
        buffer.subspan<totalSerialSize<decltype(fileDirectivePdu.directiveCode)>>();
    if(not IsValid(fileDirectivePdu.directiveCode))
    {
        return ErrorCode::invalidFileDirectiveCode;
    }
    return fileDirectivePdu;
}


auto ParseAsEndOfFilePdu(std::span<Byte const> buffer) -> Result<EndOfFilePdu>
{
    if(buffer.size() < EndOfFilePdu::minParameterFieldLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto endOfFilePdu = EndOfFilePdu{};
    auto const * cursor = DeserializeFrom<ccsdsEndianness>(
        buffer.data(), &value_of(endOfFilePdu.conditionCode), &endOfFilePdu.spare);
    cursor = DeserializeFrom<ccsdsEndianness>(cursor, &endOfFilePdu.fileChecksum);
    cursor = DeserializeFrom<ccsdsEndianness>(cursor, &endOfFilePdu.fileSize);
    if(endOfFilePdu.conditionCode != noErrorConditionCode)
    {
        if(buffer.size() != EndOfFilePdu::minParameterFieldLength + totalSerialSize<FaultLocation>)
        {
            return ErrorCode::invalidDataLength;
        }
        (void)DeserializeFrom<ccsdsEndianness>(cursor, &endOfFilePdu.faultLocation);
        auto faultLocationIsValid = endOfFilePdu.faultLocation.type == TlvType::entityId
                                and endOfFilePdu.faultLocation.length == totalSerialSize<EntityId>
                                and IsValid(endOfFilePdu.faultLocation.value);
        if(not faultLocationIsValid)
        {
            return ErrorCode::invalidFaultLocation;
        }
    }
    return endOfFilePdu;
}


auto ParseAsFinishedPdu(std::span<Byte const> buffer) -> Result<FinishedPdu>
{
    if(buffer.size() < FinishedPdu::minParameterFieldLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto finishedPdu = FinishedPdu{};
    auto const * cursor = DeserializeFrom<ccsdsEndianness>(buffer.data(),
                                                           &value_of(finishedPdu.conditionCode),
                                                           &finishedPdu.spare,
                                                           &value_of(finishedPdu.deliveryCode),
                                                           &value_of(finishedPdu.fileStatus));
    if(finishedPdu.conditionCode == noErrorConditionCode
       or finishedPdu.conditionCode == unsupportedChecksumTypeConditionCode)
    {
        return finishedPdu;
    }
    if(buffer.size() != FinishedPdu::minParameterFieldLength + totalSerialSize<FaultLocation>)
    {
        return ErrorCode::invalidDataLength;
    }
    (void)DeserializeFrom<ccsdsEndianness>(cursor, &finishedPdu.faultLocation);
    auto faultLocationIsValid = finishedPdu.faultLocation.type == TlvType::entityId
                            and finishedPdu.faultLocation.length == totalSerialSize<EntityId>
                            and IsValid(finishedPdu.faultLocation.value);
    if(not faultLocationIsValid)
    {
        return ErrorCode::invalidFaultLocation;
    }
    return finishedPdu;
}


auto ParseAsAckPdu(std::span<Byte const> buffer) -> Result<AckPdu>
{
    if(buffer.size() < AckPdu::minParameterFieldLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto ackPdu = AckPdu{};
    auto const * cursor = DeserializeFrom<ccsdsEndianness>(
        buffer.data(), &ackPdu.acknowledgedPduDirectiveCode, &ackPdu.directiveSubtypeCode);
    cursor = DeserializeFrom<ccsdsEndianness>(
        cursor, &value_of(ackPdu.conditionCode), &ackPdu.spare, &ackPdu.transactionStatus);
    return ackPdu;
}

auto IsValid(DirectiveCode directiveCode) -> bool
{
    switch(directiveCode)
    {
        case DirectiveCode::endOfFile:
        case DirectiveCode::finished:
        case DirectiveCode::ack:
        case DirectiveCode::metadata:
        case DirectiveCode::nack:
            return true;
    }
    return false;
}


// --- De-/Serialization ---

template<std::endian endianness>
auto SerializeTo(void * destination, FaultLocation const & faultLocation) -> void *
{
    destination = SerializeTo<endianness>(destination, faultLocation.type);
    destination = SerializeTo<endianness>(destination, faultLocation.length);
    destination = SerializeTo<endianness>(destination, faultLocation.value);
    return destination;
}


template auto SerializeTo<std::endian::big>(void *, FaultLocation const &) -> void *;


template<std::endian endianness>
auto DeserializeFrom(void const * source, FaultLocation * faultLocation) -> void const *
{
    source = DeserializeFrom<endianness>(source, &faultLocation->type);
    source = DeserializeFrom<endianness>(source, &faultLocation->length);
    source = DeserializeFrom<endianness>(source, &faultLocation->value);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const *, FaultLocation *) -> void const *;
}

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
}

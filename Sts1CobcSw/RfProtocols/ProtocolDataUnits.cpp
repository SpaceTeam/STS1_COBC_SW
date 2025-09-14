#include <Sts1CobcSw/RfProtocols/ProtocolDataUnits.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/RfProtocols/Utility.hpp>
#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>

#include <strong_type/equality.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <cassert>
#include <utility>


namespace sts1cobcsw
{
FileDataPdu::FileDataPdu(std::uint32_t offset, std::span<Byte const> fileData)
    : offset_(offset), fileData_(fileData)
{
    assert(fileData.size() <= maxFileSegmentLength);
}


auto FileDataPdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, offset_);
    std::ranges::copy(fileData_, static_cast<Byte *>(cursor));
}


auto FileDataPdu::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(totalSerialSize<decltype(offset_)> + fileData_.size());
}


EndOfFilePdu::EndOfFilePdu(std::uint32_t fileSize) noexcept
    : conditionCode_(noErrorConditionCode), fileSize_(fileSize)
{
    // No FaultLocation implies noErrorConditionCode
}


EndOfFilePdu::EndOfFilePdu(ConditionCode conditionCode,
                           std::uint32_t fileSize,
                           FaultLocation faultLocation) noexcept
    : conditionCode_(conditionCode), fileSize_(fileSize), faultLocation_(faultLocation)
{
    // We cannot have a FaultLocation if there is no error
    assert(conditionCode != noErrorConditionCode);
}


auto EndOfFilePdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor =
        SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, value_of(conditionCode_), spare_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, fileChecksum_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, fileSize_);
    if(conditionCode_ != noErrorConditionCode)
    {
        (void)SerializeTo<ccsdsEndianness>(cursor, faultLocation_);
    }
}


auto EndOfFilePdu::DoSize() const -> std::uint16_t
{
    return conditionCode_ == noErrorConditionCode
             ? minParameterFieldLength
             : minParameterFieldLength + totalSerialSize<FaultLocation>;
}


FinishedPdu::FinishedPdu(DeliveryCode deliveryCode, FileStatus fileStatus) noexcept
    : conditionCode_(noErrorConditionCode), deliveryCode_(deliveryCode), fileStatus_(fileStatus)
{
    // No FaultLocation implies noErrorConditionCode since we ignore "unsupported checksum type"
}


FinishedPdu::FinishedPdu(ConditionCode conditionCode,
                         DeliveryCode deliveryCode,
                         FileStatus fileStatus,
                         FaultLocation faultLocation) noexcept
    : conditionCode_(conditionCode),
      deliveryCode_(deliveryCode),
      fileStatus_(fileStatus),
      faultLocation_(faultLocation)
{
    // We cannot have a FaultLocation if there is no error or an unsupported checksum type
    assert(conditionCode != noErrorConditionCode
           and conditionCode != unsupportedChecksumTypeConditionCode);
}


auto FinishedPdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize,
                                                 value_of(conditionCode_),
                                                 spare_,
                                                 value_of(deliveryCode_),
                                                 value_of(fileStatus_));
    if(conditionCode_ != noErrorConditionCode
       and conditionCode_ != unsupportedChecksumTypeConditionCode)
    {
        (void)SerializeTo<ccsdsEndianness>(cursor, faultLocation_);
    }
}


auto FinishedPdu::DoSize() const -> std::uint16_t
{
    return (conditionCode_ == noErrorConditionCode
            or conditionCode_ == unsupportedChecksumTypeConditionCode)
             ? minParameterFieldLength
             : minParameterFieldLength + totalSerialSize<FaultLocation>;
}


AckPdu::AckPdu(DirectiveCode acknowledgedDirectiveCode,
               ConditionCode conditionCode,
               TransactionStatus transactionStatus) noexcept
    : acknowledgedPduDirectiveCode_(static_cast<std::uint8_t>(acknowledgedDirectiveCode)),
      directiveSubtypeCode_(acknowledgedDirectiveCode == DirectiveCode::finished ? 1 : 0),
      conditionCode_(conditionCode),
      transactionStatus_(transactionStatus)
{}


auto AckPdu::DoSize() const -> std::uint16_t
{
    return minParameterFieldLength;
}


auto AckPdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    (void)SerializeTo<ccsdsEndianness>(dataField->data() + oldSize,
                                       acknowledgedPduDirectiveCode_,
                                       directiveSubtypeCode_,
                                       value_of(conditionCode_),
                                       spare_,
                                       value_of(transactionStatus_));
}


MetadataPdu::MetadataPdu(std::uint32_t fileSize,
                         fs::Path const & sourceFileName,
                         fs::Path const & destinationFileName) noexcept
    : fileSize_(fileSize),
      sourceFileNameLength_(static_cast<std::uint8_t>(sourceFileName.size())),
      sourceFileNameValue_(sourceFileName),
      destinationFileNameLength_(static_cast<std::uint8_t>(destinationFileName.size())),
      destinationFileNameValue_(destinationFileName)
{}


auto MetadataPdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize,
                                                 reserved1_,
                                                 closureRequested_,
                                                 checksumType_.Value(),
                                                 reserved2_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, fileSize_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, sourceFileNameLength_);
    cursor = std::ranges::copy(sourceFileNameValue_, static_cast<char *>(cursor)).out;
    cursor = SerializeTo<ccsdsEndianness>(cursor, destinationFileNameLength_);
    std::ranges::copy(destinationFileNameValue_, static_cast<char *>(cursor));
}


auto MetadataPdu::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(totalSerialSize<decltype(reserved1_),
                                                      decltype(closureRequested_),
                                                      ChecksumType::ValueType,
                                                      decltype(reserved2_)>
                                      + totalSerialSize<decltype(fileSize_),
                                                        decltype(sourceFileNameLength_),
                                                        decltype(destinationFileNameLength_)>
                                      + sourceFileNameValue_.size()
                                      + destinationFileNameValue_.size());
}


NakPdu::NakPdu(std::uint32_t endOfScope,
               etl::vector<SegmentRequest, maxSegmentRequests> const & segmentRequests) noexcept
    : endOfScope_(endOfScope), segmentRequests_(segmentRequests)
{
    assert(segmentRequests_.size() <= NakPdu::maxSegmentRequests);
}


auto NakPdu::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, startOfScope_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, endOfScope_);
    (void)SerializeTo<ccsdsEndianness>(cursor, segmentRequests_);
}


auto NakPdu::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(startOfScope_), decltype(endOfScope_)>
        + segmentRequests_.size() * sizeof(std::uint64_t));
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
    if(buffer.size() < totalSerialSize<decltype(FileDataPdu::offset_)>)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto fileDataPdu = FileDataPdu{};
    (void)DeserializeFrom<ccsdsEndianness>(buffer.data(), &fileDataPdu.offset_);
    fileDataPdu.fileData_ = buffer.subspan<totalSerialSize<decltype(FileDataPdu::offset_)>>();
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
        buffer.data(), &value_of(endOfFilePdu.conditionCode_), &endOfFilePdu.spare_);
    cursor = DeserializeFrom<ccsdsEndianness>(cursor, &endOfFilePdu.fileChecksum_);
    cursor = DeserializeFrom<ccsdsEndianness>(cursor, &endOfFilePdu.fileSize_);
    if(endOfFilePdu.conditionCode_ != noErrorConditionCode)
    {
        if(buffer.size() != EndOfFilePdu::minParameterFieldLength + totalSerialSize<FaultLocation>)
        {
            return ErrorCode::invalidDataLength;
        }
        (void)DeserializeFrom<ccsdsEndianness>(cursor, &endOfFilePdu.faultLocation_);
        auto faultLocationIsValid = endOfFilePdu.faultLocation_.type == TlvType::entityId
                                and endOfFilePdu.faultLocation_.length == totalSerialSize<EntityId>
                                and IsValid(endOfFilePdu.faultLocation_.value);
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
                                                           &value_of(finishedPdu.conditionCode_),
                                                           &finishedPdu.spare_,
                                                           &value_of(finishedPdu.deliveryCode_),
                                                           &value_of(finishedPdu.fileStatus_));
    if(finishedPdu.conditionCode_ == noErrorConditionCode
       or finishedPdu.conditionCode_ == unsupportedChecksumTypeConditionCode)
    {
        return finishedPdu;
    }
    if(buffer.size() != FinishedPdu::minParameterFieldLength + totalSerialSize<FaultLocation>)
    {
        return ErrorCode::invalidDataLength;
    }
    (void)DeserializeFrom<ccsdsEndianness>(cursor, &finishedPdu.faultLocation_);
    auto faultLocationIsValid = finishedPdu.faultLocation_.type == TlvType::entityId
                            and finishedPdu.faultLocation_.length == totalSerialSize<EntityId>
                            and IsValid(finishedPdu.faultLocation_.value);
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
        buffer.data(), &ackPdu.acknowledgedPduDirectiveCode_, &ackPdu.directiveSubtypeCode_);
    if(ackPdu.acknowledgedPduDirectiveCode_.ToUnderlying()
           != static_cast<std::uint8_t>(DirectiveCode::finished)
       and ackPdu.acknowledgedPduDirectiveCode_.ToUnderlying()
               != static_cast<std::uint8_t>(DirectiveCode::endOfFile))
    {
        return ErrorCode::invalidAckPduDirectiveCode;
    }
    if(ackPdu.directiveSubtypeCode_ != 0 and ackPdu.directiveSubtypeCode_ != 1)
    {
        return ErrorCode::invalidDirectiveSubtypeCode;
    }
    (void)DeserializeFrom<ccsdsEndianness>(cursor,
                                           &value_of(ackPdu.conditionCode_),
                                           &ackPdu.spare_,
                                           &value_of(ackPdu.transactionStatus_));
    return ackPdu;
}


auto ParseAsMetadataPdu(std::span<Byte const> buffer) -> Result<MetadataPdu>
{
    if(buffer.size() < MetadataPdu::minParameterFieldLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto metadataPdu = MetadataPdu{};
    auto const * cursor = static_cast<void const *>(buffer.data());
    auto checksumTypeValue = ChecksumType::ValueType{};
    cursor = DeserializeFrom<ccsdsEndianness>(cursor,
                                              &metadataPdu.reserved1_,
                                              &metadataPdu.closureRequested_,
                                              &metadataPdu.reserved2_,
                                              &checksumTypeValue);
    metadataPdu.checksumType_ = ChecksumType(checksumTypeValue);
    // TODO: Error handling
    cursor = DeserializeFrom<ccsdsEndianness>(cursor, &metadataPdu.fileSize_);
    cursor = DeserializeFrom<ccsdsEndianness>(cursor, &metadataPdu.sourceFileNameLength_);

    if(buffer.size() < MetadataPdu::minParameterFieldLength + metadataPdu.sourceFileNameLength_)
    {
        return ErrorCode::bufferTooSmall;
    }

    auto sourceFileNameValueBuffer =
        buffer.subspan(MetadataPdu::minParameterFieldLength,
                       static_cast<std::uint32_t>(metadataPdu.sourceFileNameLength_));

    metadataPdu.sourceFileNameValue_ = fs::Path();
    for(auto const byte : sourceFileNameValueBuffer)
    {
        metadataPdu.sourceFileNameValue_.push_back(static_cast<char>(byte));
    }

    cursor = DeserializeFrom<ccsdsEndianness>(
        buffer.data() + MetadataPdu::minParameterFieldLength
            + static_cast<std::size_t>(metadataPdu.sourceFileNameLength_),
        &metadataPdu.destinationFileNameLength_);

    if(buffer.size() < MetadataPdu::minParameterFieldLength + metadataPdu.sourceFileNameLength_
                           + metadataPdu.destinationFileNameLength_ + 1)
    {
        return ErrorCode::bufferTooSmall;
    }

    auto destinationFileNameValueBuffer =
        buffer.subspan(MetadataPdu::minParameterFieldLength + 1U
                           + static_cast<std::size_t>(metadataPdu.sourceFileNameLength_),
                       static_cast<std::uint32_t>(metadataPdu.destinationFileNameLength_));
    metadataPdu.destinationFileNameValue_ = fs::Path();
    for(auto const byte : destinationFileNameValueBuffer)
    {
        metadataPdu.destinationFileNameValue_.push_back(static_cast<char>(byte));
    }


    return metadataPdu;
}


auto ParseAsNakPdu(std::span<Byte const> buffer) -> Result<NakPdu>
{
    auto nakPdu = NakPdu{};
    auto const * cursor = DeserializeFrom<ccsdsEndianness>(buffer.data(), &nakPdu.startOfScope_);
    cursor = DeserializeFrom<ccsdsEndianness>(cursor, &nakPdu.endOfScope_);
    (void)DeserializeFrom<ccsdsEndianness>(cursor, nakPdu.segmentRequests_.data());
    return nakPdu;
}


auto IsValid(DirectiveCode directiveCode) -> bool
{
    switch(directiveCode)
    {
        case DirectiveCode::endOfFile:
        case DirectiveCode::finished:
        case DirectiveCode::ack:
        case DirectiveCode::metadata:
        case DirectiveCode::nak:
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


template<std::endian endianness>
auto SerializeTo(void * destination, SegmentRequest const & segmentRequest) -> void *
{
    destination = SerializeTo<endianness>(destination, segmentRequest.startOffset);
    destination = SerializeTo<endianness>(destination, segmentRequest.endOffset);
    return destination;
}


template auto SerializeTo<std::endian::big>(void *, SegmentRequest const &) -> void *;


template<std::endian endianness>
auto DeserializeFrom(void const * source, SegmentRequest * segmentRequest) -> void const *
{
    source = sts1cobcsw::DeserializeFrom<endianness>(source, &(segmentRequest->startOffset));
    source = sts1cobcsw::DeserializeFrom<endianness>(source, &(segmentRequest->endOffset));
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const *, SegmentRequest *) -> void const *;
}

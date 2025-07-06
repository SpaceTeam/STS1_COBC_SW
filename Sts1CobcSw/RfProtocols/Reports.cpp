#include <Sts1CobcSw/RfProtocols/Reports.hpp>

#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RfProtocols/IdCounters.hpp>
#include <Sts1CobcSw/RfProtocols/Utility.hpp>
#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>

#include <algorithm>


namespace sts1cobcsw
{
namespace
{
auto messageTypeCounters = IdCounters<std::uint16_t, tm::MessageTypeId>{};


template<tm::MessageTypeId id>
auto UpdateMessageTypeCounterAndTime(tm::SpacePacketSecondaryHeader<id> * secondaryHeader) -> void;
}


// --- Public function definitions ---

template<VerificationStage stage>
SuccessfulVerificationReport<stage>::SuccessfulVerificationReport(RequestId const & requestId)
    : requestId_(requestId)
{}


template<VerificationStage stage>
auto SuccessfulVerificationReport<stage>::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    (void)SerializeTo<ccsdsEndianness>(cursor, requestId_);
}


template<VerificationStage stage>
auto SuccessfulVerificationReport<stage>::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(secondaryHeader_), decltype(requestId_)>);
}


template class SuccessfulVerificationReport<VerificationStage::acceptance>;
template class SuccessfulVerificationReport<VerificationStage::completionOfExecution>;


template<VerificationStage stage>
FailedVerificationReport<stage>::FailedVerificationReport(RequestId const & requestId,
                                                          ErrorCode errorCode)
    : requestId_(requestId), errorCode_(errorCode)
{}


template<VerificationStage stage>
auto FailedVerificationReport<stage>::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, requestId_);
    (void)SerializeTo<ccsdsEndianness>(cursor, errorCode_);
}


template<VerificationStage stage>
auto FailedVerificationReport<stage>::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(secondaryHeader_), decltype(requestId_), decltype(errorCode_)>);
}


template class FailedVerificationReport<VerificationStage::acceptance>;
template class FailedVerificationReport<VerificationStage::completionOfExecution>;


HousekeepingParameterReport::HousekeepingParameterReport(TelemetryRecord const & record)
    : record_(record)
{}


auto HousekeepingParameterReport::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, structureId);
    (void)SerializeTo<ccsdsEndianness>(cursor, record_);
}


auto HousekeepingParameterReport::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(secondaryHeader_), decltype(structureId), decltype(record_)>);
}


ParameterValueReport::ParameterValueReport(Parameter::Id parameterId,
                                           Parameter::Value parameterValue)
    : nParameters_(1),
      parameters_({
          Parameter{parameterId, parameterValue}
})
{}


ParameterValueReport::ParameterValueReport(
    etl::vector<Parameter, maxNParameters> const & parameters)
    : nParameters_(static_cast<std::uint8_t>(parameters.size())), parameters_(parameters)
{}


auto ParameterValueReport::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, nParameters_);
    (void)SerializeTo<ccsdsEndianness>(cursor, parameters_);
}


auto ParameterValueReport::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(secondaryHeader_), decltype(nParameters_)>
        + nParameters_ * totalSerialSize<Parameter::Id, Parameter::Value>);
}


FileAttributeReport::FileAttributeReport(fs::Path const & filePath,
                                         std::uint32_t fileSize,
                                         LockState lockState)
    : filePath_(filePath), fileSize_(fileSize), lockState_(lockState)
{
    filePath_.resize(fs::Path::MAX_SIZE, '\0');
}


auto FileAttributeReport::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, filePath_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, fileSize_);
    (void)SerializeTo<ccsdsEndianness>(cursor, lockState_);
}


auto FileAttributeReport::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(totalSerialSize<decltype(secondaryHeader_),
                                                      decltype(filePath_),
                                                      decltype(fileSize_),
                                                      decltype(lockState_)>);
}


RepositoryContentSummaryReport::RepositoryContentSummaryReport(
    fs::Path const & repositoryPath,
    std::uint8_t nObjects,
    etl::vector<FileSystemObject, maxNObjectsPerPacket> const & objects)
    : repositoryPath_(repositoryPath), nObjects_(nObjects), objects_(objects)
{
    repositoryPath_.resize(fs::Path::MAX_SIZE, '\0');
    for(auto && object : objects_)
    {
        object.name.resize(fs::Path::MAX_SIZE, '\0');
    }
}


auto RepositoryContentSummaryReport::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, repositoryPath_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, nObjects_);
    for(auto && object : objects_)
    {
        cursor = SerializeTo<ccsdsEndianness>(cursor, object);
    }
}


auto RepositoryContentSummaryReport::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(secondaryHeader_), decltype(repositoryPath_), decltype(nObjects_)>
        + nObjects_ * totalSerialSize<FileSystemObject>);
}


DumpedRawMemoryDataReport::DumpedRawMemoryDataReport(
    std::uint8_t nDataBlocks,
    fram::Address startAddress,
    etl::vector<Byte, maxDumpedDataLength> const & dumpedData)
    : nDataBlocks_(nDataBlocks), startAddress_(startAddress), dumpedData_(dumpedData)
{}


auto DumpedRawMemoryDataReport::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, nDataBlocks_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, startAddress_);
    auto dumpedDataLength = static_cast<std::uint8_t>(dumpedData_.size());
    cursor = SerializeTo<ccsdsEndianness>(cursor, dumpedDataLength);
    std::ranges::copy(dumpedData_, static_cast<Byte *>(cursor));
}


auto DumpedRawMemoryDataReport::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(totalSerialSize<decltype(secondaryHeader_),
                                                      decltype(nDataBlocks_),
                                                      decltype(startAddress_),
                                                      std::uint8_t>
                                      + dumpedData_.size());
}


// --- De-/Serialization ---


template<std::endian endianness>
auto SerializeTo(void * destination, RequestId const & requestId) -> void *
{
    destination = SerializeTo<endianness>(destination,
                                          requestId.packetVersionNumber,
                                          value_of(requestId.packetType),
                                          requestId.secondaryHeaderFlag,
                                          requestId.apid.Value(),
                                          requestId.sequenceFlags,
                                          requestId.packetSequenceCount);
    return destination;
}


template auto SerializeTo<std::endian::big>(void * destination, RequestId const & requestId)
    -> void *;


template<std::endian endianness>
auto DeserializeFrom(void const * source, RequestId * requestId) -> void const *
{
    auto apidValue = Apid::ValueType{};
    source = DeserializeFrom<endianness>(source,
                                         &requestId->packetVersionNumber,
                                         &value_of(requestId->packetType),
                                         &requestId->secondaryHeaderFlag,
                                         &apidValue,
                                         &requestId->sequenceFlags,
                                         &requestId->packetSequenceCount);
    requestId->apid = Apid(apidValue);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const * source, RequestId * requestId)
    -> void const *;


// --- Private function definitions ---

namespace
{
template<tm::MessageTypeId id>
auto UpdateMessageTypeCounterAndTime(tm::SpacePacketSecondaryHeader<id> * secondaryHeader) -> void
{
    secondaryHeader->messageTypeCounter =
        messageTypeCounters.PostIncrement(secondaryHeader->messageTypeId);
    secondaryHeader->time = CurrentRealTime();
}
}
}

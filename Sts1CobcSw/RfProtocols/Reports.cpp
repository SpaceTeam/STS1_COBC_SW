#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RfProtocols/IdCounters.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>

#include <etl/string.h>

#include <algorithm>
#include <cassert>


namespace sts1cobcsw
{
namespace
{
auto messageTypeCounters = IdCounters<std::uint16_t, tm::MessageTypeId>{};


template<tm::MessageTypeId id>
auto UpdateMessageTypeCounterAndTime(tm::SpacePacketSecondaryHeader<id> * secondaryHeader) -> void;

auto IncreaseSize(etl::ivector<Byte> * dataField, std::size_t sizeIncrease) -> std::size_t;
}


// --- Public function definitions ---

template<VerificationStage stage>
SuccessfulVerificationReport<stage>::SuccessfulVerificationReport(RequestId const & requestId)
    : requestId_(requestId)
{
}


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
{
}


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
{
}


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
      parameters_(decltype(parameters_){
          Parameter{.parameterId = parameterId, .parameterValue = parameterValue}
})
{
}


ParameterValueReport::ParameterValueReport(
    etl::vector<Parameter, maxNParameters> const & parameters)
    : nParameters_(static_cast<std::uint8_t>(parameters.size())), parameters_(parameters)
{
}


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
                                         FileStatus fileStatus)
    : filePath_(filePath), fileSize_(fileSize), fileStatus_(fileStatus)
{
    filePath_.resize(fs::Path::MAX_SIZE, '\0');
}


auto FileAttributeReport::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = std::copy(filePath_.begin(), filePath_.end(), static_cast<char *>(cursor));
    cursor = SerializeTo<ccsdsEndianness>(cursor, fileSize_);
    (void)SerializeTo<ccsdsEndianness>(cursor, fileStatus_);
}


auto FileAttributeReport::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(secondaryHeader_), decltype(fileSize_), decltype(fileStatus_)>
        + fs::Path::MAX_SIZE);
}


RepositoryContentSummaryReport::RepositoryContentSummaryReport(
    fs::Path const & repositoryPath,
    std::uint8_t nObjects,
    etl::vector<ObjectType, maxNObjectsPerPacket> const &  // NOLINT(modernize-pass-by-value)
        objectTypes,
    etl::vector<fs::Path, maxNObjectsPerPacket> const &  // NOLINT(modernize-pass-by-value)
        objectNames)
    : repositoryPath_(repositoryPath),
      nObjects_(nObjects),
      objectTypes_(objectTypes),
      objectNames_(objectNames)
{
    assert(objectTypes_.size() == objectNames_.size());  // NOLINT(*array*decay)
    repositoryPath_.resize(fs::Path::MAX_SIZE, '\0');
    for(auto & objectName : objectNames_)
    {
        objectName.resize(fs::Path::MAX_SIZE, '\0');
    }
}


auto RepositoryContentSummaryReport::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = std::copy(repositoryPath_.begin(), repositoryPath_.end(), static_cast<char *>(cursor));
    cursor = SerializeTo<ccsdsEndianness>(cursor, nObjects_);
    for(auto i = 0U; i < objectTypes_.size(); ++i)
    {
        cursor = SerializeTo<ccsdsEndianness>(cursor, objectTypes_[i]);
        cursor =
            std::copy(objectNames_[i].begin(), objectNames_[i].end(), static_cast<char *>(cursor));
    }
}


auto RepositoryContentSummaryReport::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(secondaryHeader_), decltype(nObjects_)> + fs::Path::MAX_SIZE
        + objectTypes_.size() * (totalSerialSize<ObjectType> + fs::Path::MAX_SIZE));
}


DumpedRawMemoryDataReport::DumpedRawMemoryDataReport(
    std::uint8_t nDataBlocks,
    fram::Address startAddress,
    etl::vector<Byte, maxDumpedDataLength> const & dumpedData)  // NOLINT(modernize-pass-by-value)
    : nDataBlocks_(nDataBlocks), startAddress_(startAddress), dumpedData_(dumpedData)
{
}


auto DumpedRawMemoryDataReport::DoAddTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, nDataBlocks_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, startAddress_);
    auto dumpedDataLength = static_cast<std::uint8_t>(dumpedData_.size());
    cursor = SerializeTo<ccsdsEndianness>(cursor, dumpedDataLength);
    std::copy(dumpedData_.begin(), dumpedData_.end(), static_cast<Byte *>(cursor));
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
                                          requestId.packetType,
                                          requestId.secondaryHeaderFlag,
                                          requestId.apid.Value(),
                                          requestId.sequenceFlags,
                                          requestId.packetSequenceCount);
    return destination;
}


template auto SerializeTo<std::endian::big>(void * destination, RequestId const & requestId)
    -> void *;


template<std::endian endianness>
auto DeserializeFrom(void const * source, RequestId & requestId) -> void const *
{
    auto apidValue = Apid::ValueType{};
    source = DeserializeFrom<endianness>(source,
                                         &requestId.packetVersionNumber,
                                         &requestId.packetType,
                                         &requestId.secondaryHeaderFlag,
                                         &apidValue,
                                         &requestId.sequenceFlags,
                                         &requestId.packetSequenceCount);
    requestId.apid = Apid(apidValue);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const * source, RequestId & requestId)
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


auto IncreaseSize(etl::ivector<Byte> * dataField, std::size_t sizeIncrease) -> std::size_t
{
    auto oldSize = dataField->size();
    dataField->resize(oldSize + sizeIncrease);
    return oldSize;
}
}
}

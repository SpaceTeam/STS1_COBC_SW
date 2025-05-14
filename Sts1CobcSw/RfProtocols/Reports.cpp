#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RfProtocols/IdCounters.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>

#include <algorithm>
#include <cassert>
#include <utility>


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
auto SuccessfulVerificationReport<stage>::DoWriteTo(etl::ivector<Byte> * dataField) const -> void
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
auto FailedVerificationReport<stage>::DoWriteTo(etl::ivector<Byte> * dataField) const -> void
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


auto HousekeepingParameterReport::DoWriteTo(etl::ivector<Byte> * dataField) const -> void
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


ParameterValueReport::ParameterValueReport(ParameterId parameterId, ParameterValue parameterValue)
    : nParameters_(1),
      parameterIds_(decltype(parameterIds_){parameterId}),
      parameterValues_(decltype(parameterValues_){parameterValue})
{
}


ParameterValueReport::ParameterValueReport(
    etl::vector<ParameterId, maxNParameters> parameterIds,
    etl::vector<ParameterValue, maxNParameters> parameterValues)
    : nParameters_(static_cast<std::uint8_t>(parameterIds.size())),
      parameterIds_(std::move(parameterIds)),
      parameterValues_(std::move(parameterValues))
{
    assert(parameterIds_.size() == parameterValues_.size());  // NOLINT(*array*decay)
}


auto ParameterValueReport::DoWriteTo(etl::ivector<Byte> * dataField) const -> void
{
    UpdateMessageTypeCounterAndTime(&secondaryHeader_);
    auto oldSize = IncreaseSize(dataField, DoSize());
    auto * cursor = SerializeTo<ccsdsEndianness>(dataField->data() + oldSize, secondaryHeader_);
    cursor = SerializeTo<ccsdsEndianness>(cursor, nParameters_);
    for(auto i = 0U; i < nParameters_; ++i)
    {
        cursor = SerializeTo<ccsdsEndianness>(cursor, parameterIds_[i]);
        cursor = SerializeTo<ccsdsEndianness>(cursor, parameterValues_[i]);
    }
}


auto ParameterValueReport::DoSize() const -> std::uint16_t
{
    return static_cast<std::uint16_t>(
        totalSerialSize<decltype(secondaryHeader_), decltype(nParameters_)>
        + nParameters_ * totalSerialSize<ParameterId, ParameterValue>);
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

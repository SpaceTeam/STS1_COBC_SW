#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/RfProtocols/Requests.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <etl/algorithm.h>

#include <type_traits>


namespace sts1cobcsw
{

[[nodiscard]] auto ParseAsRequest(std::span<Byte const> buffer) -> Result<Request>
{
    if(buffer.size() < tc::packetSecondaryHeaderLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto request = Request{.packetSecondaryHeader = tc::SpacePacketSecondaryHeader(),
                           .applicationData = buffer.subspan(tc::packetSecondaryHeaderLength)};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(),
                                                       &request.packetSecondaryHeader);
    auto packetIsValid =
        request.packetSecondaryHeader.tcPacketPusVersionNumber == tc::packetPusVersionNumber;
    if(not packetIsValid)
    {
        return ErrorCode::invalidSpacePacket;
    }
    if(not IsValid(request.packetSecondaryHeader.messageTypeId))
    {
        return ErrorCode::invalidMessageTypeId;
    }
    if(not IsValid(request.packetSecondaryHeader.sourceId))
    {
        return ErrorCode::invalidSourceId;
    }
    return request;
}


[[nodiscard]] auto ParseAsLoadRawMemoryDataAreasRequest(std::span<Byte const> buffer)
    -> Result<LoadRawMemoryDataAreasRequest>
{
    static constexpr auto minApplicationDataLength =
        totalSerialSize<decltype(LoadRawMemoryDataAreasRequest::nDataAreas),
                        decltype(LoadRawMemoryDataAreasRequest::startAddress),
                        decltype(LoadRawMemoryDataAreasRequest::dataLength)>;
    if(buffer.size() < minApplicationDataLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto request = LoadRawMemoryDataAreasRequest{};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request);
    if(not(request.nDataAreas == 1))
    {
        return ErrorCode::invalidApplicationData;
    }
    static constexpr auto maxDataLength = tc::maxMessageDataLength - minApplicationDataLength;
    if(request.dataLength > maxDataLength)
    {
        return ErrorCode::invalidApplicationData;
    }
    if(buffer.size() != minApplicationDataLength + request.dataLength)
    {
        return ErrorCode::invalidDataLength;
    }
    request.data = buffer.subspan(minApplicationDataLength, request.dataLength);
    return request;
}


[[nodiscard]] auto ParseAsDumpRawMemoryDataRequest(std::span<Byte const> buffer)
    -> Result<DumpRawMemoryDataRequest>
{
    static constexpr auto minApplicationDataLength =
        totalSerialSize<decltype(DumpRawMemoryDataRequest::nDataAreas)>;
    if(buffer.size() < minApplicationDataLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto request = DumpRawMemoryDataRequest{};
    auto const * cursor =
        DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request.nDataAreas);
    if(request.nDataAreas > DumpRawMemoryDataRequest::maxNDataAreas)
    {
        return ErrorCode::invalidApplicationData;
    }
    request.dataAreas.resize(request.nDataAreas);
    if(buffer.size()
       != minApplicationDataLength + totalSerialSize<DumpRawMemoryDataArea> * request.nDataAreas)
    {
        return ErrorCode::invalidDataLength;
    }
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(cursor, &request.dataAreas);
    for(auto dataArea : request.dataAreas)
    {
        if(dataArea.length > maxDumpedDataLength)
        {
            return ErrorCode::invalidApplicationData;
        }
    }
    return request;
}


[[nodiscard]] auto ParseAsPerformAFunctionRequest(std::span<Byte const> buffer)
    -> Result<PerformAFunctionRequest>
{
    if(buffer.empty())
    {
        return ErrorCode::bufferTooSmall;
    }
    auto request = PerformAFunctionRequest{};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request.functionId);
    request.dataField = buffer.subspan(totalSerialSize<tc::FunctionId>);
    return request;
}


[[nodiscard]] auto ParseAsReportParameterValuesRequest(std::span<Byte const> buffer)
    -> Result<ReportParameterValuesRequest>
{
    static constexpr auto minApplicationDataLength =
        totalSerialSize<decltype(ReportParameterValuesRequest::nParameters)>;
    if(buffer.size() < minApplicationDataLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto request = ReportParameterValuesRequest{};
    auto const * cursor =
        DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request.nParameters);
    if(request.nParameters > maxNParameters)
    {
        return ErrorCode::invalidApplicationData;
    }
    if(buffer.size()
       != minApplicationDataLength + totalSerialSize<ParameterId> * request.nParameters)
    {
        return ErrorCode::invalidDataLength;
    }
    request.parameterIds.resize(request.nParameters);
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(cursor, &request.parameterIds);
    return request;
}


[[nodiscard]] auto ParseAsSetParameterValuesRequest(std::span<Byte const> buffer)
    -> Result<SetParameterValuesRequest>
{
    static constexpr auto minApplicationDataLength =
        totalSerialSize<decltype(ReportParameterValuesRequest::nParameters)>;
    if(buffer.size() < minApplicationDataLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto request = SetParameterValuesRequest{};
    auto const * cursor =
        DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request.nParameters);
    if(request.nParameters > maxNParameters)
    {
        return ErrorCode::invalidApplicationData;
    }
    if(buffer.size() != minApplicationDataLength + totalSerialSize<Parameter> * request.nParameters)
    {
        return ErrorCode::invalidDataLength;
    }
    request.parameters.resize(request.nParameters);
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(cursor, &request.parameters);
    return request;
}


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, LoadRawMemoryDataAreasRequest * header)
    -> void const *
{
    source = DeserializeFrom<endianness>(source, &header->nDataAreas);
    source = DeserializeFrom<endianness>(source, &header->startAddress);
    source = DeserializeFrom<endianness>(source, &header->dataLength);
    return source;
}


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, DumpRawMemoryDataArea * dataArea)
    -> void const *
{
    source = DeserializeFrom<endianness>(source, &dataArea->startAddress);
    source = DeserializeFrom<endianness>(source, &dataArea->length);
    return source;
}


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, Parameter * parameter) -> void const *
{
    source = DeserializeFrom<endianness>(source, &parameter->parameterId);
    source = DeserializeFrom<endianness>(source, &parameter->parameterValue);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const * source,
                                                LoadRawMemoryDataAreasRequest * header)
    -> void const *;
template auto DeserializeFrom<std::endian::big>(void const * source,
                                                DumpRawMemoryDataArea * dataArea) -> void const *;
template auto DeserializeFrom<std::endian::big>(void const * source, Parameter * parameter)
    -> void const *;
}

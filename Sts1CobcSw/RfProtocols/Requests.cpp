#include <Sts1CobcSw/RfProtocols/Requests.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>


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
    if(request.nDataAreas != 1)
    {
        return ErrorCode::invalidApplicationData;
    }
    static constexpr auto maxDataLength = tc::maxMessageDataLength - minApplicationDataLength;
    auto endAddress = request.startAddress + fram::Size(request.dataLength);
    auto dataAreaIsValid = request.dataLength <= maxDataLength
                       and request.startAddress >= framSections.Get<"testMemory">().begin
                       and endAddress < framSections.Get<"testMemory">().end;
    if(not dataAreaIsValid)
    {
        return ErrorCode::invalidDataArea;
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
            return ErrorCode::invalidDataArea;
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
       != minApplicationDataLength + totalSerialSize<Parameter::Id> * request.nParameters)
    {
        return ErrorCode::invalidDataLength;
    }
    request.parameterIds.resize(request.nParameters);
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(cursor, &request.parameterIds);
    for(auto parameterId : request.parameterIds)
    {
        if(not IsValid(parameterId))
        {
            return ErrorCode::invalidParameterId;
        }
    }
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
    for(auto parameter : request.parameters)
    {
        if(not IsValid(parameter.id))
        {
            return ErrorCode::invalidParameterId;
        }
    }
    return request;
}


[[nodiscard]] auto ParseAsDeleteAFileRequest(std::span<Byte const> buffer)
    -> Result<DeleteAFileRequest>
{
    auto request = DeleteAFileRequest{};
    if(buffer.size() < request.filePath.capacity())
    {
        return ErrorCode::bufferTooSmall;
    }
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request.filePath);
    if(request.filePath.empty())
    {
        return ErrorCode::emptyFilePath;
    }
    return request;
}


[[nodiscard]] auto ParseAsReportTheAttributesOfAFileRequest(std::span<Byte const> buffer)
    -> Result<ReportTheAttributesOfAFileRequest>
{
    auto request = ReportTheAttributesOfAFileRequest{};
    if(buffer.size() < request.filePath.capacity())
    {
        return ErrorCode::bufferTooSmall;
    }
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request.filePath);
    if(request.filePath.empty())
    {
        return ErrorCode::emptyFilePath;
    }
    return request;
}


[[nodiscard]] auto ParseAsSummaryReportTheContentOfARepositoryRequest(std::span<Byte const> buffer)
    -> Result<SummaryReportTheContentOfARepositoryRequest>

{
    auto request = SummaryReportTheContentOfARepositoryRequest{};
    if(buffer.size() < request.repositoryPath.capacity())
    {
        return ErrorCode::bufferTooSmall;
    }
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request.repositoryPath);
    if(request.repositoryPath.empty())
    {
        return ErrorCode::emptyFilePath;
    }
    return request;
}


[[nodiscard]] auto ParseAsCopyAFileRequest(std::span<Byte const> buffer) -> Result<CopyAFileRequest>
{
    auto request = CopyAFileRequest{};
    static constexpr auto applicationDataLength =
        totalSerialSize<decltype(CopyAFileRequest::operationId),
                        decltype(CopyAFileRequest::sourceFilePath),
                        decltype(CopyAFileRequest::targetFilePath)>;
    if(buffer.size() != applicationDataLength)
    {
        return ErrorCode::invalidDataLength;
    }
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &request);
    if(not IsValid(request.operationId))
    {
        return ErrorCode::invalidApplicationData;
    }
    if(request.sourceFilePath.empty() or request.targetFilePath.empty())
    {
        return ErrorCode::emptyFilePath;
    }
    return request;
}


[[nodiscard]] auto ParseAsReportHousekeepingParameterReportFunction(std::span<Byte const> buffer)
    -> Result<ReportHousekeepingParameterReportFunction>
{
    if(buffer.size()
       != totalSerialSize<decltype(ReportHousekeepingParameterReportFunction::firstReportIndex),
                          decltype(ReportHousekeepingParameterReportFunction::lastReportIndex)>)
    {
        return ErrorCode::invalidDataLength;
    }
    auto function = ReportHousekeepingParameterReportFunction{};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &function);
    if(function.firstReportIndex > function.lastReportIndex)
    {
        return ErrorCode::invalidApplicationData;
    }
    return function;
}


[[nodiscard]] auto ParseAsEnableFileTransferFunction(std::span<Byte const> buffer)
    -> Result<EnableFileTransferFunction>
{
    if(buffer.size() != totalSerialSize<decltype(EnableFileTransferFunction::durationInS)>)
    {
        return ErrorCode::invalidDataLength;
    }
    auto function = EnableFileTransferFunction{};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &function.durationInS);
    return function;
}


[[nodiscard]] auto ParseAsUpdateEduQueueFunction(std::span<Byte const> buffer)
    -> Result<UpdateEduQueueFunction>
{
    static constexpr auto minApplicationDataLength =
        totalSerialSize<decltype(UpdateEduQueueFunction::nQueueEntries)>;
    if(buffer.size() < minApplicationDataLength)
    {
        return ErrorCode::bufferTooSmall;
    }
    auto function = UpdateEduQueueFunction{};
    auto const * cursor =
        DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &function.nQueueEntries);
    if(function.nQueueEntries > UpdateEduQueueFunction::maxNQueueEntries)
    {
        return ErrorCode::invalidApplicationData;
    }
    if(buffer.size()
       != minApplicationDataLength
              + totalSerialSize<edu::ProgramQueueEntry> * function.nQueueEntries)
    {
        return ErrorCode::invalidDataLength;
    }
    function.queueEntries.resize(function.nQueueEntries);
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(cursor, &function.queueEntries);
    return function;
}


[[nodiscard]] auto ParseAsSetActiveFirmwareFunction(std::span<Byte const> buffer)
    -> Result<SetActiveFirmwareFunction>
{
    if(buffer.size() != totalSerialSize<decltype(SetActiveFirmwareFunction::partitionId)>)
    {
        return ErrorCode::invalidDataLength;
    }
    auto function = SetActiveFirmwareFunction{};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &function.partitionId);
    if(function.partitionId != tc::FirmwarePartitionId::secondary1
       and function.partitionId != tc::FirmwarePartitionId::secondary2)
    {
        return ErrorCode::invalidApplicationData;
    }
    return function;
}


[[nodiscard]] auto ParseAsSetBackupFirmwareFunction(std::span<Byte const> buffer)
    -> Result<SetBackupFirmwareFunction>
{
    if(buffer.size() != totalSerialSize<decltype(SetBackupFirmwareFunction::partitionId)>)
    {
        return ErrorCode::invalidDataLength;
    }
    auto function = SetBackupFirmwareFunction{};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &function.partitionId);
    if(function.partitionId != tc::FirmwarePartitionId::secondary1
       && function.partitionId != tc::FirmwarePartitionId::secondary2)
    {
        return ErrorCode::invalidApplicationData;
    }
    return function;
}


[[nodiscard]] auto ParseAsCheckFirmwareIntegrityFunction(std::span<Byte const> buffer)
    -> Result<CheckFirmwareIntegrityFunction>
{
    if(buffer.size() != totalSerialSize<decltype(CheckFirmwareIntegrityFunction::partitionId)>)
    {
        return ErrorCode::invalidDataLength;
    }
    auto function = CheckFirmwareIntegrityFunction{};
    (void)DeserializeFrom<sts1cobcsw::ccsdsEndianness>(buffer.data(), &function.partitionId);
    if(function.partitionId != tc::FirmwarePartitionId::primary
       && function.partitionId != tc::FirmwarePartitionId::secondary1
       && function.partitionId != tc::FirmwarePartitionId::secondary2)
    {
        return ErrorCode::invalidApplicationData;
    }
    return function;
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
[[nodiscard]] auto DeserializeFrom(void const * source, CopyAFileRequest * header) -> void const *
{
    source = DeserializeFrom<endianness>(source, &header->operationId);
    source = DeserializeFrom<endianness>(source, &header->sourceFilePath);
    source = DeserializeFrom<endianness>(source, &header->targetFilePath);
    return source;
}


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source,
                                   ReportHousekeepingParameterReportFunction * function)
    -> void const *
{
    source = DeserializeFrom<endianness>(source, &function->firstReportIndex);
    source = DeserializeFrom<endianness>(source, &function->lastReportIndex);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const * source,
                                                LoadRawMemoryDataAreasRequest * header)
    -> void const *;
template auto DeserializeFrom<std::endian::big>(void const * source,
                                                DumpRawMemoryDataArea * dataArea) -> void const *;
template auto DeserializeFrom<std::endian::big>(void const * source, CopyAFileRequest * header)
    -> void const *;
template auto DeserializeFrom<std::endian::big>(
    void const * source, ReportHousekeepingParameterReportFunction * function) -> void const *;
}

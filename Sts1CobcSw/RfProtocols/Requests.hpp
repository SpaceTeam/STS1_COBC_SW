#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <etl/utility.h>
#include <etl/vector.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
struct Request
{
    tc::SpacePacketSecondaryHeader packetSecondaryHeader;
    std::span<Byte const> applicationData;
};


struct LoadRawMemoryDataAreasRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{6, 2}>();
    std::uint8_t nDataAreas;
    fram::Address startAddress;
    std::uint8_t dataLength;
    std::span<Byte const> data;
};


struct DumpRawMemoryDataArea
{
    fram::Address startAddress;
    std::uint8_t length = 0U;
};


template<>
inline constexpr std::size_t serialSize<DumpRawMemoryDataArea> =
    totalSerialSize<decltype(DumpRawMemoryDataArea::startAddress),
                    decltype(DumpRawMemoryDataArea::length)>;


struct DumpRawMemoryDataRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{6, 5}>();
    static constexpr auto maxNDataAreas = (tc::maxMessageDataLength - totalSerialSize<std::uint8_t>)
                                        / totalSerialSize<DumpRawMemoryDataArea>;
    std::uint8_t nDataAreas;
    etl::vector<DumpRawMemoryDataArea, maxNDataAreas> dataAreas;
};


struct PerformAFunctionRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{8, 1}>();
    tc::FunctionId functionId;
    std::span<Byte const> dataField;
};


struct ReportParameterValuesRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{20, 1}>();
    std::uint8_t nParameters;
    etl::vector<ParameterId, maxNParameters> parameterIds;
};


struct SetParameterValuesRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{20, 3}>();
    std::uint8_t nParameters;
    etl::vector<Parameter, maxNParameters> parameters;
};


struct DeleteAFileRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{23, 2}>();
    fs::Path filePath;
};


struct ReportTheAttributesOfAFileRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{23, 3}>();
    fs::Path filePath;
};


struct SummaryReportTheContentOfARepositoryRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{23, 12}>();
    fs::Path repositoryPath;
};


struct CopyAFileRequest
{
    static constexpr auto id = Make<tc::MessageTypeId, MessageTypeIdFields{23, 14}>();
    CopyOperationId operationId;
    fs::Path sourceFilePath;
    fs::Path targetFilePath;
};


[[nodiscard]] auto ParseAsRequest(std::span<Byte const> buffer) -> Result<Request>;

[[nodiscard]] auto ParseAsLoadRawMemoryDataAreasRequest(std::span<Byte const> buffer)
    -> Result<LoadRawMemoryDataAreasRequest>;
[[nodiscard]] auto ParseAsDumpRawMemoryDataRequest(std::span<Byte const> buffer)
    -> Result<DumpRawMemoryDataRequest>;
[[nodiscard]] auto ParseAsPerformAFunctionRequest(std::span<Byte const> buffer)
    -> Result<PerformAFunctionRequest>;
[[nodiscard]] auto ParseAsReportParameterValuesRequest(std::span<Byte const> buffer)
    -> Result<ReportParameterValuesRequest>;
[[nodiscard]] auto ParseAsSetParameterValuesRequest(std::span<Byte const> buffer)
    -> Result<SetParameterValuesRequest>;
[[nodiscard]] auto ParseAsDeleteAFileRequest(std::span<Byte const> buffer)
    -> Result<DeleteAFileRequest>;
[[nodiscard]] auto ParseAsReportTheAttributesOfAFileRequest(std::span<Byte const> buffer)
    -> Result<ReportTheAttributesOfAFileRequest>;
[[nodiscard]] auto ParseAsSummaryReportTheContentOfARepositoryRequest(std::span<Byte const> buffer)
    -> Result<SummaryReportTheContentOfARepositoryRequest>;
[[nodiscard]] auto ParseAsCopyAFileRequest(std::span<Byte const> buffer)
    -> Result<CopyAFileRequest>;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, LoadRawMemoryDataAreasRequest * header)
    -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, DumpRawMemoryDataArea * dataArea)
    -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, Parameter * parameter) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, CopyAFileRequest * header) -> void const *;
}

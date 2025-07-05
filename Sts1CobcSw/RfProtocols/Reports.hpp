#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/SpacePacket.hpp>
#include <Sts1CobcSw/RfProtocols/TmSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>

#include <strong_type/type.hpp>

#include <etl/vector.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>


namespace sts1cobcsw
{
struct RequestId
{
    UInt<3> packetVersionNumber;
    PacketType packetType;
    UInt<1> secondaryHeaderFlag;
    Apid apid;
    UInt<2> sequenceFlags;
    UInt<14> packetSequenceCount;  // NOLINT(*magic-numbers)
};


// The values are chosen to match the subtype IDs of the failed verification reports. The IDs of the
// successful ones are one less. The implementations of the reports rely on this.
enum class VerificationStage : std::uint8_t
{
    acceptance = 2,
    completionOfExecution = 8,
};


// --- Reports ---

template<VerificationStage stage>
class SuccessfulVerificationReport : public Payload
{
public:
    static constexpr auto verificationStage = stage;

    explicit SuccessfulVerificationReport(RequestId const & requestId);


private:
    static constexpr auto messageTypeId =
        Make<tm::MessageTypeId, {1, static_cast<int>(stage) - 1}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    RequestId requestId_;

    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


using SuccessfulAcceptanceVerificationReport =
    SuccessfulVerificationReport<VerificationStage::acceptance>;
using SuccessfulCompletionOfExecutionVerificationReport =
    SuccessfulVerificationReport<VerificationStage::completionOfExecution>;


template<VerificationStage stage>
class FailedVerificationReport : public Payload
{
public:
    static constexpr auto verificationStage = stage;

    FailedVerificationReport(RequestId const & requestId, ErrorCode errorCode);


private:
    static constexpr auto messageTypeId = Make<tm::MessageTypeId, {1, static_cast<int>(stage)}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    RequestId requestId_;
    ErrorCode errorCode_;

    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


using FailedAcceptanceVerificationReport = FailedVerificationReport<VerificationStage::acceptance>;
using FailedCompletionOfExecutionVerificationReport =
    FailedVerificationReport<VerificationStage::completionOfExecution>;


class HousekeepingParameterReport : public Payload
{
public:
    explicit HousekeepingParameterReport(TelemetryRecord const & record);


private:
    static constexpr auto messageTypeId = Make<tm::MessageTypeId, {3, 25}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    static constexpr std::uint8_t structureId = 0;
    TelemetryRecord record_;

    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


class ParameterValueReport : public Payload
{
public:
    ParameterValueReport(Parameter::Id parameterId, Parameter::Value parameterValue);
    explicit ParameterValueReport(etl::vector<Parameter, maxNParameters> const & parameters);


private:
    static constexpr auto messageTypeId = Make<tm::MessageTypeId, {20, 2}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    std::uint8_t nParameters_ = 0;
    etl::vector<Parameter, maxNParameters> parameters_;

    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


class FileAttributeReport : public Payload
{
public:
    FileAttributeReport(fs::Path const & filePath, std::uint32_t fileSize, FileStatus fileStatus);


private:
    static constexpr auto messageTypeId = Make<tm::MessageTypeId, {23, 4}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    fs::Path filePath_;
    std::uint32_t fileSize_;
    FileStatus fileStatus_;

    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


class RepositoryContentSummaryReport : public Payload
{
public:
    static constexpr auto maxNObjectsPerPacket =
        (tm::maxPacketDataLength - tm::packetSecondaryHeaderLength
         - totalSerialSize<fs::Path, std::uint8_t>)
        / (totalSerialSize<FileSystemObject>);

    RepositoryContentSummaryReport(
        fs::Path const & repositoryPath,
        std::uint8_t nObjects,
        etl::vector<FileSystemObject, maxNObjectsPerPacket> const & objects);


private:
    static constexpr auto messageTypeId = Make<tm::MessageTypeId, {23, 13}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    fs::Path repositoryPath_;
    std::uint8_t nObjects_ = 0;
    etl::vector<FileSystemObject, maxNObjectsPerPacket> objects_;

    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


class DumpedRawMemoryDataReport : public Payload
{
public:
    DumpedRawMemoryDataReport(std::uint8_t nDataBlocks,
                              fram::Address startAddress,
                              etl::vector<Byte, maxDumpedDataLength> const & dumpedData);


private:
    static constexpr auto messageTypeId = Make<tm::MessageTypeId, {6, 6}>();
    mutable tm::SpacePacketSecondaryHeader<messageTypeId> secondaryHeader_;
    std::uint8_t nDataBlocks_ = 0;
    fram::Address startAddress_ = fram::Address(0);
    etl::vector<Byte, maxDumpedDataLength> dumpedData_;


    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


// --- De-/Serialization ---

template<>
inline constexpr std::size_t serialSize<RequestId> =
    totalSerialSize<decltype(RequestId::packetVersionNumber),
                    strong::underlying_type_t<PacketType>,
                    decltype(RequestId::secondaryHeaderFlag),
                    decltype(RequestId::apid.Value()),
                    decltype(RequestId::sequenceFlags),
                    decltype(RequestId::packetSequenceCount)>;


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, RequestId const & requestId) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, RequestId * requestId) -> void const *;
}

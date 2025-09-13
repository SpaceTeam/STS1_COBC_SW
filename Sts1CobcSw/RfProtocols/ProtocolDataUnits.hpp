#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <strong_type/regular.hpp>
#include <strong_type/type.hpp>

#include <etl/utility.h>
#include <etl/vector.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>


namespace sts1cobcsw
{
namespace tc
{
struct ProtocolDataUnit
{
    using Header = ProtocolDataUnitHeader;

    Header header;
    etl::vector<Byte, tc::maxPduDataLength> dataField;
};
}


class FileDataPdu : public Payload
{
public:
    static constexpr auto maxFileDataLength = 205U;

    FileDataPdu() = default;
    explicit FileDataPdu(std::uint32_t offset, std::span<Byte const> fileData);

    std::uint32_t offset = 0;
    std::span<Byte const> fileData;


private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


enum class DirectiveCode : std::uint8_t
{
    endOfFile = 4,
    finished = 5,
    ack = 6,
    metadata = 7,
    nak = 8,
};


struct FileDirectivePdu
{
    DirectiveCode directiveCode = DirectiveCode::endOfFile;
    std::span<Byte const> parameterField;
};


using ConditionCode = strong::type<UInt<4>, struct ConditionCodeTag, strong::regular>;


enum TlvType : std::uint8_t
{
    entityId = 6,
};


struct FaultLocation
{
    TlvType type = TlvType::entityId;
    std::uint8_t length = 0;
    EntityId value;
};


class EndOfFilePdu : public Payload
{
public:
    static constexpr auto directiveCode = DirectiveCode::endOfFile;

    ConditionCode conditionCode = ConditionCode(0);
    UInt<4> spare;
    std::uint32_t fileChecksum = 0;
    std::uint32_t fileSize = 0;
    FaultLocation faultLocation;  // omitted if conditionCode == noError

    static constexpr auto minParameterFieldLength =
        totalSerialSize<strong::underlying_type_t<ConditionCode>, decltype(spare)>
        + totalSerialSize<decltype(fileChecksum), decltype(fileSize)>;


private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


using DeliveryCode = strong::type<UInt<1>, struct DeliveryCodeTag, strong::regular>;
using FileStatus = strong::type<UInt<2>, struct FileStatusTag, strong::regular>;


class FinishedPdu : public Payload
{
public:
    static constexpr auto directiveCode = DirectiveCode::finished;

    ConditionCode conditionCode = ConditionCode(0);
    UInt<1> spare = 0;
    DeliveryCode deliveryCode = DeliveryCode(0);
    FileStatus fileStatus = FileStatus(0);
    FaultLocation faultLocation;  // omitted if conditionCode == noError or unsupportedChecksumType

    static constexpr auto minParameterFieldLength =
        totalSerialSize<strong::underlying_type_t<ConditionCode>,
                        decltype(spare),
                        strong::underlying_type_t<DeliveryCode>,
                        strong::underlying_type_t<FileStatus>>;

private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};

using TransactionStatus = strong::type<UInt<2>, struct TransactionStatusTag, strong::regular>;


class AckPdu : public Payload
{
public:
    static constexpr auto directiveCode = DirectiveCode::ack;

    AckPdu() = default;
    explicit AckPdu(DirectiveCode acknowledgedDirectiveCode,
                    ConditionCode conditionCode,
                    TransactionStatus transactionStatus) noexcept;

    UInt<4> acknowledgedPduDirectiveCode;  // EOF or Finished PDU
    UInt<4> directiveSubtypeCode;          // 1 if Finished PDU is acknowledged, 0 otherwise
    ConditionCode conditionCode;
    UInt<2> spare = 0;
    TransactionStatus transactionStatus;

    static constexpr auto minParameterFieldLength =
        totalSerialSize<decltype(acknowledgedPduDirectiveCode),
                        decltype(directiveSubtypeCode),
                        strong::underlying_type_t<ConditionCode>,
                        decltype(spare),
                        strong::underlying_type_t<TransactionStatus>>;


private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


class MetadataPdu : public Payload
{
public:
    static constexpr auto directiveCode = DirectiveCode::metadata;

    MetadataPdu() = default;
    explicit MetadataPdu(std::uint32_t fileSize,
                         std::span<Byte const> sourceFileName,
                         std::span<Byte const> destinationFileName) noexcept;

    UInt<1> reserved1 = 0;
    UInt<1> closureRequestd = 0;  // 0 in ACK mode
    UInt<4> checksumType = 15;    // NOLINT(*-magic-numbers)
    UInt<2> reserved2 = 0;
    std::uint32_t fileSize;
    std::uint8_t sourceFileNameLength;
    std::span<Byte const> sourceFileNameValue;
    std::uint8_t destinationFileNameLength;
    std::span<Byte const> destinationFileNameValue;

    static constexpr auto minParameterFieldLength =
        totalSerialSize<decltype(reserved1),
                        decltype(closureRequestd),
                        decltype(checksumType),
                        decltype(reserved2)>
        + totalSerialSize<decltype(fileSize), decltype(sourceFileNameLength)>;


private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


class NakPdu : public Payload
{
public:
    static constexpr auto directiveCode = DirectiveCode::nak;
    static constexpr auto maxSegmentRequests = 25U;

    NakPdu() = default;
    explicit NakPdu(std::uint32_t endOfScope,
                    std::span<std::uint64_t const> segementRequests) noexcept;

    std::uint32_t startOfScope;
    std::uint32_t endOfScope;
    std::span<std::uint64_t const> segmentRequests;


private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


inline constexpr auto noErrorConditionCode = ConditionCode(0);
inline constexpr auto positiveAckLimitReachedConditionCode = ConditionCode(1);
inline constexpr auto keepAliveLimitReachedConditionCode = ConditionCode(2);
inline constexpr auto invalidTransmissionModeConditionCode = ConditionCode(3);
inline constexpr auto filestoreRejectionConditionCode = ConditionCode(4);
inline constexpr auto fileChecksumFailureConditionCode = ConditionCode(5);
inline constexpr auto fileSizeErrorConditionCode = ConditionCode(6);
inline constexpr auto nackLimitReachedConditionCode = ConditionCode(7);
inline constexpr auto inactivityDetectedConditionCode = ConditionCode(8);
inline constexpr auto invalidFileStructureConditionCode = ConditionCode(9);
inline constexpr auto checkLimitReachedConditionCode = ConditionCode(10);
inline constexpr auto unsupportedChecksumTypeConditionCode = ConditionCode(11);
inline constexpr auto reserved1ConditionCode = ConditionCode(12);
inline constexpr auto reserved2ConditionCode = ConditionCode(13);
inline constexpr auto suspendRequestReceivedConditionCode = ConditionCode(14);
inline constexpr auto cancelRequestReceivedConditionCode = ConditionCode(15);

inline constexpr auto dataCompleteDeliveryCode = DeliveryCode(0);
inline constexpr auto dataIncompleteDeliveryCode = DeliveryCode(1);

inline constexpr auto fileDiscardedFileStatus = FileStatus(0b00);
inline constexpr auto fileRejectedFileStatus = FileStatus(0b01);
inline constexpr auto fileRetainedFileStatus = FileStatus(0b10);
inline constexpr auto unreportedFileStatus = FileStatus(0b11);

inline constexpr auto undefinedTransactionStatus = TransactionStatus(0b00);
inline constexpr auto activeTransactionStatus = TransactionStatus(0b01);
inline constexpr auto terminatedTransactionStatus = TransactionStatus(0b10);
inline constexpr auto unrecognizedTransactionStatus = TransactionStatus(0b11);


[[nodiscard]] auto ParseAsProtocolDataUnit(std::span<Byte const> buffer)
    -> Result<tc::ProtocolDataUnit>;
[[nodiscard]] auto ParseAsFileDataPdu(std::span<Byte const> buffer) -> Result<FileDataPdu>;
[[nodiscard]] auto ParseAsFileDirectivePdu(std::span<Byte const> buffer)
    -> Result<FileDirectivePdu>;
[[nodiscard]] auto ParseAsEndOfFilePdu(std::span<Byte const> buffer) -> Result<EndOfFilePdu>;
[[nodiscard]] auto ParseAsFinishedPdu(std::span<Byte const> buffer) -> Result<FinishedPdu>;
[[nodiscard]] auto ParseAsAckPdu(std::span<Byte const> buffer) -> Result<AckPdu>;
[[nodiscard]] auto ParseAsMetadataPdu(std::span<Byte const> buffer) -> Result<MetadataPdu>;
[[nodiscard]] auto ParseAsNakPdu(std::span<Byte const> buffer) -> Result<NakPdu>;


[[nodiscard]] auto IsValid(DirectiveCode directiveCode) -> bool;


// --- De-/Serialization ---

template<>
inline constexpr std::size_t serialSize<FaultLocation> =
    totalSerialSize<decltype(FaultLocation::type),
                    decltype(FaultLocation::length),
                    decltype(FaultLocation::value)>;


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, FaultLocation const & faultLocation) -> void *;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, FaultLocation * faultLocation)
    -> void const *;
}

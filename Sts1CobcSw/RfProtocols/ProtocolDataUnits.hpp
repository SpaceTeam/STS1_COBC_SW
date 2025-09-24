#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <strong_type/regular.hpp>
#include <strong_type/type.hpp>

#include <etl/utility.h>
#include <etl/vector.h>

#include <algorithm>
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
    FileDataPdu() = default;
    explicit FileDataPdu(std::uint32_t offset, std::span<Byte const> fileData);

    std::uint32_t offset_ = 0;        // NOLINT(readability-identifier-naming)
    std::span<Byte const> fileData_;  // NOLINT(readability-identifier-naming)


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

    EndOfFilePdu() = default;
    explicit EndOfFilePdu(std::uint32_t fileSize) noexcept;
    explicit EndOfFilePdu(ConditionCode conditionCode, std::uint32_t fileSize) noexcept;

    // NOLINTBEGIN(readability-identifier-naming)
    ConditionCode conditionCode_;
    UInt<4> spare_;
    std::uint32_t fileChecksum_ = 0;
    std::uint32_t fileSize_ = 0;
    // Omitted if conditionCode == noError
    FaultLocation faultLocation_ = FaultLocation{.value = cubeSatEntityId};
    // NOLINTEND(readability-identifier-naming)

    static constexpr auto minParameterFieldLength =
        totalSerialSize<strong::underlying_type_t<ConditionCode>, decltype(spare_)>
        + totalSerialSize<decltype(fileChecksum_), decltype(fileSize_)>;


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

    FinishedPdu() = default;
    explicit FinishedPdu(DeliveryCode deliveryCode, FileStatus fileStatus) noexcept;
    explicit FinishedPdu(ConditionCode conditionCode,
                         DeliveryCode deliveryCode,
                         FileStatus fileStatus) noexcept;

    // NOLINTBEGIN(readability-identifier-naming)
    ConditionCode conditionCode_;
    UInt<1> spare_;
    DeliveryCode deliveryCode_;
    FileStatus fileStatus_;
    // Omitted if conditionCode == noError or unsupportedChecksumType
    FaultLocation faultLocation_ = FaultLocation{.value = cubeSatEntityId};
    // NOLINTEND(readability-identifier-naming)

    static constexpr auto minParameterFieldLength =
        totalSerialSize<strong::underlying_type_t<ConditionCode>,
                        decltype(spare_),
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

    // NOLINTBEGIN(readability-identifier-naming)
    UInt<4> acknowledgedPduDirectiveCode_;  // EOF or Finished PDU
    UInt<4> directiveSubtypeCode_;          // 1 if Finished PDU is acknowledged, 0 otherwise
    ConditionCode conditionCode_;
    UInt<2> spare_;
    TransactionStatus transactionStatus_;
    // NOLINTEND(readability-identifier-naming)

    static constexpr auto minParameterFieldLength =
        totalSerialSize<decltype(acknowledgedPduDirectiveCode_),
                        decltype(directiveSubtypeCode_),
                        strong::underlying_type_t<ConditionCode>,
                        decltype(spare_),
                        strong::underlying_type_t<TransactionStatus>>;


private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


using ChecksumType = Id<UInt<4>, 15>;  // NOLINT(*-magic-numbers)
inline constexpr auto nullChecksumType = Make<ChecksumType, 15>();


class MetadataPdu : public Payload
{
public:
    static constexpr auto directiveCode = DirectiveCode::metadata;

    MetadataPdu() = default;
    explicit MetadataPdu(std::uint32_t fileSize,
                         fs::Path const & sourceFileName,
                         fs::Path const & destinationFileName) noexcept;

    // NOLINTBEGIN(readability-identifier-naming)
    UInt<1> reserved1_ = 0;
    UInt<1> closureRequested_ = 0;  // 0 in ACK mode
    UInt<2> reserved2_ = 0;
    ChecksumType checksumType_ = nullChecksumType;
    std::uint32_t fileSize_ = 0;
    std::uint8_t sourceFileNameLength_ = 0;
    fs::Path sourceFileNameValue_;
    std::uint8_t destinationFileNameLength_ = 0;
    fs::Path destinationFileNameValue_;
    // NOLINTEND(readability-identifier-naming)

    static constexpr auto minParameterFieldLength =
        totalSerialSize<decltype(reserved1_),
                        decltype(closureRequested_),
                        decltype(reserved2_),
                        ChecksumType::ValueType>
        + totalSerialSize<decltype(fileSize_), decltype(sourceFileNameLength_)>;


private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


struct SegmentRequest
{
    std::uint32_t startOffset = 0;
    std::uint32_t endOffset = 0;
};


template<>
inline constexpr std::size_t serialSize<SegmentRequest> =
    totalSerialSize<decltype(SegmentRequest::startOffset), decltype(SegmentRequest::endOffset)>;


class NakPdu : public Payload
{
public:
    static constexpr auto directiveCode = DirectiveCode::nak;
    static constexpr auto maxNSegmentRequests =
        (std::min(tm::maxPduDataLength, tc::maxPduDataLength)
         - totalSerialSize<DirectiveCode, std::uint32_t, std::uint32_t>)
        / totalSerialSize<SegmentRequest>;

    NakPdu() = default;
    explicit NakPdu(
        etl::vector<SegmentRequest, maxNSegmentRequests> const & segmentRequests) noexcept;

    // NOLINTBEGIN(readability-identifier-naming)
    std::uint32_t startOfScope_ = 0;
    std::uint32_t endOfScope_ = 0;
    etl::vector<SegmentRequest, maxNSegmentRequests> segmentRequests_;
    // NOLINTEND(readability-identifier-naming)


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


[[nodiscard]] auto AddPduTo(etl::ivector<Byte> * dataField,
                            PduType pduType,
                            EntityId sourceEntityId,
                            std::uint16_t transactionSequenceNumber,
                            Payload const & payload) -> Result<void>;

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

template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, SegmentRequest const & segmentRequest) -> void *;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, SegmentRequest * segmentRequest)
    -> void const *;
}

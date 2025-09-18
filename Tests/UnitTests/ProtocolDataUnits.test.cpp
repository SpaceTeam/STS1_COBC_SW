#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnits.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <strong_type/equality.hpp>
#include <strong_type/type.hpp>

#include <etl/string.h>
#include <etl/utility.h>
#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::operator""_b;


TEST_CASE("Parsing ProtocolDataUnit")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    buffer.resize(sts1cobcsw::pduHeaderLength);
    buffer[0] = 0b0010'0100_b;  // Version, PDU type, direction, transmission mode, CRC flag, large
                                // file flag
    buffer[1] = 0x00_b;         // PDU data field length (high byte)
    buffer[2] = 0x01_b;         // PDU data field length (low byte)
    buffer[3] = 0b0000'0001_b;  // Segmentation control, length of entity IDs, segment metadata
                                // flag, length of transaction sequence number
    buffer[4] = 0x0F_b;         // Source entity ID
    buffer[5] = 0x12_b;         // Transaction sequence number (high byte)
    buffer[6] = 0x34_b;         // Transaction sequence number (low byte)
    buffer[7] = 0xF0_b;         // Destination entity ID
    buffer.push_back(0xAB_b);   // Data field
    auto parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    REQUIRE(parseResult.has_value());
    auto & pdu = parseResult.value();
    CHECK(pdu.header.version == sts1cobcsw::pduVersion);
    CHECK(pdu.header.pduType == sts1cobcsw::fileDirectivePduType);
    CHECK(pdu.header.direction == sts1cobcsw::towardsFileReceiverDirection);
    CHECK(pdu.header.transmissionMode == sts1cobcsw::acknowledgedTransmissionMode);
    CHECK(pdu.header.crcFlag == 0);
    CHECK(pdu.header.largeFileFlag == 0);
    CHECK(pdu.header.pduDataFieldLength == 1);
    CHECK(pdu.header.segmentationControl == 0);
    CHECK(pdu.header.lengthOfEntityIds == sts1cobcsw::totalSerialSize<sts1cobcsw::EntityId> - 1);
    CHECK(pdu.header.segmentMetadataFlag == 0);
    CHECK(pdu.header.lengthOfTransactionSequenceNumber
          == sts1cobcsw::totalSerialSize<decltype(pdu.header.transactionSequenceNumber)> - 1);
    CHECK(pdu.header.sourceEntityId == sts1cobcsw::groundStationEntityId);
    CHECK(pdu.header.transactionSequenceNumber == 0x1234);
    CHECK(pdu.header.destinationEntityId == sts1cobcsw::cubeSatEntityId);
    CHECK(pdu.dataField.size() == 1U);
    CHECK(pdu.dataField[0] == 0xAB_b);

    buffer[0] ^= 0xE0_b;  // Invalid version
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidProtocolDataUnit);
    buffer[0] ^= 0xE0_b;

    buffer[0] ^= 0x04_b;  // Invalid transmission mode
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidProtocolDataUnit);
    buffer[0] ^= 0x04_b;

    buffer[0] ^= 0x02_b;  // Invalid CRC flag
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidProtocolDataUnit);
    buffer[0] ^= 0x02_b;

    buffer[0] ^= 0x01_b;  // Invalid large file flag
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidProtocolDataUnit);
    buffer[0] ^= 0x01_b;

    buffer[3] ^= 0x80_b;  // Invalid segmentation control
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidProtocolDataUnit);
    buffer[3] ^= 0x80_b;

    buffer[3] ^= 0x70_b;  // Invalid length of entity IDs
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidProtocolDataUnit);
    buffer[3] ^= 0x70_b;

    buffer[3] ^= 0x08_b;  // Invalid segment metadata flag
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidProtocolDataUnit);
    buffer[3] ^= 0x08_b;

    buffer[3] ^= 0x07_b;  // Invalid length of transaction sequence number
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidProtocolDataUnit);
    buffer[3] ^= 0x07_b;

    buffer[2] = Byte{sts1cobcsw::tc::maxPduDataLength + 1U};  // Invalid PDU data field length
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidPduDataLength);
    buffer[2] = 0x01_b;

    buffer[4] = 0x00_b;  // Invalid source entity ID
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidEntityId);
    buffer[4] = 0x0F_b;

    buffer[7] = 0x00_b;  // Invalid destination entity ID
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidEntityId);
    buffer[7] = 0xF0_b;

    buffer[7] = buffer[4];  // Source and destination entity IDs must not be equal
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidEntityId);
    buffer[7] = 0xF0_b;

    buffer.resize(sts1cobcsw::pduHeaderLength - 1);  // Buffer too small
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("FileDataPdu Constructor")
{
    static constexpr auto fileData = std::array{0xAB_b, 0xCD_b, 0xEF_b};
    auto fileDataPdu = sts1cobcsw::FileDataPdu(0x1234'5678U, fileData);

    CHECK(fileDataPdu.offset_ == 0x1234'5678U);
    CHECK(fileDataPdu.fileData_.size() == 3U);
    CHECK(fileDataPdu.fileData_[0] == 0xAB_b);
    CHECK(fileDataPdu.fileData_[1] == 0xCD_b);
    CHECK(fileDataPdu.fileData_[2] == 0xEF_b);

    // Serialization
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto addResult = fileDataPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == fileDataPdu.Size());
    CHECK(dataField[0] == 0x12_b);
    CHECK(dataField[1] == 0x34_b);
    CHECK(dataField[2] == 0x56_b);
    CHECK(dataField[3] == 0x78_b);
    CHECK(dataField[4] == 0xAB_b);  // file data
    CHECK(dataField[5] == 0xCD_b);
    CHECK(dataField[6] == 0xEF_b);

    // Test with maximum allowed file data length (205 bytes)
    auto maxFileData = etl::vector<Byte, sts1cobcsw::maxFileSegmentLength>{};
    maxFileData.resize(sts1cobcsw::maxFileSegmentLength);
    std::fill(maxFileData.begin(), maxFileData.end(), 0xFF_b);

    auto maxFileDataPdu = sts1cobcsw::FileDataPdu(0, maxFileData);
    CHECK(maxFileDataPdu.fileData_.size() == sts1cobcsw::maxFileSegmentLength);

    // Test empty file data (valid case)
    auto emptyFileDataPdu = sts1cobcsw::FileDataPdu(42U, std::span<Byte const>{});
    CHECK(emptyFileDataPdu.offset_ == 42U);
    CHECK(emptyFileDataPdu.fileData_.size() == 0U);
}


TEST_CASE("Adding FileDataPdu to data field")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto fileDataPdu = sts1cobcsw::FileDataPdu{};
    fileDataPdu.offset_ = 0x0102'0304U;
    static constexpr auto fileData = std::array{0xAB_b, 0xCD_b};
    fileDataPdu.fileData_ = fileData;

    CHECK(fileDataPdu.Size() == 6U);  // 4B offset + 2B data field

    auto addResult = fileDataPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == fileDataPdu.Size());
    CHECK(dataField[0] == 0x01_b);
    CHECK(dataField[1] == 0x02_b);
    CHECK(dataField[2] == 0x03_b);
    CHECK(dataField[3] == 0x04_b);
    CHECK(dataField[4] == 0xAB_b);
    CHECK(dataField[5] == 0xCD_b);

    // Adding to a full data field should fail (I won't check this for the other PDUs/payloads since
    // this behavior is inherited from Payload)
    dataField.resize(sts1cobcsw::tc::maxPduDataLength);
    addResult = fileDataPdu.AddTo(&dataField);
    CHECK(addResult.has_error());
    CHECK(addResult.error() == ErrorCode::dataFieldTooShort);
}


TEST_CASE("Parsing FileDataPdu")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    buffer.resize(4 + 2);
    buffer[0] = 0x01_b;  // Offset (high byte)
    buffer[1] = 0x02_b;  // Offset
    buffer[2] = 0x03_b;  // Offset
    buffer[3] = 0x04_b;  // Offset (low byte)
    buffer[4] = 0xAB_b;  // Data field
    buffer[5] = 0xCD_b;  // Data field
    auto parseResult = sts1cobcsw::ParseAsFileDataPdu(buffer);
    REQUIRE(parseResult.has_value());
    auto & fileDataPdu = parseResult.value();
    CHECK(fileDataPdu.offset_ == 0x0102'0304U);
    CHECK(fileDataPdu.fileData_.size() == 2U);
    CHECK(fileDataPdu.fileData_[0] == 0xAB_b);
    CHECK(fileDataPdu.fileData_[1] == 0xCD_b);

    // Buffer must be > serialSize(offset)
    buffer.resize(2);
    parseResult = sts1cobcsw::ParseAsFileDataPdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("Parsing FileDirectivePdu")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    buffer.resize(1 + 2);
    buffer[0] = 0x04_b;  // Directive code (EOF)
    buffer[1] = 0xAB_b;  // Parameter field
    buffer[2] = 0xCD_b;  // Parameter field
    auto parseResult = sts1cobcsw::ParseAsFileDirectivePdu(buffer);
    REQUIRE(parseResult.has_value());
    auto & fileDirectivePdu = parseResult.value();
    CHECK(fileDirectivePdu.directiveCode == sts1cobcsw::DirectiveCode::endOfFile);
    CHECK(fileDirectivePdu.parameterField.size() == 2U);
    CHECK(fileDirectivePdu.parameterField[0] == 0xAB_b);
    CHECK(fileDirectivePdu.parameterField[1] == 0xCD_b);

    // Buffer must be >= serialSize(directiveCode)
    buffer.resize(0);
    parseResult = sts1cobcsw::ParseAsFileDirectivePdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);

    // Invalid directive code
    buffer.resize(3);
    buffer[0] = 0xFF_b;  // Invalid directive code
    parseResult = sts1cobcsw::ParseAsFileDirectivePdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidFileDirectiveCode);
}


TEST_CASE("EndOfFilePdu Constructor")
{
    // Test constructor for no-error condition (without fault location)
    auto endOfFilePdu = sts1cobcsw::EndOfFilePdu(0x9ABC'DEF0U);  // File Size

    CHECK(endOfFilePdu.conditionCode_ == sts1cobcsw::noErrorConditionCode);
    CHECK(endOfFilePdu.fileChecksum_ == 0x0000'0000U);
    CHECK(endOfFilePdu.fileSize_ == 0x9ABC'DEF0U);
    CHECK(endOfFilePdu.Size() == 9U);  // No fault location

    // Test serialization
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto addResult = endOfFilePdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == endOfFilePdu.Size());
    CHECK(dataField[0] == 0x00_b);  // Condition code (no error)
    CHECK(dataField[1] == 0x00_b);  // File checksum
    CHECK(dataField[2] == 0x00_b);
    CHECK(dataField[3] == 0x00_b);
    CHECK(dataField[4] == 0x00_b);
    CHECK(dataField[5] == 0x9A_b);  // File size
    CHECK(dataField[6] == 0xBC_b);
    CHECK(dataField[7] == 0xDE_b);
    CHECK(dataField[8] == 0xF0_b);

    // Test constructor for error condition (with fault location)
    auto faultLocation = sts1cobcsw::FaultLocation{};
    faultLocation.type = sts1cobcsw::TlvType::entityId;
    faultLocation.length = 1;
    faultLocation.value = sts1cobcsw::EntityId(0x0F);

    auto errorEndOfFilePdu =
        sts1cobcsw::EndOfFilePdu(sts1cobcsw::positiveAckLimitReachedConditionCode,
                                 0x1122'3344,  // fileSize
                                 faultLocation);

    CHECK(errorEndOfFilePdu.conditionCode_ == sts1cobcsw::positiveAckLimitReachedConditionCode);
    CHECK(errorEndOfFilePdu.fileChecksum_ == 0x0000'0000U);
    CHECK(errorEndOfFilePdu.fileSize_ == 0x1122'3344U);
    CHECK(errorEndOfFilePdu.faultLocation_.type == sts1cobcsw::TlvType::entityId);
    CHECK(errorEndOfFilePdu.faultLocation_.length == 1);
    CHECK(errorEndOfFilePdu.faultLocation_.value == sts1cobcsw::EntityId(0x0F));
    CHECK(errorEndOfFilePdu.Size() == 12U);  // 9B + 3B fault location

    // Test serialization with fault location
    dataField.clear();
    addResult = errorEndOfFilePdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == errorEndOfFilePdu.Size());
    CHECK(dataField[0] == 0x10_b);   // Condition code (positive ACK limit reached)
    CHECK(dataField[9] == 0x06_b);   // Fault location type (entity ID)
    CHECK(dataField[10] == 0x01_b);  // Fault location length
    CHECK(dataField[11] == 0x0F_b);  // Fault location value (entity ID)
}


TEST_CASE("Adding EndOfFilePdu to data field")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto endOfFilePdu = sts1cobcsw::EndOfFilePdu{};
    endOfFilePdu.conditionCode_ = sts1cobcsw::noErrorConditionCode;
    endOfFilePdu.fileChecksum_ = 0x1234'5678U;
    endOfFilePdu.fileSize_ = 0x9ABC'DEF0U;

    CHECK(endOfFilePdu.Size() == 9U);  // 1B condition code + 4B file checksum + 4B file size

    auto addResult = endOfFilePdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == endOfFilePdu.Size());
    CHECK(dataField[0] == 0x00_b);  // Condition code (no error)
    CHECK(dataField[1] == 0x12_b);  // File checksum
    CHECK(dataField[2] == 0x34_b);  // File checksum
    CHECK(dataField[3] == 0x56_b);  // File checksum
    CHECK(dataField[4] == 0x78_b);  // File checksum
    CHECK(dataField[5] == 0x9A_b);  // File size
    CHECK(dataField[6] == 0xBC_b);  // File size
    CHECK(dataField[7] == 0xDE_b);  // File size
    CHECK(dataField[8] == 0xF0_b);  // File size

    endOfFilePdu.conditionCode_ = sts1cobcsw::invalidTransmissionModeConditionCode;
    endOfFilePdu.faultLocation_.type = sts1cobcsw::TlvType::entityId;
    endOfFilePdu.faultLocation_.length = 1;
    endOfFilePdu.faultLocation_.value = sts1cobcsw::EntityId(0x0F);

    CHECK(endOfFilePdu.Size() == 12U);  // 9 B + 3 B fault location

    dataField.clear();
    addResult = endOfFilePdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == endOfFilePdu.Size());
    CHECK(dataField[0] == 0x30_b);   // Condition code (invalid transmission mode)
    CHECK(dataField[9] == 0x06_b);   // Fault location type (entity ID)
    CHECK(dataField[10] == 0x01_b);  // Fault location length
    CHECK(dataField[11] == 0x0F_b);  // Fault location value (entity ID)
}


TEST_CASE("Parsing EndOfFilePdu")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    buffer.resize(9);
    buffer[0] = 0x00_b;  // Condition code (no error)
    buffer[1] = 0x12_b;  // File checksum (high byte)
    buffer[2] = 0x34_b;  // File checksum
    buffer[3] = 0x56_b;  // File checksum
    buffer[4] = 0x78_b;  // File checksum (low byte)
    buffer[5] = 0x9A_b;  // File size (high byte)
    buffer[6] = 0xBC_b;  // File size
    buffer[7] = 0xDE_b;  // File size
    buffer[8] = 0xF0_b;  // File size (low byte)

    auto parseResult = sts1cobcsw::ParseAsEndOfFilePdu(buffer);
    REQUIRE(parseResult.has_value());
    auto & endOfFilePdu = parseResult.value();
    CHECK(endOfFilePdu.conditionCode_ == sts1cobcsw::noErrorConditionCode);
    CHECK(endOfFilePdu.fileChecksum_ == 0x1234'5678U);
    CHECK(endOfFilePdu.fileSize_ == 0x9ABC'DEF0U);

    // Buffer must be >= serialSize(conditionCode, fileChecksum, fileSize)
    buffer.resize(8);
    parseResult = sts1cobcsw::ParseAsEndOfFilePdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);

    buffer.resize(12);
    buffer[9] = 6_b;      // Fault location type (entity ID)
    buffer[10] = 1_b;     // Fault location length
    buffer[11] = 0x0F_b;  // Fault location value (entity ID)
    parseResult = sts1cobcsw::ParseAsEndOfFilePdu(buffer);
    REQUIRE(parseResult.has_value());
    endOfFilePdu = parseResult.value();
    // Condition code is still no error, so we don't parse the fault location
    CHECK(endOfFilePdu.faultLocation_.type == sts1cobcsw::TlvType::entityId);
    CHECK(endOfFilePdu.faultLocation_.length == 0);
    CHECK(endOfFilePdu.faultLocation_.value == sts1cobcsw::EntityId(0));

    buffer[0] = 0x10_b;  // Condition code (positive ACK limit reached)
    parseResult = sts1cobcsw::ParseAsEndOfFilePdu(buffer);
    REQUIRE(parseResult.has_value());
    endOfFilePdu = parseResult.value();
    CHECK(endOfFilePdu.conditionCode_ == sts1cobcsw::positiveAckLimitReachedConditionCode);
    CHECK(endOfFilePdu.faultLocation_.type == sts1cobcsw::TlvType::entityId);
    CHECK(endOfFilePdu.faultLocation_.length == 1);
    CHECK(endOfFilePdu.faultLocation_.value == sts1cobcsw::EntityId(0x0F));

    // Buffer must be == serialSize(conditionCode, fileChecksum, fileSize, faultLocation)
    buffer.resize(13);
    parseResult = sts1cobcsw::ParseAsEndOfFilePdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
    buffer.resize(12);
}


TEST_CASE("FinishedPdu Constructor")
{
    // Test constructor for no-error condition (without fault location)
    auto finishedPdu = sts1cobcsw::FinishedPdu(sts1cobcsw::dataCompleteDeliveryCode,
                                               sts1cobcsw::fileRetainedFileStatus);

    CHECK(finishedPdu.conditionCode_ == sts1cobcsw::noErrorConditionCode);
    CHECK(finishedPdu.deliveryCode_ == sts1cobcsw::dataCompleteDeliveryCode);
    CHECK(finishedPdu.fileStatus_ == sts1cobcsw::fileRetainedFileStatus);
    CHECK(finishedPdu.Size() == 1U);  // No fault location

    // Test serialization
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto addResult = finishedPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == finishedPdu.Size());
    CHECK(dataField[0] == 0x02_b);  // Condition=0, spare=0, delivery=0, fileStatus=2 (0b10)

    // Test constructor for error condition (with fault location)
    auto faultLocation = sts1cobcsw::FaultLocation{};
    faultLocation.type = sts1cobcsw::TlvType::entityId;
    faultLocation.length = 1;
    faultLocation.value = sts1cobcsw::EntityId(0x0F);

    auto errorFinishedPdu =
        sts1cobcsw::FinishedPdu(sts1cobcsw::positiveAckLimitReachedConditionCode,
                                sts1cobcsw::dataIncompleteDeliveryCode,
                                sts1cobcsw::fileRejectedFileStatus,
                                faultLocation);

    CHECK(errorFinishedPdu.conditionCode_ == sts1cobcsw::positiveAckLimitReachedConditionCode);
    CHECK(errorFinishedPdu.deliveryCode_ == sts1cobcsw::dataIncompleteDeliveryCode);
    CHECK(errorFinishedPdu.fileStatus_ == sts1cobcsw::fileRejectedFileStatus);
    CHECK(errorFinishedPdu.faultLocation_.type == sts1cobcsw::TlvType::entityId);
    CHECK(errorFinishedPdu.faultLocation_.length == 1);
    CHECK(errorFinishedPdu.faultLocation_.value == sts1cobcsw::EntityId(0x0F));
    CHECK(errorFinishedPdu.Size() == 4U);  // 1B + 3B fault location

    // Serialization with fault location
    dataField.clear();
    addResult = errorFinishedPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == errorFinishedPdu.Size());
    CHECK(dataField[0] == 0x15_b);  // Condition=1, spare=0, delivery=1, fileStatus=1 (0b0001'0101)
    CHECK(dataField[1] == 0x06_b);  // Fault location type (entity ID)
    CHECK(dataField[2] == 0x01_b);  // Fault location length
    CHECK(dataField[3] == 0x0F_b);  // Fault location value (entity ID)
}


TEST_CASE("Adding FinishedPdu")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto finishedPdu = sts1cobcsw::FinishedPdu{};
    finishedPdu.conditionCode_ = sts1cobcsw::noErrorConditionCode;
    finishedPdu.deliveryCode_ = sts1cobcsw::DeliveryCode(0);
    finishedPdu.fileStatus_ = sts1cobcsw::FileStatus(0);

    CHECK(finishedPdu.Size() == 1U);

    auto addResult = finishedPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == finishedPdu.Size());
    CHECK(dataField[0]
          == 0x00_b);  // Condition code (no error), Delivery code (0), and FileStatus(0)

    finishedPdu.conditionCode_ = sts1cobcsw::invalidTransmissionModeConditionCode;
    finishedPdu.deliveryCode_ = sts1cobcsw::DeliveryCode(0);
    finishedPdu.fileStatus_ = sts1cobcsw::FileStatus(0);
    finishedPdu.faultLocation_.length = 1;
    finishedPdu.faultLocation_.value = sts1cobcsw::EntityId(0x0F);

    CHECK(finishedPdu.Size() == 4U);  // 1 B + 3 B fault location

    dataField.clear();
    addResult = finishedPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == finishedPdu.Size());
    CHECK(dataField[0] == 0x30_b);  // Condition code (invalid transmission mode)
    CHECK(dataField[1] == 0x06_b);  // Fault location type (entity ID)
    CHECK(dataField[2] == 0x01_b);  // Fault location length
    CHECK(dataField[3] == 0x0F_b);  // Fault location value (entity ID)

    finishedPdu.conditionCode_ = sts1cobcsw::unsupportedChecksumTypeConditionCode;
    finishedPdu.deliveryCode_ = sts1cobcsw::DeliveryCode(1);
    finishedPdu.fileStatus_ = sts1cobcsw::FileStatus(1);

    CHECK(finishedPdu.Size() == 1U);  // Fault Location is omitted

    dataField.clear();
    addResult = finishedPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == finishedPdu.Size());
    CHECK(dataField[0]
          == 0xB5_b);  // Unsupported Checksum Type (0xB), Delivery Code and File Status (0b0101)
}


TEST_CASE("Parsing FinishedPdu")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    // Minimum parameter field length for FinishedPdu is 1 byte
    CHECK(sts1cobcsw::FinishedPdu::minParameterFieldLength == 1U);
    buffer.resize(sts1cobcsw::FinishedPdu::minParameterFieldLength);

    buffer[0] = 0x05_b;  // Condition Code : 0b0000
                         // Spare, Delivery Code, FileStatus : 0b0101

    auto parseResult = sts1cobcsw::ParseAsFinishedPdu(buffer);
    REQUIRE(parseResult.has_value());
    auto & finishedPdu = parseResult.value();
    CHECK(finishedPdu.conditionCode_ == sts1cobcsw::noErrorConditionCode);
    CHECK(finishedPdu.deliveryCode_ == sts1cobcsw::dataIncompleteDeliveryCode);
    CHECK(finishedPdu.fileStatus_ == sts1cobcsw::fileRejectedFileStatus);

    // Buffer size must >= serialSize(conditionCode, spare, deliveryCode, fileStatus)
    buffer.resize(sts1cobcsw::FinishedPdu::minParameterFieldLength - 1);
    parseResult = sts1cobcsw::ParseAsFinishedPdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
    buffer.resize(sts1cobcsw::FinishedPdu::minParameterFieldLength);

    // Extra bytes should be ignored when no error
    buffer.resize(
        sts1cobcsw::FinishedPdu::minParameterFieldLength
        + static_cast<std::size_t>(sts1cobcsw::totalSerialSize<sts1cobcsw::FaultLocation>));
    buffer[1] = 0x06_b;
    buffer[2] = 0x01_b;
    buffer[3] = 0x0F_b;
    parseResult = sts1cobcsw::ParseAsFinishedPdu(buffer);
    REQUIRE(parseResult.has_value());
    finishedPdu = parseResult.value();
    CHECK(finishedPdu.conditionCode_ == sts1cobcsw::noErrorConditionCode);
    CHECK(finishedPdu.faultLocation_.type == sts1cobcsw::TlvType::entityId);
    CHECK(finishedPdu.faultLocation_.length == 0);
    CHECK(finishedPdu.faultLocation_.value == sts1cobcsw::EntityId(0));

    // Error condition: positive ACK limit reached (ConditionCode = 1)
    buffer[0] = 0x10_b;
    parseResult = sts1cobcsw::ParseAsFinishedPdu(buffer);
    REQUIRE(parseResult.has_value());
    finishedPdu = parseResult.value();
    CHECK(finishedPdu.conditionCode_ == sts1cobcsw::positiveAckLimitReachedConditionCode);
    CHECK(finishedPdu.faultLocation_.type == sts1cobcsw::TlvType::entityId);
    CHECK(finishedPdu.faultLocation_.length
          == static_cast<std::uint8_t>(sts1cobcsw::totalSerialSize<sts1cobcsw::EntityId>));
    CHECK(finishedPdu.faultLocation_.value == sts1cobcsw::EntityId(0x0F));

    // Invalid data length for error condition
    buffer.resize(sts1cobcsw::FinishedPdu::minParameterFieldLength
                  + static_cast<std::size_t>(sts1cobcsw::totalSerialSize<sts1cobcsw::FaultLocation>)
                  + 1);
    parseResult = sts1cobcsw::ParseAsFinishedPdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
    // Restore correct buffer size
    buffer.resize(
        sts1cobcsw::FinishedPdu::minParameterFieldLength
        + static_cast<std::size_t>(sts1cobcsw::totalSerialSize<sts1cobcsw::FaultLocation>));
}


TEST_CASE("AckPdu Constructor")
{
    // Test constructor with Finished directive
    auto ackPdu = sts1cobcsw::AckPdu(sts1cobcsw::DirectiveCode::finished,
                                     sts1cobcsw::noErrorConditionCode,
                                     sts1cobcsw::terminatedTransactionStatus);

    CHECK(ackPdu.acknowledgedPduDirectiveCode_.ToUnderlying() == 5);  // Finished = 5
    CHECK(ackPdu.directiveSubtypeCode_ == 0b0001);                    // Finished gets 0b0001
    CHECK(value_of(ackPdu.conditionCode_) == 0);                      // no error
    CHECK(ackPdu.transactionStatus_ == sts1cobcsw::terminatedTransactionStatus);

    // Test constructor with EndOfFile directive
    auto ackPduEof = sts1cobcsw::AckPdu(sts1cobcsw::DirectiveCode::endOfFile,
                                        sts1cobcsw::positiveAckLimitReachedConditionCode,
                                        sts1cobcsw::activeTransactionStatus);

    CHECK(ackPduEof.acknowledgedPduDirectiveCode_.ToUnderlying() == 4);  // EndOfFile = 4
    CHECK(ackPduEof.directiveSubtypeCode_ == 0b0000);                    // Non-Finished gets 0b0000
    CHECK(value_of(ackPduEof.conditionCode_) == 1);  // positive ACK limit reached
    CHECK(ackPduEof.transactionStatus_ == sts1cobcsw::activeTransactionStatus);

    // Test serialization
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto addResult = ackPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == ackPdu.Size());
    CHECK(dataField[0] == 0x51_b);  // 5 << 4 | 1 = 0x51 (Finished with subtype 1)
    CHECK(dataField[1] == 0x02_b);  //
}


TEST_CASE("Adding AckPdu")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto ackPdu = sts1cobcsw::AckPdu{};
    ackPdu.acknowledgedPduDirectiveCode_ =
        static_cast<std::uint32_t>(sts1cobcsw::DirectiveCode::finished);
    ackPdu.directiveSubtypeCode_ = 0;
    ackPdu.conditionCode_ = sts1cobcsw::noErrorConditionCode;
    ackPdu.transactionStatus_ = sts1cobcsw::undefinedTransactionStatus;

    CHECK(ackPdu.minParameterFieldLength == 2U);
    CHECK(ackPdu.Size() == 2U);

    auto addResult = ackPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == ackPdu.Size());
    CHECK(dataField[0] == 0x50_b);
    CHECK(dataField[1] == 0x00_b);
}


TEST_CASE("Parsing AckPdu")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    CHECK(sts1cobcsw::AckPdu::minParameterFieldLength == 2U);
    buffer.resize(sts1cobcsw::AckPdu::minParameterFieldLength);

    buffer[0] = 0x50_b;
    buffer[1] = 0x12_b;

    auto parseResult = sts1cobcsw::ParseAsAckPdu(buffer);
    REQUIRE(parseResult.has_value());
    auto & ackPdu = parseResult.value();
    CHECK(ackPdu.acknowledgedPduDirectiveCode_.ToUnderlying() == 5);
    CHECK(ackPdu.directiveSubtypeCode_ == 0);
    CHECK(value_of(ackPdu.conditionCode_) == 1);
    CHECK(ackPdu.transactionStatus_ == sts1cobcsw::terminatedTransactionStatus);

    buffer.resize(1);
    parseResult = sts1cobcsw::ParseAsAckPdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);

    buffer.resize(2);
    buffer[0] = 0x00_b;
    parseResult = sts1cobcsw::ParseAsAckPdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidAckPduDirectiveCode);
    buffer[0] = 0x52_b;
    parseResult = sts1cobcsw::ParseAsAckPdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDirectiveSubtypeCode);
}


TEST_CASE("MetadataPdu Constructor")
{
    static auto const sourceFileName = sts1cobcsw::fs::Path("foo");
    static auto const destinationFileName = sts1cobcsw::fs::Path("bar");

    auto metadataPdu = sts1cobcsw::MetadataPdu(42U,  // fileSize
                                               sourceFileName,
                                               destinationFileName);

    CHECK(metadataPdu.fileSize_ == 42U);
    CHECK(metadataPdu.sourceFileNameLength_ == 3U);
    CHECK(metadataPdu.sourceFileNameValue_.size() == 3U);
    CHECK(metadataPdu.sourceFileNameValue_[0] == 0x66);
    CHECK(metadataPdu.sourceFileNameValue_[1] == 0x6F);
    CHECK(metadataPdu.sourceFileNameValue_[2] == 0x6F);

    CHECK(metadataPdu.destinationFileNameLength_ == 3U);
    CHECK(metadataPdu.destinationFileNameValue_.size() == 3U);
    CHECK(metadataPdu.destinationFileNameValue_[0] == 0x62);
    CHECK(metadataPdu.destinationFileNameValue_[1] == 0x61);
    CHECK(metadataPdu.destinationFileNameValue_[2] == 0x72);

    // Test serialization
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto addResult = metadataPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());

    CHECK(dataField.size() == metadataPdu.Size());
    // Expected: 1B flags + 4B file size + 1B source length + 3B source + 1B dest length + 3B
    // dest = 13B
    CHECK(dataField.size() == 13U);
    CHECK(dataField[0] == 0x0F_b);   // flags: reserved=0, closure=0, reserved2=0, checksum=15
    CHECK(dataField[1] == 0x00_b);   // file size
    CHECK(dataField[4] == 0x2A_b);   // file size
    CHECK(dataField[5] == 0x03_b);   // source file name length
    CHECK(dataField[6] == 0x66_b);   // source file name
    CHECK(dataField[7] == 0x6F_b);   // source file name
    CHECK(dataField[8] == 0x6F_b);   // source file name
    CHECK(dataField[9] == 0x03_b);   // destination file name length
    CHECK(dataField[10] == 0x62_b);  // destination file name
    CHECK(dataField[11] == 0x61_b);  // destination file name
    CHECK(dataField[12] == 0x72_b);  // destination file name
}


TEST_CASE("Adding MetadataPdu")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto metadataPdu = sts1cobcsw::MetadataPdu{};

    metadataPdu.fileSize_ = 42;

    static auto const sourceFileName = sts1cobcsw::fs::Path("abc");
    static auto const destinationFileName = sts1cobcsw::fs::Path("def");

    metadataPdu.sourceFileNameLength_ = static_cast<std::uint8_t>(sourceFileName.size());
    metadataPdu.sourceFileNameValue_ = sourceFileName;

    metadataPdu.destinationFileNameLength_ = static_cast<std::uint8_t>(destinationFileName.size());
    metadataPdu.destinationFileNameValue_ = destinationFileName;

    CHECK(metadataPdu.Size() == (1 + 4 + 1 + 3 + 1 + 3));

    auto addResult = metadataPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());

    CHECK(dataField.size() == metadataPdu.Size());
    CHECK(dataField[0] == 0x0F_b);
    CHECK(dataField[1] == 0x00_b);
    CHECK(dataField[2] == 0x00_b);
    CHECK(dataField[3] == 0x00_b);
    CHECK(dataField[4] == 0x2A_b);
    CHECK(dataField[5] == 0x03_b);
    CHECK(dataField[6] == 0x61_b);
    CHECK(dataField[7] == 0x62_b);
    CHECK(dataField[8] == 0x63_b);
    CHECK(dataField[9] == 0x03_b);
    CHECK(dataField[10] == 0x64_b);
    CHECK(dataField[11] == 0x65_b);
    CHECK(dataField[12] == 0x66_b);
}


TEST_CASE("Parsing MetadataPdu")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    CHECK(sts1cobcsw::MetadataPdu::minParameterFieldLength == 6U);
    buffer.resize(11);

    buffer[0] = 0x00_b;
    buffer[1] = 0x00_b;
    buffer[2] = 0x00_b;
    buffer[3] = 0x00_b;
    buffer[4] = 0x12_b;
    buffer[5] = 0x02_b;
    buffer[6] = 0x67_b;
    buffer[7] = 0x67_b;
    buffer[8] = 0x02_b;
    buffer[9] = 0x67_b;
    buffer[10] = 0x67_b;

    auto parseResult = sts1cobcsw::ParseAsMetadataPdu(buffer);
    REQUIRE(parseResult.has_value());

    auto & metadataPdu = parseResult.value();

    CHECK(metadataPdu.fileSize_ == 18U);
    CHECK(metadataPdu.sourceFileNameLength_ == 2U);
    CHECK(metadataPdu.sourceFileNameValue_[0] == static_cast<char>(0x67_b));
    CHECK(metadataPdu.sourceFileNameValue_[1] == static_cast<char>(0x67_b));
    CHECK(metadataPdu.destinationFileNameLength_ == 2U);
    CHECK(metadataPdu.destinationFileNameValue_[0] == static_cast<char>(0x67_b));
    CHECK(metadataPdu.destinationFileNameValue_[1] == static_cast<char>(0x67_b));

    buffer.resize(1);
    parseResult = sts1cobcsw::ParseAsMetadataPdu(buffer);
    CHECK((parseResult.has_error() and parseResult.error() == ErrorCode::bufferTooSmall));

    buffer.resize(10);
    buffer[5] = 0x0A_b;  // Size of source file name length
    parseResult = sts1cobcsw::ParseAsMetadataPdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);

    buffer.resize(12);
    buffer[5] = 0x02_b;  // Size of source file name length
    buffer[8] = 0x04_b;  // Size of destination file name length
    parseResult = sts1cobcsw::ParseAsMetadataPdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);

    buffer.resize(13);
    parseResult = sts1cobcsw::ParseAsMetadataPdu(buffer);
    CHECK(parseResult.has_value());
}


TEST_CASE("NakPdu Constructor")
{
    // Test valid constructor with segment requests within limit
    static auto const segmentRequests =
        etl::vector<sts1cobcsw::SegmentRequest, sts1cobcsw::NakPdu::maxNSegmentRequests>{
            sts1cobcsw::SegmentRequest{.startOffset = 0x1234'5678, .endOffset = 0x1234'9ABC},
            sts1cobcsw::SegmentRequest{.startOffset = 0x1234'DEF0, .endOffset = 0x2222'3333},
            sts1cobcsw::SegmentRequest{.startOffset = 0x4444'5555, .endOffset = 0x6789'ABCD}
    };

    auto nakPdu = sts1cobcsw::NakPdu(segmentRequests);

    CHECK(nakPdu.startOfScope_ == 0x1234'5678U);
    CHECK(nakPdu.endOfScope_ == 0x6789'ABCDU);
    CHECK(nakPdu.segmentRequests_.size() == 3U);

    CHECK(nakPdu.segmentRequests_[0].startOffset == 0x1234'5678U);
    CHECK(nakPdu.segmentRequests_[0].endOffset == 0x1234'9ABCU);
    CHECK(nakPdu.segmentRequests_[1].startOffset == 0x1234'DEF0U);
    CHECK(nakPdu.segmentRequests_[1].endOffset == 0x2222'3333U);
    CHECK(nakPdu.segmentRequests_[2].startOffset == 0x4444'5555U);
    CHECK(nakPdu.segmentRequests_[2].endOffset == 0x6789'ABCDU);

    // Test serialization
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto addResult = nakPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == nakPdu.Size());
    CHECK(dataField[0] == 0x12_b);  // startOfScope high byte
    CHECK(dataField[1] == 0x34_b);
    CHECK(dataField[2] == 0x56_b);
    CHECK(dataField[3] == 0x78_b);  // startOfScope low byte
    CHECK(dataField[4] == 0x67_b);  // endOfScope high byte
    CHECK(dataField[5] == 0x89_b);
    CHECK(dataField[6] == 0xAB_b);
    CHECK(dataField[7] == 0xCD_b);  // endOfScope low byte

    // Test with maximum allowed segment requests (25)
    // auto maxSegmentRequests = etl::vector<std::uint64_t,
    // sts1cobcsw::NakPdu::maxSegmentRequests>{};
    // maxSegmentRequests.resize(sts1cobcsw::NakPdu::maxSegmentRequests);
    // std::fill(maxSegmentRequests.begin(), maxSegmentRequests.end(), 0xAAAA'BBBB'CCCC'DDDD);
    // auto maxNakPdu = sts1cobcsw::NakPdu(0, maxSegmentRequests);
    // CHECK(maxNakPdu.segmentRequests_.size() == sts1cobcsw::NakPdu::maxSegmentRequests);
}


TEST_CASE("Adding NakPdu")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto nakPdu = sts1cobcsw::NakPdu{};

    nakPdu.startOfScope_ = 0;
    nakPdu.endOfScope_ = 0;

    auto segmentRequest1 = sts1cobcsw::SegmentRequest{.startOffset = 0xAB, .endOffset = 0x0};
    auto segmentRequest2 = sts1cobcsw::SegmentRequest{.startOffset = 0xCD, .endOffset = 0x0};

    nakPdu.segmentRequests_ =
        etl::vector<sts1cobcsw::SegmentRequest, sts1cobcsw::NakPdu::maxNSegmentRequests>{
            segmentRequest1, segmentRequest2};

    CHECK(nakPdu.Size() == 24U);  // NOLINT(*magic-numbers)

    auto addResult = nakPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());

    CHECK(dataField.size() == nakPdu.Size());
    CHECK(dataField[0] == 0x00_b);
    CHECK(dataField[1] == 0x00_b);
    CHECK(dataField[2] == 0x00_b);
    CHECK(dataField[3] == 0x00_b);
    CHECK(dataField[4] == 0x00_b);
    CHECK(dataField[5] == 0x00_b);
    CHECK(dataField[6] == 0x00_b);
    CHECK(dataField[7] == 0x00_b);

    CHECK(dataField[8] == 0x00_b);
    CHECK(dataField[9] == 0x00_b);
    CHECK(dataField[10] == 0x00_b);
    CHECK(dataField[11] == 0x0AB_b);
    CHECK(dataField[12] == 0x00_b);
    CHECK(dataField[13] == 0x00_b);
    CHECK(dataField[14] == 0x00_b);
    CHECK(dataField[15] == 0x00_b);

    CHECK(dataField[16] == 0x00_b);
    CHECK(dataField[17] == 0x00_b);
    CHECK(dataField[18] == 0x00_b);
    CHECK(dataField[19] == 0xCD_b);
    CHECK(dataField[20] == 0x00_b);
    CHECK(dataField[21] == 0x00_b);
    CHECK(dataField[22] == 0x00_b);
    CHECK(dataField[23] == 0x00_b);
}

TEST_CASE("Parsing NakPdu")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    buffer.resize(24U);

    buffer[0] = 0x11_b;
    buffer[1] = 0x11_b;
    buffer[2] = 0x22_b;
    buffer[3] = 0x22_b;
    buffer[4] = 0x77_b;
    buffer[5] = 0x77_b;
    buffer[6] = 0x88_b;
    buffer[7] = 0x88_b;
    buffer[8] = 0x11_b;  // start of first segment request
    buffer[9] = 0x11_b;
    buffer[10] = 0x22_b;
    buffer[11] = 0x22_b;
    buffer[12] = 0x33_b;
    buffer[13] = 0x33_b;
    buffer[14] = 0x44_b;
    buffer[15] = 0x44_b;
    buffer[16] = 0x55_b;  // start of second segment request
    buffer[17] = 0x55_b;
    buffer[18] = 0x66_b;
    buffer[19] = 0x66_b;
    buffer[20] = 0x77_b;
    buffer[21] = 0x77_b;
    buffer[22] = 0x88_b;
    buffer[23] = 0x88_b;

    auto parseResult = sts1cobcsw::ParseAsNakPdu(buffer);
    REQUIRE(parseResult.has_value());

    auto & nakPdu = parseResult.value();

    CHECK(nakPdu.startOfScope_ == 0x1111'2222U);
    CHECK(nakPdu.endOfScope_ == 0x7777'8888U);
    CHECK(nakPdu.segmentRequests_.size() == 2U);
    CHECK(nakPdu.segmentRequests_[0].startOffset == 0x1111'2222U);
    CHECK(nakPdu.segmentRequests_[0].endOffset == 0x3333'4444U);
    CHECK(nakPdu.segmentRequests_[1].startOffset == 0x5555'6666U);
    CHECK(nakPdu.segmentRequests_[1].endOffset == 0x7777'8888U);

    // endOfScope must be the endOffset of the last segment request
    buffer[23] = 0x00_b;
    parseResult = sts1cobcsw::ParseAsNakPdu(buffer);
    REQUIRE(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidNakPdu);
}


TEST_CASE("AddPduTo function")
{
    // Test successful case with file data PDU from ground station to cubesat
    {
        auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
        static constexpr auto fileData = std::array{0xAB_b, 0xCD_b, 0xEF_b};
        auto fileDataPdu = sts1cobcsw::FileDataPdu(0x1234'5678U, fileData);

        auto result = sts1cobcsw::AddPduTo(&dataField,
                                           sts1cobcsw::fileDataPduType,
                                           sts1cobcsw::groundStationEntityId,
                                           0x9ABC,
                                           fileDataPdu);

        REQUIRE(result.has_value());
        CHECK(dataField.size() == sts1cobcsw::pduHeaderLength + fileDataPdu.Size());

        // Verify header fields by parsing the created PDU
        auto parseResult = sts1cobcsw::ParseAsProtocolDataUnit(dataField);
        REQUIRE(parseResult.has_value());
        auto & pdu = parseResult.value();

        CHECK(pdu.header.version == sts1cobcsw::pduVersion);
        CHECK(pdu.header.pduType == sts1cobcsw::fileDataPduType);
        CHECK(pdu.header.direction == sts1cobcsw::towardsFileSenderDirection);
        CHECK(pdu.header.transmissionMode == sts1cobcsw::acknowledgedTransmissionMode);
        CHECK(pdu.header.crcFlag == 0);
        CHECK(pdu.header.largeFileFlag == 0);
        CHECK(pdu.header.pduDataFieldLength == fileDataPdu.Size());
        CHECK(pdu.header.segmentationControl == 0);
        CHECK(pdu.header.lengthOfEntityIds
              == sts1cobcsw::totalSerialSize<sts1cobcsw::EntityId> - 1);
        CHECK(pdu.header.segmentMetadataFlag == 0);
        CHECK(pdu.header.lengthOfTransactionSequenceNumber
              == sts1cobcsw::totalSerialSize<std::uint16_t> - 1);
        CHECK(pdu.header.sourceEntityId == sts1cobcsw::groundStationEntityId);
        CHECK(pdu.header.transactionSequenceNumber == 0x9ABC);
        CHECK(pdu.header.destinationEntityId == sts1cobcsw::cubeSatEntityId);
        CHECK(pdu.dataField.size() == fileDataPdu.Size());
    }

    // Test successful case with file directive PDU from cubesat to ground station
    {
        auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
        auto endOfFilePdu = sts1cobcsw::EndOfFilePdu(0x1234'5678U);

        auto result = sts1cobcsw::AddPduTo(&dataField,
                                           sts1cobcsw::fileDirectivePduType,
                                           sts1cobcsw::cubeSatEntityId,
                                           0x1234,
                                           endOfFilePdu);

        REQUIRE(result.has_value());
        CHECK(dataField.size() == sts1cobcsw::pduHeaderLength + endOfFilePdu.Size());

        // Verify header fields
        auto parseResult = sts1cobcsw::ParseAsProtocolDataUnit(dataField);
        REQUIRE(parseResult.has_value());
        auto & pdu = parseResult.value();

        CHECK(pdu.header.pduType == sts1cobcsw::fileDirectivePduType);
        CHECK(pdu.header.direction == sts1cobcsw::towardsFileReceiverDirection);
        CHECK(pdu.header.sourceEntityId == sts1cobcsw::cubeSatEntityId);
        CHECK(pdu.header.destinationEntityId == sts1cobcsw::groundStationEntityId);
        CHECK(pdu.header.transactionSequenceNumber == 0x1234);
    }

    // Test error case: insufficient buffer space
    {
        auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
        // Fill most of the buffer to leave insufficient space
        dataField.resize(sts1cobcsw::tc::maxPduLength - 5);

        static constexpr auto fileData = std::array{0xAB_b, 0xCD_b, 0xEF_b};
        auto fileDataPdu = sts1cobcsw::FileDataPdu(0x1234'5678U, fileData);

        auto result = sts1cobcsw::AddPduTo(&dataField,
                                           sts1cobcsw::fileDataPduType,
                                           sts1cobcsw::groundStationEntityId,
                                           0x1234,
                                           fileDataPdu);

        CHECK(result.has_error());
        CHECK(result.error() == ErrorCode::dataFieldTooShort);
    }
}

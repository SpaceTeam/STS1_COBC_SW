#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnits.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <strong_type/equality.hpp>

#include <etl/vector.h>

#include <array>
#include <cstddef>
#include <span>
#include <utility>


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::operator""_b;


TEST_CASE("Parsing ProtocolDataUnit")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    buffer.resize(sts1cobcsw::tc::pduHeaderLength);
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

    buffer.resize(sts1cobcsw::tc::pduHeaderLength - 1);  // Buffer too small
    parseResult = sts1cobcsw::ParseAsProtocolDataUnit(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("Adding FileDataPdu to data field")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto fileDataPdu = sts1cobcsw::FileDataPdu{};
    fileDataPdu.offset = 0x0102'0304U;
    static constexpr auto fileData = std::array{0xAB_b, 0xCD_b};
    fileDataPdu.fileData = fileData;

    CHECK(fileDataPdu.Size() == 6U);  // 4 B offset + 2 B data field

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
    CHECK(fileDataPdu.offset == 0x0102'0304U);
    CHECK(fileDataPdu.fileData.size() == 2U);
    CHECK(fileDataPdu.fileData[0] == 0xAB_b);
    CHECK(fileDataPdu.fileData[1] == 0xCD_b);

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


TEST_CASE("Adding EndOfFilePdu to data field")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto endOfFilePdu = sts1cobcsw::EndOfFilePdu{};
    endOfFilePdu.conditionCode = sts1cobcsw::noErrorConditionCode;
    endOfFilePdu.fileChecksum = 0x1234'5678U;
    endOfFilePdu.fileSize = 0x9ABC'DEF0U;

    CHECK(endOfFilePdu.Size() == 9U);  // 1 B condition code + 4 B file checksum + 4 B file size

    auto addResult = endOfFilePdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == endOfFilePdu.Size());
    CHECK(dataField[0] == 0x00_b);  // Condition code (no error)
    CHECK(dataField[1] == 0x12_b);  // File checksum (high byte)
    CHECK(dataField[2] == 0x34_b);  // File checksum
    CHECK(dataField[3] == 0x56_b);  // File checksum
    CHECK(dataField[4] == 0x78_b);  // File checksum (low byte)
    CHECK(dataField[5] == 0x9A_b);  // File size (high byte)
    CHECK(dataField[6] == 0xBC_b);  // File size
    CHECK(dataField[7] == 0xDE_b);  // File size
    CHECK(dataField[8] == 0xF0_b);  // File size (low byte)

    endOfFilePdu.conditionCode = sts1cobcsw::invalidTransmissionModeConditionCode;
    endOfFilePdu.faultLocation.type = sts1cobcsw::TlvType::entityId;
    endOfFilePdu.faultLocation.length = 1;
    endOfFilePdu.faultLocation.value = sts1cobcsw::EntityId(0x0F);

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
    CHECK(endOfFilePdu.conditionCode == sts1cobcsw::noErrorConditionCode);
    CHECK(endOfFilePdu.fileChecksum == 0x1234'5678U);
    CHECK(endOfFilePdu.fileSize == 0x9ABC'DEF0U);

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
    CHECK(endOfFilePdu.faultLocation.type == sts1cobcsw::TlvType::entityId);
    CHECK(endOfFilePdu.faultLocation.length == 0);
    CHECK(endOfFilePdu.faultLocation.value == sts1cobcsw::EntityId(0));

    buffer[0] = 0x10_b;  // Condition code (positive ACK limit reached)
    parseResult = sts1cobcsw::ParseAsEndOfFilePdu(buffer);
    REQUIRE(parseResult.has_value());
    endOfFilePdu = parseResult.value();
    CHECK(endOfFilePdu.conditionCode == sts1cobcsw::positiveAckLimitReachedConditionCode);
    CHECK(endOfFilePdu.faultLocation.type == sts1cobcsw::TlvType::entityId);
    CHECK(endOfFilePdu.faultLocation.length == 1);
    CHECK(endOfFilePdu.faultLocation.value == sts1cobcsw::EntityId(0x0F));

    // Buffer must be == serialSize(conditionCode, fileChecksum, fileSize, faultLocation)
    buffer.resize(13);
    parseResult = sts1cobcsw::ParseAsEndOfFilePdu(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
    buffer.resize(12);
}


TEST_CASE("Adding FinishedPdu")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto finishedPdu = sts1cobcsw::FinishedPdu{};
    finishedPdu.conditionCode = sts1cobcsw::noErrorConditionCode;
    finishedPdu.deliveryCode = sts1cobcsw::DeliveryCode(0);
    finishedPdu.fileStatus = sts1cobcsw::FileStatus(0);

    CHECK(finishedPdu.Size() == 1U);

    auto addResult = finishedPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == finishedPdu.Size());
    CHECK(dataField[0]
          == 0x00_b);  // Condition code (no error), Delivery code (0), and FileStatus(0)

    finishedPdu.conditionCode = sts1cobcsw::invalidTransmissionModeConditionCode;
    finishedPdu.deliveryCode = sts1cobcsw::DeliveryCode(0);
    finishedPdu.fileStatus = sts1cobcsw::FileStatus(0);
    finishedPdu.faultLocation.length = 1;
    finishedPdu.faultLocation.value = sts1cobcsw::EntityId(0x0F);

    CHECK(finishedPdu.Size() == 4U);  // 1 B + 3 B fault location

    dataField.clear();
    addResult = finishedPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());
    CHECK(dataField.size() == finishedPdu.Size());
    CHECK(dataField[0] == 0x30_b);  // Condition code (invalid transmission mode)
    CHECK(dataField[1] == 0x06_b);  // Fault location type (entity ID)
    CHECK(dataField[2] == 0x01_b);  // Fault location length
    CHECK(dataField[3] == 0x0F_b);  // Fault location value (entity ID)

    finishedPdu.conditionCode = sts1cobcsw::unsupportedChecksumTypeConditionCode;
    finishedPdu.deliveryCode = sts1cobcsw::DeliveryCode(1);
    finishedPdu.fileStatus = sts1cobcsw::FileStatus(1);

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
    CHECK(finishedPdu.conditionCode == sts1cobcsw::noErrorConditionCode);
    CHECK(finishedPdu.deliveryCode == sts1cobcsw::dataIncompleteDeliveryCode);
    CHECK(finishedPdu.fileStatus == sts1cobcsw::fileRejectedFileStatus);

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
    CHECK(finishedPdu.conditionCode == sts1cobcsw::noErrorConditionCode);
    CHECK(finishedPdu.faultLocation.type == sts1cobcsw::TlvType::entityId);
    CHECK(finishedPdu.faultLocation.length == 0);
    CHECK(finishedPdu.faultLocation.value == sts1cobcsw::EntityId(0));

    // Error condition: positive ACK limit reached (ConditionCode = 1)
    buffer[0] = 0x10_b;
    parseResult = sts1cobcsw::ParseAsFinishedPdu(buffer);
    REQUIRE(parseResult.has_value());
    finishedPdu = parseResult.value();
    CHECK(finishedPdu.conditionCode == sts1cobcsw::positiveAckLimitReachedConditionCode);
    CHECK(finishedPdu.faultLocation.type == sts1cobcsw::TlvType::entityId);
    CHECK(finishedPdu.faultLocation.length
          == static_cast<std::uint8_t>(sts1cobcsw::totalSerialSize<sts1cobcsw::EntityId>));
    CHECK(finishedPdu.faultLocation.value == sts1cobcsw::EntityId(0x0F));

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


TEST_CASE("Adding AckPdu")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto ackPdu = sts1cobcsw::AckPdu{};
    ackPdu.acknowledgedPduDirectiveCode =
        static_cast<std::uint32_t>(sts1cobcsw::DirectiveCode::finished);
    ackPdu.directiveSubtypeCode = 0;
    ackPdu.conditionCode = sts1cobcsw::noErrorConditionCode;
    ackPdu.transactionStatus = sts1cobcsw::undefinedTransactionstatus;

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
    CHECK(ackPdu.acknowledgedPduDirectiveCode.ToUnderlying() == 5);
    CHECK(ackPdu.directiveSubtypeCode == 0);
    CHECK(value_of(ackPdu.conditionCode) == 1);
    CHECK(ackPdu.transactionStatus == sts1cobcsw::terminatedTransactionStatus);

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


TEST_CASE("Adding MetadataPdu")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto metadataPdu = sts1cobcsw::MetadataPdu{};

    metadataPdu.fileSize = 42;

    static constexpr auto sourceFileName = std::array{0xAB_b, 0xCD_b};
    static constexpr auto destinationFileName = std::array{0x01_b, 0x23_b};

    metadataPdu.sourceFileNameLength = sourceFileName.size();
    metadataPdu.sourceFileNameValue = sourceFileName;

    metadataPdu.destinationFileNameLength = destinationFileName.size();
    metadataPdu.destinationFileNameValue = destinationFileName;

    CHECK(metadataPdu.Size() == (1 + 4 + 1 + 2 + 1 + 2));

    auto addResult = metadataPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());

    CHECK(dataField.size() == metadataPdu.Size());
    CHECK(dataField[0] == 0x3C_b);
    CHECK(dataField[1] == 0x00_b);
    CHECK(dataField[2] == 0x00_b);
    CHECK(dataField[3] == 0x00_b);
    CHECK(dataField[4] == 0x2A_b);
    CHECK(dataField[5] == 0x02_b);
    CHECK(dataField[6] == 0xAB_b);
    CHECK(dataField[7] == 0xCD_b);
    CHECK(dataField[8] == 0x02_b);
    CHECK(dataField[9] == 0x01_b);
    CHECK(dataField[10] == 0x23_b);
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
    buffer[6] = 0xAA_b;
    buffer[7] = 0xAA_b;
    buffer[8] = 0x02_b;
    buffer[9] = 0xAA_b;
    buffer[10] = 0xAA_b;

    auto parseResult = sts1cobcsw::ParseAsMetadataPdu(buffer);
    REQUIRE(parseResult.has_value());

    auto & metadataPdu = parseResult.value();

    CHECK(metadataPdu.fileSize == 18U);
    CHECK(metadataPdu.sourceFileNameLength == 2U);
    CHECK(metadataPdu.sourceFileNameValue[0] == 0xAA_b);
    CHECK(metadataPdu.sourceFileNameValue[1] == 0xAA_b);
    CHECK(metadataPdu.destinationFileNameLength == 2U);
    CHECK(metadataPdu.destinationFileNameValue[0] == 0xAA_b);
    CHECK(metadataPdu.destinationFileNameValue[1] == 0xAA_b);

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


TEST_CASE("Adding NackPdu")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tc::maxPduDataLength>{};
    auto nackPdu = sts1cobcsw::NackPdu{};

    nackPdu.startOfScope = 0;
    nackPdu.endOfScope = 0;
    static constexpr auto segmentRequests = std::array<std::uint64_t, 2>{0xAB, 0xCD};
    nackPdu.segmentRequests = segmentRequests;

    CHECK(nackPdu.Size() == 24U);  // NOLINT(*magic-numbers)

    auto addResult = nackPdu.AddTo(&dataField);
    REQUIRE(addResult.has_value());

    CHECK(dataField.size() == nackPdu.Size());
    CHECK(dataField[0] == 0x00_b);
    CHECK(dataField[1] == 0x00_b);
    CHECK(dataField[2] == 0x00_b);
    CHECK(dataField[3] == 0x00_b);
    CHECK(dataField[4] == 0x00_b);
    CHECK(dataField[5] == 0x00_b);
    CHECK(dataField[6] == 0x00_b);
    CHECK(dataField[7] == 0x00_b);

    CHECK(dataField[8] == 0xAB_b);
    CHECK(dataField[9] == 0x00_b);
    CHECK(dataField[10] == 0x00_b);
    CHECK(dataField[11] == 0x00_b);
    CHECK(dataField[12] == 0x00_b);
    CHECK(dataField[13] == 0x00_b);
    CHECK(dataField[14] == 0x00_b);
    CHECK(dataField[15] == 0x00_b);

    CHECK(dataField[16] == 0xCD_b);
    CHECK(dataField[17] == 0x00_b);
    CHECK(dataField[18] == 0x00_b);
    CHECK(dataField[19] == 0x00_b);
    CHECK(dataField[20] == 0x00_b);
    CHECK(dataField[21] == 0x00_b);
    CHECK(dataField[22] == 0x00_b);
    CHECK(dataField[23] == 0x00_b);
}

TEST_CASE("Parsing NackPdu")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    buffer.resize(24U);

    buffer[0] = 0x00_b;
    buffer[1] = 0x00_b;
    buffer[2] = 0x00_b;
    buffer[3] = 0x00_b;
    buffer[4] = 0x12_b;
    buffer[5] = 0x02_b;
    buffer[6] = 0xAA_b;
    buffer[7] = 0xAA_b;
    buffer[8] = 0x02_b;
    buffer[9] = 0xAA_b;
    buffer[10] = 0xAA_b;
    buffer[11] = 0xAA_b;
    buffer[12] = 0xAA_b;
    buffer[13] = 0xAA_b;
    buffer[14] = 0xAA_b;
    buffer[15] = 0xAA_b;

    auto parseResult = sts1cobcsw::ParseAsNackPdu(buffer);
    REQUIRE(parseResult.has_value());

    auto & nackPdu = parseResult.value();

    CHECK(nackPdu.startOfScope == 0x0000);
    CHECK(nackPdu.endOfScope == 0x1202'AAAA);
    CHECK(nackPdu.segmentRequests[0] == 0xAAAA'AAAA'AAAA'AA02U);
}

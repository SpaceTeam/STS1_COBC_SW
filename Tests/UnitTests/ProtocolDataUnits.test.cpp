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

#include <span>


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::operator""_b;


TEST_CASE("Parsing Protocol Data Units")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPduLength>{};
    buffer.resize(sts1cobcsw::tc::pduHeaderLength);
    buffer[0] = 0b0010'0100_b;  // Version, PDU type, direction, transmission mode, CRC flag, large
                                // file flag
    buffer[1] = 0x00_b;         // PDU data field length (high byte)
    buffer[2] = 0x00_b;         // PDU data field length (low byte)
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
    CHECK(pdu.header.pduDataFieldLength == 0);
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

    // TODO: Add tests for invalid PDU headers
}

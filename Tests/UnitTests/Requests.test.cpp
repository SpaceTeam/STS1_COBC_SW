#include "Sts1CobcSw/RfProtocols/Requests.hpp"

#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include "Sts1CobcSw/RfProtocols/Configuration.hpp"


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::operator""_b;


TEST_CASE("Parsing Request")
{
    using sts1cobcsw::tc::SpacePacketSecondaryHeader;
    using sts1cobcsw::operator""_b;

    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(sts1cobcsw::tc::packetSecondaryHeaderLength);
    buffer[0] = 0x22_b;        // Version number, ack. flags
    buffer[1] = 0x06_b;        // Service type ID
    buffer[2] = 0x05_b;        // Message subtype ID
    buffer[3] = 0xAA_b;        // Source ID (high byte)
    buffer[4] = 0x33_b;        // Source ID (low byte)
    buffer.push_back(0xAA_b);  // DataField

    auto parseResult = sts1cobcsw::ParseAsRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.secondaryHeader.tcPacketPusVersionNumber == 0x2);
    CHECK(request.secondaryHeader.acknowledgementFlags == 0x2);
    CHECK(request.secondaryHeader.messageTypeId.Value().serviceTypeId == 0x06);
    CHECK(request.secondaryHeader.messageTypeId.Value().messageSubtypeId == 0x05);
    CHECK(request.secondaryHeader.sourceId.Value() == 0xAA33);
    CHECK(request.dataField[0] == 0xAA_b);

    buffer[0] = 0x12_b;  // Version number 0x01 is invalid
    parseResult = sts1cobcsw::ParseAsRequest(buffer);
    CHECK(parseResult.has_error());
    buffer[0] = 0x22_b;

    buffer[1] = 0xAA_b;  // Service type ID 0xAA is invalid
    parseResult = sts1cobcsw::ParseAsRequest(buffer);
    CHECK(parseResult.has_error());
    buffer[1] = 0x06_b;

    buffer[3] = 0xBB_b;  // Source ID 0xBB33 is invalid
    parseResult = sts1cobcsw::ParseAsRequest(buffer);
    CHECK(parseResult.has_error());

    // Minimum buffer size needs to be sts1cobcsw::packetPrimaryHeaderLength
    auto smallBuffer = etl::vector<Byte, sts1cobcsw::packetPrimaryHeaderLength - 1>{};
    buffer.resize(sts1cobcsw::packetPrimaryHeaderLength - 1);
    parseResult = sts1cobcsw::ParseAsRequest(buffer);
    CHECK(parseResult.has_error());
}

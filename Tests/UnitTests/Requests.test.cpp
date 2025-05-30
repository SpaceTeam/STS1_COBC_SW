#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Requests.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::operator""_b;


TEST_CASE("Parsing Request")
{
    using sts1cobcsw::tc::SpacePacketSecondaryHeader;

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

    CHECK(request.applicationData[0] == 0xAA_b);

    buffer[0] = 0x12_b;  // Version number 0x01 is invalid
    parseResult = sts1cobcsw::ParseAsRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidSpacePacket);
    buffer[0] = 0x22_b;

    buffer[1] = 0xAA_b;  // Service type ID 0xAA is invalid
    parseResult = sts1cobcsw::ParseAsRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidMessageTypeId);
    buffer[1] = 0x06_b;

    buffer[3] = 0xBB_b;  // Source ID 0xBB33 is invalid
    parseResult = sts1cobcsw::ParseAsRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidSourceId);

    // Minimum buffer size needs to be sts1cobcsw::tc::packetSecondaryHeaderLength
    auto smallBuffer = etl::vector<Byte, sts1cobcsw::tc::packetSecondaryHeaderLength - 1>{};
    smallBuffer.resize(sts1cobcsw::tc::packetSecondaryHeaderLength - 1);
    parseResult = sts1cobcsw::ParseAsRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}

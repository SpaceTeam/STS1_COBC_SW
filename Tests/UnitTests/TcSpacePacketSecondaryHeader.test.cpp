#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>


TEST_CASE("Deserialize TC Space Packet secondary header")
{
    using sts1cobcsw::tc::SpacePacketSecondaryHeader;
    using sts1cobcsw::operator""_b;

    auto buffer = sts1cobcsw::SerialBuffer<SpacePacketSecondaryHeader>{};
    buffer[0] = 0x12_b;  // Version number, ack. flags
    buffer[1] = 0x34_b;  // Service type ID
    buffer[2] = 0x56_b;  // Message subtype ID
    buffer[3] = 0x78_b;  // Source ID (high byte)
    buffer[4] = 0x9A_b;  // Source ID (low byte)
    auto header =
        sts1cobcsw::Deserialize<sts1cobcsw::ccsdsEndianness, SpacePacketSecondaryHeader>(buffer);
    CHECK(header.tcPacketPusVersionNumber == 0x1);
    CHECK(header.acknowledgementFlags == 0x2);
    CHECK(header.messageTypeId.Value().serviceTypeId == 0x34);
    CHECK(header.messageTypeId.Value().messageSubtypeId == 0x56);
    CHECK(header.sourceId.Value() == 0x789A);
}

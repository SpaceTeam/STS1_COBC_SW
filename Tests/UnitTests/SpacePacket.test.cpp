#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/SpacePacket.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <cstdint>
#include <span>


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::operator""_b;


class TestPayload : public sts1cobcsw::Payload
{
public:
    TestPayload(Byte fillValue, std::uint16_t size) : fillValue_(fillValue), size_(size)
    {
    }


private:
    Byte fillValue_;
    std::uint16_t size_;

    auto DoWriteTo(etl::ivector<Byte> * dataField) const -> void override
    {
        dataField->resize(dataField->size() + size_, fillValue_);
    }


    [[nodiscard]] auto DoSize() const -> std::uint16_t override
    {
        return size_;
    }
};


TEST_CASE("Adding Space Packets")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::maxPacketLength>{};
    auto payload = TestPayload(0xAB_b, 10);
    auto addSpacePacketResult =
        sts1cobcsw::AddSpacePacketTo(&dataField, false, sts1cobcsw::normalApid, payload);
    CHECK(addSpacePacketResult.has_error() == false);
    CHECK(dataField.size() == payload.Size() + sts1cobcsw::packetPrimaryHeaderLength);
    CHECK(dataField[0] == 0b0000'0000_b);  // Version number, packet type, sec. header flag, APID
    CHECK(dataField[1] == 0b1100'1100_b);  // APID
    CHECK(dataField[2] == 0b1100'0000_b);  // Sequence flags, packet sequence count
    CHECK(dataField[3] == 0_b);            // Packet sequence count
    CHECK(dataField[4] == 0_b);            // Packet data length (high byte)
    CHECK(dataField[5] == 9_b);            // Packet data length (low byte)
    auto packetDataField = std::span(dataField).subspan(sts1cobcsw::packetPrimaryHeaderLength);
    for(auto byte : packetDataField)
    {
        CHECK(byte == 0xAB_b);
    }

    dataField.clear();
    payload = TestPayload(0x47_b, 13);
    addSpacePacketResult =
        sts1cobcsw::AddSpacePacketTo(&dataField, true, sts1cobcsw::normalApid, payload);
    CHECK(addSpacePacketResult.has_error() == false);
    CHECK(dataField.size() == payload.Size() + sts1cobcsw::packetPrimaryHeaderLength);
    CHECK(dataField[0] == 0b0000'1000_b);  // Version number, packet type, sec. header flag, APID
    CHECK(dataField[1] == 0b1100'1100_b);  // APID
    CHECK(dataField[2] == 0b1100'0000_b);  // Sequence flags, packet sequence count
    CHECK(dataField[3] == 1_b);            // Packet sequence count
    CHECK(dataField[4] == 0_b);            // Packet data length (high byte)
    CHECK(dataField[5] == 12_b);           // Packet data length (low byte)
    packetDataField = std::span(dataField).subspan(sts1cobcsw::packetPrimaryHeaderLength);
    for(auto byte : packetDataField)
    {
        CHECK(byte == 0x47_b);
    }

    dataField.clear();
    payload = TestPayload(0xA1_b, 5);
    addSpacePacketResult =
        sts1cobcsw::AddSpacePacketTo(&dataField, false, sts1cobcsw::idlePacketApid, payload);
    CHECK(addSpacePacketResult.has_error() == false);
    CHECK(dataField.size() == payload.Size() + sts1cobcsw::packetPrimaryHeaderLength);
    CHECK(dataField[0] == 0b0000'0111_b);  // Version number, packet type, sec. header flag, APID
    CHECK(dataField[1] == 0b1111'1111_b);  // APID
    CHECK(dataField[2] == 0b1100'0000_b);  // Sequence flags, packet sequence count
    CHECK(dataField[3] == 0_b);            // Packet sequence count
    CHECK(dataField[4] == 0_b);            // Packet data length (high byte)
    CHECK(dataField[5] == 4_b);            // Packet data length (low byte)
    packetDataField = std::span(dataField).subspan(sts1cobcsw::packetPrimaryHeaderLength);
    for(auto byte : packetDataField)
    {
        CHECK(byte == 0xA1_b);
    }

    dataField.clear();
    payload = TestPayload(0xFF_b, 0);
    addSpacePacketResult =
        sts1cobcsw::AddSpacePacketTo(&dataField, false, sts1cobcsw::normalApid, payload);
    CHECK(addSpacePacketResult.has_error());
    CHECK(addSpacePacketResult.error() == ErrorCode::invalidPayload);
    CHECK(dataField.size() == 0U);

    dataField.clear();
    payload = TestPayload(0xFF_b, sts1cobcsw::maxPacketDataLength + 1);
    addSpacePacketResult =
        sts1cobcsw::AddSpacePacketTo(&dataField, false, sts1cobcsw::normalApid, payload);
    CHECK(addSpacePacketResult.has_error());
    CHECK(addSpacePacketResult.error() == ErrorCode::tooLarge);
    CHECK(dataField.size() == 0U);
}


TEST_CASE("Parsing Space Packets")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::maxPacketLength>{};
    buffer.resize(sts1cobcsw::packetPrimaryHeaderLength);
    buffer[0] = 0b0001'1000_b;  // Version number, packet type, sec. header flag, APID
    buffer[1] = 0b1100'1100_b;  // APID
    buffer[2] = 0b1100'0000_b;  // Sequence flags, packet sequence count
    buffer[3] = 123_b;          // Packet sequence count
    buffer[4] = 0_b;            // Packet data length (high byte)
    buffer[5] = 0_b;            // Packet data length (low byte)
    buffer.push_back(0xAB_b);
    auto parseResult = sts1cobcsw::ParseAsSpacePacket(buffer);
    CHECK(parseResult.has_value());
    auto packet = parseResult.value();
    CHECK(packet.primaryHeader.versionNumber == sts1cobcsw::packetVersionNumber);
    CHECK(packet.primaryHeader.packetType == sts1cobcsw::PacketType::telecommand);
    CHECK(packet.primaryHeader.secondaryHeaderFlag == 1);
    CHECK(packet.primaryHeader.apid == sts1cobcsw::normalApid);
    CHECK(packet.primaryHeader.sequenceFlags == 0b11);
    CHECK(packet.primaryHeader.packetSequenceCount == 123);
    CHECK(packet.primaryHeader.packetDataLength == 0);
    CHECK(packet.dataField.size() == 1U);
    CHECK(packet.dataField[0] == 0xAB_b);

    buffer[0] = 0b0001'0111_b;  // Version number, packet type, sec. header flag, APID
    buffer[1] = 0b1111'1111_b;  // APID
    buffer[2] = 0b1100'0100_b;  // Sequence flags, packet sequence count
    buffer[3] = 26_b;           // Packet sequence count
    buffer[4] = 0_b;            // Packet data length (high byte)
    buffer[5] = 1_b;            // Packet data length (low byte)
    buffer.push_back(0x12_b);
    parseResult = sts1cobcsw::ParseAsSpacePacket(buffer);
    CHECK(parseResult.has_value());
    packet = parseResult.value();
    CHECK(packet.primaryHeader.versionNumber == sts1cobcsw::packetVersionNumber);
    CHECK(packet.primaryHeader.packetType == sts1cobcsw::PacketType::telecommand);
    CHECK(packet.primaryHeader.secondaryHeaderFlag == 0);
    CHECK(packet.primaryHeader.apid == sts1cobcsw::idlePacketApid);
    CHECK(packet.primaryHeader.sequenceFlags == 0b11);
    CHECK(packet.primaryHeader.packetSequenceCount == 1024 + 26);
    CHECK(packet.primaryHeader.packetDataLength == 1);
    CHECK(packet.dataField.size() == 2U);
    CHECK(packet.dataField[0] == 0xAB_b);
    CHECK(packet.dataField[1] == 0x12_b);

    // Wrong version number
    buffer[0] = 0b0101'0111_b;  // Version number, packet type, sec. header flag, APID
    buffer[1] = 0b1111'1111_b;  // APID
    parseResult = sts1cobcsw::ParseAsSpacePacket(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidSpacePacket);

    // Wrong packet type
    buffer[0] = 0b0000'0111_b;  // Version number, packet type, sec. header flag, APID
    buffer[1] = 0b1111'1111_b;  // APID
    parseResult = sts1cobcsw::ParseAsSpacePacket(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidSpacePacket);

    // Wrong APID
    buffer[0] = 0b0001'0000_b;  // Version number, packet type, sec. header flag, APID
    buffer[1] = 0b0000'0000_b;  // APID
    parseResult = sts1cobcsw::ParseAsSpacePacket(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidSpacePacket);

    // Wrong sequence flags
    buffer[0] = 0b0001'0111_b;  // Version number, packet type, sec. header flag, APID
    buffer[1] = 0b1111'1111_b;  // APID
    buffer[2] = 0b0000'0000_b;  // Sequence flags, packet sequence count
    parseResult = sts1cobcsw::ParseAsSpacePacket(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidSpacePacket);

    // Wrong packet data length
    buffer[2] = 0b1100'0000_b;  // Sequence flags, packet sequence count
    buffer[4] = 0_b;            // Packet data length (high byte)
    buffer[5] = 17_b;           // Packet data length (low byte)
    parseResult = sts1cobcsw::ParseAsSpacePacket(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);

    // Buffer smaller than packet header
    buffer.resize(sts1cobcsw::packetPrimaryHeaderLength - 1);
    parseResult = sts1cobcsw::ParseAsSpacePacket(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}

#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/TcTransferFrame.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <array>
#include <span>


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::Span;
using sts1cobcsw::operator""_b;


TEST_CASE("Parsing TC Transfer Frames")
{
    auto buffer = std::array<Byte, sts1cobcsw::tc::transferFrameLength>{};
    buffer[0] = 0b0010'0001_b;  // Version number, bypass flag, control command flag, spare
    buffer[1] = 0x23_b;         // Spacecraft ID
    buffer[2] = 0b0000'1100_b;  // VCID, frame length
    buffer[3] = 222_b;          // Frame length
    buffer[4] = 13_b;           // Frame sequence number
    // Data field
    buffer[5 + sts1cobcsw::tc::securityHeaderLength] = 0xFF_b;
    buffer[6 + sts1cobcsw::tc::securityHeaderLength] = 0xEE_b;
    auto parseResult = sts1cobcsw::tc::ParseAsTransferFrame(Span(buffer));
    CHECK(parseResult.has_value());
    auto & frame = parseResult.value();
    CHECK(frame.primaryHeader.vcid == sts1cobcsw::pusVcid);
    CHECK(frame.primaryHeader.frameSequenceNumber == 13);
    CHECK(frame.dataField[0] == 0xFF_b);
    CHECK(frame.dataField[1] == 0xEE_b);
    CHECK(frame.dataField[2] == 0x00_b);

    buffer[0] = 0b1010'0001_b;  // Wrong version number
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(Span(buffer));
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidTransferFrame);

    buffer[0] = 0b0000'0001_b;  // Wrong bypass flag
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(Span(buffer));
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidTransferFrame);

    buffer[0] = 0b0011'1001_b;  // Wrong control command flag
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(Span(buffer));
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidTransferFrame);

    buffer[0] = 0b0010'0001_b;  // Correct version number, bypass flag, control command flag
    buffer[1] = 0x17_b;         // Wrong spacecraft ID
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(Span(buffer));
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidSpacecraftId);

    buffer[1] = 0x23_b;         // Correct spacecraft ID
    buffer[2] = 0b0001'1100_b;  // Wrong VCID
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(Span(buffer));
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidVcid);

    buffer[2] = 0b0000'1100_b;  // Correct VCID
    buffer[3] = 255_b;          // Frame length too large
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(Span(buffer));
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidFrameLength);
}

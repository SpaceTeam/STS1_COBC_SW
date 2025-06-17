#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/TcTransferFrame.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <span>


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::operator""_b;


TEST_CASE("Parsing TC Transfer Frames")
{
    auto buffer = std::array<Byte, sts1cobcsw::tc::transferFrameLength>{};
    buffer[0] = 0b0010'0001_b;  // Version number, bypass flag, control command flag, spare
    buffer[1] = 0x23_b;         // Spacecraft ID
    buffer[2] = 0b0000'1100_b;  // VCID, frame length
    buffer[3] = 222_b;          // Frame length
    buffer[4] = 13_b;           // Frame sequence number
    buffer[5] = 0x00_b;         // Security parameter index (high byte)
    buffer[6] = 0x01_b;         // Security parameter index (low byte)
    // Data field
    buffer[7] = 0xFF_b;
    buffer[8] = 0xEE_b;
    buffer[sts1cobcsw::tc::transferFrameLength - 8] = 0x38_b;
    buffer[sts1cobcsw::tc::transferFrameLength - 7] = 0x34_b;
    buffer[sts1cobcsw::tc::transferFrameLength - 6] = 0x83_b;
    buffer[sts1cobcsw::tc::transferFrameLength - 5] = 0x8B_b;
    buffer[sts1cobcsw::tc::transferFrameLength - 4] = 0x28_b;
    buffer[sts1cobcsw::tc::transferFrameLength - 3] = 0xA3_b;
    buffer[sts1cobcsw::tc::transferFrameLength - 2] = 0xF6_b;
    buffer[sts1cobcsw::tc::transferFrameLength - 1] = 0x2C_b;
    auto parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_value());
    auto & frame = parseResult.value();
    CHECK(frame.primaryHeader.vcid == sts1cobcsw::pusVcid);
    CHECK(frame.primaryHeader.frameSequenceNumber == 13);
    CHECK(frame.dataField[0] == 0xFF_b);
    CHECK(frame.dataField[1] == 0xEE_b);
    CHECK(frame.dataField[2] == 0x00_b);

    buffer[0] = 0b1010'0001_b;  // Wrong version number
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidTransferFrame);

    buffer[0] = 0b0000'0001_b;  // Wrong bypass flag
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidTransferFrame);

    buffer[0] = 0b0011'1001_b;  // Wrong control command flag
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidTransferFrame);

    buffer[0] = 0b0010'0001_b;  // Correct version number, bypass flag, control command flag
    buffer[1] = 0x17_b;         // Wrong spacecraft ID
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidSpacecraftId);

    buffer[1] = 0x23_b;         // Correct spacecraft ID
    buffer[2] = 0b0001'1100_b;  // Wrong VCID
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidVcid);

    buffer[2] = 0b0000'1100_b;  // Correct VCID
    buffer[3] = 255_b;          // Frame length too large
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidFrameLength);

    buffer[3] = 222_b;   // Correct frame length
    buffer[5] = 0xFF_b;  // Wrong security parameter index
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::invalidSecurityParameterIndex);

    buffer[5] = 0x00_b;                                        // Correct security parameter index
    buffer[sts1cobcsw::tc::transferFrameLength - 1] = 0x00_b;  // Wrong authentication code
    parseResult = sts1cobcsw::tc::ParseAsTransferFrame(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == sts1cobcsw::ErrorCode::authenticationFailed);
}

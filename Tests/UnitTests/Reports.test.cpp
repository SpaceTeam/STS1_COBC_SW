#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Fram/FramMock.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <span>


using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::Span;
using sts1cobcsw::VerificationStage;

using sts1cobcsw::operator""_b;


TEST_INIT("Initialize FRAM for reports")
{
    sts1cobcsw::fram::ram::SetAllDoFunctions();
    sts1cobcsw::fram::ram::memory.fill(0x00_b);
    sts1cobcsw::fram::Initialize();
}


TEST_CASE("Successful verification reports")
{
    using sts1cobcsw::SuccessfulVerificationReport;

    auto dataField = etl::vector<Byte, sts1cobcsw::maxPacketDataLength>{};
    auto requestId = sts1cobcsw::RequestId{
        .packetVersionNumber = 0b111,
        .packetType = 0,
        .secondaryHeaderFlag = 1,
        .apid = sts1cobcsw::Apid(0),
        .sequenceFlags = 0b11,
        .packetSequenceCount = 0,
    };
    auto acceptanceReport = SuccessfulVerificationReport<VerificationStage::acceptance>(requestId);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto writeResult = acceptanceReport.WriteTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == acceptanceReport.Size());
    CHECK(acceptanceReport.Size() == sts1cobcsw::tm::packetSecondaryHeaderLength + 4);
    // Packet secondary header
    CHECK(dataField[0] == 0x20_b);  // PUS version number, spacecraft time reference status
    CHECK(dataField[1] == 1_b);     // Service type ID
    CHECK(dataField[2] == 1_b);     // Submessage type ID
    CHECK(dataField[3] == 0_b);     // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);     // Message type counter (low byte)
    CHECK(dataField[5] == 0xAA_b);  // Destination ID (high byte)
    CHECK(dataField[6] == 0x33_b);  // Destination ID (low byte)
    auto timestamp = sts1cobcsw::Deserialize<sts1cobcsw::ccsdsEndianness, sts1cobcsw::RealTime>(
        Span(dataField).subspan<7, 4>());
    CHECK(tBeforeWrite <= timestamp);
    CHECK(timestamp <= tAfterWrite);
    // Request ID
    CHECK(dataField[11] == 0b1110'1000_b);  // Version number, packet type, sec. header flag, APID
    CHECK(dataField[12] == 0_b);            // APID
    CHECK(dataField[13] == 0b1100'0000_b);  // Sequence flags, packet sequence count
    CHECK(dataField[14] == 0_b);            // Packet sequence count

    dataField.clear();
    writeResult = acceptanceReport.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 1_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);  // Message type counter (low byte)

    dataField.resize(dataField.capacity() - acceptanceReport.Size() + 1);
    CHECK(dataField.available() < acceptanceReport.Size());
    writeResult = acceptanceReport.WriteTo(&dataField);
    CHECK(writeResult.has_error());

    dataField.clear();
    requestId = sts1cobcsw::RequestId{
        .packetVersionNumber = 0,
        .packetType = 1,
        .secondaryHeaderFlag = 0,
        .apid = sts1cobcsw::Apid(0x7FF),
        .sequenceFlags = 0,
        .packetSequenceCount = 0x3FFF,
    };
    auto completionOfExecutionReport =
        SuccessfulVerificationReport<VerificationStage::completionOfExecution>(requestId);
    writeResult = completionOfExecutionReport.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == completionOfExecutionReport.Size());
    CHECK(completionOfExecutionReport.Size() == sts1cobcsw::tm::packetSecondaryHeaderLength + 4);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 7_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);  // Message type counter (low byte)
    // Request ID
    CHECK(dataField[11] == 0b0001'0111_b);  // Version number, packet type, sec. header flag, APID
    CHECK(dataField[12] == 0xFF_b);         // APID
    CHECK(dataField[13] == 0b0011'1111_b);  // Sequence flags, packet sequence count
    CHECK(dataField[14] == 0xFF_b);         // Packet sequence count

    dataField.clear();
    writeResult = completionOfExecutionReport.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 7_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);  // Message type counter (low byte)
}


TEST_CASE("Failed verification reports")
{
    using sts1cobcsw::FailedVerificationReport;

    auto dataField = etl::vector<Byte, sts1cobcsw::maxPacketDataLength>{};
    auto requestId = sts1cobcsw::RequestId{
        .packetVersionNumber = 0b111,
        .packetType = 0,
        .secondaryHeaderFlag = 1,
        .apid = sts1cobcsw::Apid(0),
        .sequenceFlags = 0b11,
        .packetSequenceCount = 0,
    };
    auto acceptanceReport = FailedVerificationReport<VerificationStage::acceptance>(
        requestId, ErrorCode::invalidSpacePacket);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto writeResult = acceptanceReport.WriteTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == acceptanceReport.Size());
    CHECK(acceptanceReport.Size() == sts1cobcsw::tm::packetSecondaryHeaderLength + 5);
    // Packet secondary header
    CHECK(dataField[0] == 0x20_b);  // PUS version number, spacecraft time reference status
    CHECK(dataField[1] == 1_b);     // Service type ID
    CHECK(dataField[2] == 2_b);     // Submessage type ID
    CHECK(dataField[3] == 0_b);     // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);     // Message type counter (low byte)
    CHECK(dataField[5] == 0xAA_b);  // Destination ID (high byte)
    CHECK(dataField[6] == 0x33_b);  // Destination ID (low byte)
    auto timestamp = sts1cobcsw::Deserialize<sts1cobcsw::ccsdsEndianness, sts1cobcsw::RealTime>(
        Span(dataField).subspan<7, 4>());
    CHECK(tBeforeWrite <= timestamp);
    CHECK(timestamp <= tAfterWrite);
    // Request ID
    CHECK(dataField[11] == 0b1110'1000_b);  // Version number, packet type, sec. header flag, APID
    CHECK(dataField[12] == 0_b);            // APID
    CHECK(dataField[13] == 0b1100'0000_b);  // Sequence flags, packet sequence count
    CHECK(dataField[14] == 0_b);            // Packet sequence count
    // Error code
    CHECK(dataField[15] == static_cast<Byte>(ErrorCode::invalidSpacePacket));

    dataField.clear();
    writeResult = acceptanceReport.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 2_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);  // Message type counter (low byte)

    dataField.clear();
    requestId = sts1cobcsw::RequestId{
        .packetVersionNumber = 0,
        .packetType = 1,
        .secondaryHeaderFlag = 0,
        .apid = sts1cobcsw::Apid(0x7FF),
        .sequenceFlags = 0,
        .packetSequenceCount = 0x3FFF,
    };
    auto completionOfExecutionReport =
        FailedVerificationReport<VerificationStage::completionOfExecution>(requestId,
                                                                           ErrorCode::timeout);
    writeResult = completionOfExecutionReport.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == completionOfExecutionReport.Size());
    CHECK(completionOfExecutionReport.Size() == sts1cobcsw::tm::packetSecondaryHeaderLength + 5);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 8_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);  // Message type counter (low byte)
    // Request ID
    CHECK(dataField[11] == 0b0001'0111_b);  // Version number, packet type, sec. header flag, APID
    CHECK(dataField[12] == 0xFF_b);         // APID
    CHECK(dataField[13] == 0b0011'1111_b);  // Sequence flags, packet sequence count
    CHECK(dataField[14] == 0xFF_b);         // Packet sequence count
    // Error code
    CHECK(dataField[15] == static_cast<Byte>(ErrorCode::timeout));

    dataField.clear();
    writeResult = completionOfExecutionReport.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 8_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);  // Message type counter (low byte)
}

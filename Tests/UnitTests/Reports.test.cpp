#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
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
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <etl/string.h>
#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>


namespace fs = sts1cobcsw::fs;

using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;
using sts1cobcsw::RealTime;
using sts1cobcsw::Span;
using sts1cobcsw::totalSerialSize;
using sts1cobcsw::VerificationStage;

using sts1cobcsw::operator""_b;


namespace
{
template<typename T, std::size_t offset, std::size_t size>
    requires(offset + totalSerialSize<T> <= size)
[[nodiscard]] auto Deserialize(etl::vector<Byte, size> const & vector) -> T;
}


TEST_INIT("Initialize FRAM for reports")
{
    sts1cobcsw::fram::ram::SetAllDoFunctions();
    sts1cobcsw::fram::ram::memory.fill(0x00_b);
    sts1cobcsw::fram::Initialize();
}


TEST_CASE("Successful verification reports")
{
    using sts1cobcsw::SuccessfulVerificationReport;

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
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
    auto timestamp = Deserialize<RealTime, 7>(dataField);
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

    dataField.resize(dataField.MAX_SIZE - acceptanceReport.Size() + 1);
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

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
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
    auto timestamp = Deserialize<RealTime, 7>(dataField);
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


TEST_CASE("Housekeeping parameter report")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto record = sts1cobcsw::TelemetryRecord{
        .eduShouldBePowered = true,
        .eduIsAlive = true,
        .newEduResultIsAvailable = true,
        .antennasShouldBeDeployed = true,
        .framIsWorking = true,
        .epsIsWorking = true,
        .flashIsWorking = true,
        .rfIsWorking = true,
        .lastTelecommandIdWasInvalid = true,
        .lastTelecommandArgumentsWereInvalid = true,
        .nTotalResets = 1U,
        .nResetsSinceRf = 2U,
        .activeSecondaryFwPartition = 3,
        .backupSecondaryFwPartition = 4,
        .eduProgramQueueIndex = 5U,
        .programIdOfCurrentEduProgramQueueEntry = sts1cobcsw::ProgramId(6),
        .nEduCommunicationErrors = 7U,
        .lastResetReason = 8U,
        .rodosTimeInSeconds = 9,
        .realTime = RealTime(10),
        .nFirmwareChecksumErrors = 11U,
        .nFlashErrors = 12U,
        .nRfErrors = 13U,
        .nFileSystemErrors = 14U,
        .cobcTemperature = 15,
        .rfTemperature = 16,
 // clang-format off
        .epsAdcData = {
            .adc4 = {
                17U, 18U, 19U, 20U, 21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U, 32U},
            .adc5 = {33U, 34U, 35U, 36U, 37U, 38U, 39U, 40U, 41U, 42U},
            .adc6 = {43U, 44U, 45U, 46U, 47U, 48U, 49U, 50U, 51U, 52U}},
  // clang-format on
        .rxBaudRate = 53,
        .txBaudRate = 54,
        .nCorrectableUplinkErrors = 55U,
        .nUncorrectableUplinkErrors = 56U,
        .nGoodTransferFrames = 57U,
        .nBadTransferFrames = 58U,
        .lastFrameSequenceNumber = 59U,
        .lastTelecommandId = 60U
    };
    auto report = sts1cobcsw::HousekeepingParameterReport(record);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto writeResult = report.WriteTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == sts1cobcsw::tm::packetSecondaryHeaderLength + 1 + totalSerialSize<decltype(record)>);
    // Packet secondary header
    CHECK(dataField[0] == 0x20_b);  // PUS version number, spacecraft time reference status
    CHECK(dataField[1] == 3_b);     // Service type ID
    CHECK(dataField[2] == 25_b);    // Submessage type ID
    CHECK(dataField[3] == 0_b);     // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);     // Message type counter (low byte)
    CHECK(dataField[5] == 0xAA_b);  // Destination ID (high byte)
    CHECK(dataField[6] == 0x33_b);  // Destination ID (low byte)
    auto timestamp = Deserialize<RealTime, 7>(dataField);
    CHECK(tBeforeWrite <= timestamp);
    CHECK(timestamp <= tAfterWrite);
    // Structure ID
    CHECK(dataField[11] == 0_b);
    // Telemetry record
    CHECK(dataField[12] == 0b1111'1111_b);
    CHECK(dataField[13] == 0b0000'0011_b);
    CHECK(dataField[14] == 0_b);
    CHECK(dataField[15] == 0_b);
    CHECK(dataField[16] == 0_b);
    CHECK(dataField[17] == 1_b);         // nTotalResets
    CHECK(dataField[11 + 121] == 60_b);  // lastTelecommandId

    dataField.clear();
    writeResult = report.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 3_b);   // Service type ID
    CHECK(dataField[2] == 25_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);   // Message type counter (low byte)
}


TEST_CASE("Parameter value report")
{
    using sts1cobcsw::ParameterId;
    using sts1cobcsw::ParameterValue;
    using sts1cobcsw::ParameterValueReport;

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto parameterId = ParameterId::rxBaudRate;
    auto parameterValue = 9600U;
    auto report = ParameterValueReport(parameterId, parameterValue);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto writeResult = report.WriteTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength + 1
              + totalSerialSize<ParameterId, ParameterValue>));
    // Packet secondary header
    CHECK(dataField[0] == 0x20_b);  // PUS version number, spacecraft time reference status
    CHECK(dataField[1] == 20_b);    // Service type ID
    CHECK(dataField[2] == 2_b);     // Submessage type ID
    CHECK(dataField[3] == 0_b);     // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);     // Message type counter (low byte)
    CHECK(dataField[5] == 0xAA_b);  // Destination ID (high byte)
    CHECK(dataField[6] == 0x33_b);  // Destination ID (low byte)
    auto timestamp = Deserialize<RealTime, 7>(dataField);
    CHECK(tBeforeWrite <= timestamp);
    CHECK(timestamp <= tAfterWrite);
    // Number of parameters
    CHECK(dataField[11] == 1_b);
    // Parameters
    CHECK(dataField[12] == static_cast<Byte>(parameterId));
    CHECK(parameterValue == (Deserialize<ParameterValue, 13>(dataField)));

    dataField.clear();
    auto parameterIds = etl::vector<ParameterId, sts1cobcsw::maxNParameters>{
        ParameterId::rxBaudRate, ParameterId::txBaudRate, ParameterId::eduStartDelayLimit};
    auto parameterValues =
        etl::vector<ParameterValue, sts1cobcsw::maxNParameters>{9600U, 115200U, 17U};
    report = ParameterValueReport(parameterIds, parameterValues);
    writeResult = report.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength + 1
              + parameterIds.size() * totalSerialSize<ParameterId, ParameterValue>));
    // Packet secondary header
    CHECK(dataField[1] == 20_b);  // Service type ID
    CHECK(dataField[2] == 2_b);   // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);   // Message type counter (low byte)
    // Number of parameters
    CHECK(dataField[11] == 3_b);
    // Parameters
    CHECK(dataField[12] == static_cast<Byte>(ParameterId::rxBaudRate));
    CHECK(parameterValues[0] == (Deserialize<ParameterValue, 13>(dataField)));
    CHECK(dataField[17] == static_cast<Byte>(ParameterId::txBaudRate));
    CHECK(parameterValues[1] == (Deserialize<ParameterValue, 18>(dataField)));
    CHECK(dataField[22] == static_cast<Byte>(ParameterId::eduStartDelayLimit));
    CHECK(parameterValues[2] == (Deserialize<ParameterValue, 23>(dataField)));

    dataField.clear();
    writeResult = report.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 20_b);  // Service type ID
    CHECK(dataField[2] == 2_b);   // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 2_b);   // Message type counter (low byte)
}


TEST_CASE("File attribute report")
{
    using sts1cobcsw::FileAttributeReport;
    using sts1cobcsw::FileStatus;

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto filePath = fs::Path("/results/12345_67890.zip");
    auto fileSize = 0xDEADBEEFU;
    auto fileStatus = FileStatus::locked;
    auto report = FileAttributeReport(filePath, fileSize, fileStatus);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto writeResult = report.WriteTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength + fs::Path::MAX_SIZE
              + totalSerialSize<decltype(fileSize), decltype(fileStatus)>));
    // Packet secondary header
    CHECK(dataField[0] == 0x20_b);  // PUS version number, spacecraft time reference status
    CHECK(dataField[1] == 23_b);    // Service type ID
    CHECK(dataField[2] == 4_b);     // Submessage type ID
    CHECK(dataField[3] == 0_b);     // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);     // Message type counter (low byte)
    CHECK(dataField[5] == 0xAA_b);  // Destination ID (high byte)
    CHECK(dataField[6] == 0x33_b);  // Destination ID (low byte)
    auto timestamp = Deserialize<RealTime, 7>(dataField);
    CHECK(tBeforeWrite <= timestamp);
    CHECK(timestamp <= tAfterWrite);
    // File path
    CHECK(std::equal(filePath.begin(),
                     filePath.end(),
                     dataField.begin() + 11,
                     [](char c, Byte b) { return c == static_cast<char>(b); }));
    // File size
    CHECK(fileSize == (Deserialize<std::uint32_t, 11 + fs::Path::MAX_SIZE>(dataField)));
    // File status
    static constexpr auto iFileStatus =
        11 + fs::Path::MAX_SIZE + totalSerialSize<decltype(fileSize)>;
    CHECK(fileStatus == (Deserialize<FileStatus, iFileStatus>(dataField)));

    dataField.clear();
    writeResult = report.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 23_b);  // Service type ID
    CHECK(dataField[2] == 4_b);   // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);   // Message type counter (low byte)
}


TEST_CASE("Repository content summary report")
{
    using sts1cobcsw::ObjectType;
    using sts1cobcsw::RepositoryContentSummaryReport;

    static constexpr auto maxNObjectsPerPacket =
        RepositoryContentSummaryReport::maxNObjectsPerPacket;

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto repositoryPath = fs::Path("/programs");
    auto objectTypes = etl::vector<ObjectType, maxNObjectsPerPacket>{
        ObjectType::directory,
        ObjectType::directory,
        ObjectType::file,
        ObjectType::file,
    };
    auto objectNames = etl::vector<fs::Path, maxNObjectsPerPacket>{
        fs::Path("/programs/."),
        fs::Path("/programs/.."),
        fs::Path("/programs/00001"),
        fs::Path("/programs/00001.lock"),
    };
    auto nObjects = static_cast<std::uint8_t>(objectTypes.size());
    auto report =
        RepositoryContentSummaryReport(repositoryPath, nObjects, objectTypes, objectNames);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto writeResult = report.WriteTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(writeResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength + fs::Path::MAX_SIZE
              + totalSerialSize<decltype(nObjects)>
              + nObjects * (totalSerialSize<ObjectType> + fs::Path::MAX_SIZE)));
    // Packet secondary header
    CHECK(dataField[0] == 0x20_b);  // PUS version number, spacecraft time reference status
    CHECK(dataField[1] == 23_b);    // Service type ID
    CHECK(dataField[2] == 13_b);    // Submessage type ID
    CHECK(dataField[3] == 0_b);     // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);     // Message type counter (low byte)
    CHECK(dataField[5] == 0xAA_b);  // Destination ID (high byte)
    CHECK(dataField[6] == 0x33_b);  // Destination ID (low byte)
    auto timestamp = Deserialize<RealTime, 7>(dataField);
    CHECK(tBeforeWrite <= timestamp);
    CHECK(timestamp <= tAfterWrite);
    // Repository path
    CHECK(std::equal(repositoryPath.begin(),
                     repositoryPath.end(),
                     dataField.begin() + 11,
                     [](char c, Byte b) { return c == static_cast<char>(b); }));
    // Number of objects
    CHECK(dataField[11 + fs::Path::MAX_SIZE] == static_cast<Byte>(nObjects));
    // Object types and names
    for(auto i = 0U; i < objectTypes.size(); ++i)
    {
        CHECK(dataField[47 + 36 * i] == static_cast<Byte>(objectTypes[i]));
        CHECK(std::equal(objectNames[i].begin(),
                         objectNames[i].end(),
                         dataField.begin() + 48 + 36 * i,
                         [](char c, Byte b) { return c == static_cast<char>(b); }));
    }

    dataField.clear();
    writeResult = report.WriteTo(&dataField);
    CHECK(writeResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 23_b);  // Service type ID
    CHECK(dataField[2] == 13_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);   // Message type counter (low byte)
}


namespace
{
template<typename T, std::size_t offset, std::size_t size>
    requires(offset + totalSerialSize<T> <= size)
auto Deserialize(etl::vector<Byte, size> const & vector) -> T
{
    return sts1cobcsw::Deserialize<sts1cobcsw::ccsdsEndianness, T>(
        Span(vector).template subspan<offset, totalSerialSize<T>>());
}
}

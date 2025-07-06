#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Fram/FramMock.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>
#include <Sts1CobcSw/RfProtocols/SpacePacket.hpp>
#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>


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
    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto requestId = sts1cobcsw::RequestId{
        .packetVersionNumber = 0b111,
        .packetType = sts1cobcsw::PacketType(0),
        .secondaryHeaderFlag = 1,
        .apid = sts1cobcsw::Apid(0),
        .sequenceFlags = 0b11,
        .packetSequenceCount = 0,
    };
    auto acceptanceReport = sts1cobcsw::SuccessfulAcceptanceVerificationReport(requestId);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto addToResult = acceptanceReport.AddTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(addToResult.has_error() == false);
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
    addToResult = acceptanceReport.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 1_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);  // Message type counter (low byte)

    dataField.resize(dataField.MAX_SIZE - acceptanceReport.Size() + 1);
    CHECK(dataField.available() < acceptanceReport.Size());
    addToResult = acceptanceReport.AddTo(&dataField);
    CHECK(addToResult.has_error());

    dataField.clear();
    requestId = sts1cobcsw::RequestId{
        .packetVersionNumber = 0,
        .packetType = sts1cobcsw::PacketType(1),
        .secondaryHeaderFlag = 0,
        .apid = sts1cobcsw::Apid(0x7FF),
        .sequenceFlags = 0,
        .packetSequenceCount = 0x3FFF,
    };
    auto completionOfExecutionReport =
        sts1cobcsw::SuccessfulCompletionOfExecutionVerificationReport(requestId);
    addToResult = completionOfExecutionReport.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
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
    addToResult = completionOfExecutionReport.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 7_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);  // Message type counter (low byte)
}


TEST_CASE("Failed verification reports")
{
    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto requestId = sts1cobcsw::RequestId{
        .packetVersionNumber = 0b111,
        .packetType = sts1cobcsw::PacketType(0),
        .secondaryHeaderFlag = 1,
        .apid = sts1cobcsw::Apid(0),
        .sequenceFlags = 0b11,
        .packetSequenceCount = 0,
    };
    auto acceptanceReport =
        sts1cobcsw::FailedAcceptanceVerificationReport(requestId, ErrorCode::invalidSpacePacket);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto addToResult = acceptanceReport.AddTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(addToResult.has_error() == false);
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
    addToResult = acceptanceReport.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 1_b);  // Service type ID
    CHECK(dataField[2] == 2_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);  // Message type counter (low byte)

    dataField.clear();
    requestId = sts1cobcsw::RequestId{
        .packetVersionNumber = 0,
        .packetType = sts1cobcsw::PacketType(1),
        .secondaryHeaderFlag = 0,
        .apid = sts1cobcsw::Apid(0x7FF),
        .sequenceFlags = 0,
        .packetSequenceCount = 0x3FFF,
    };
    auto completionOfExecutionReport =
        sts1cobcsw::FailedCompletionOfExecutionVerificationReport(requestId, ErrorCode::timeout);
    addToResult = completionOfExecutionReport.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
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
    addToResult = completionOfExecutionReport.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
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
        .eduShouldBePowered = 1,
        .eduIsAlive = 1,
        .newEduResultIsAvailable = 1,
        .dosimeterIsPowered = 1,
        .antennasShouldBeDeployed = 1,
        .epsIsCharging = 1,
        .epsDetectedFault = 1,
        .framIsWorking = 1,
        .epsIsWorking = 1,
        .flashIsWorking = 1,
        .rfIsWorking = 1,
        .lastMessageTypeIdWasInvalid = 1,
        .lastApplicationDataWasInvalid = 1,
        .nTotalResets = 1U,
        .nResetsSinceRf = 2U,
        .activeSecondaryFwPartition = sts1cobcsw::fw::PartitionId::secondary1,
        .backupSecondaryFwPartition = sts1cobcsw::fw::PartitionId::secondary2,
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
        .rxDataRate = 53,
        .txDataRate = 54,
        .nCorrectableUplinkErrors = 55U,
        .nUncorrectableUplinkErrors = 56U,
        .nGoodTransferFrames = 57U,
        .nBadTransferFrames = 58U,
        .lastFrameSequenceNumber = 59U,
        .lastMessageTypeId = {60U, 61}
    };
    auto report = sts1cobcsw::HousekeepingParameterReport(record);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto addToResult = report.AddTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(addToResult.has_error() == false);
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
    CHECK(dataField[13] == 0b1111'1000_b);
    CHECK(dataField[14] == 0_b);
    CHECK(dataField[15] == 0_b);
    CHECK(dataField[16] == 0_b);
    CHECK(dataField[17] == 1_b);         // nTotalResets
    CHECK(dataField[11 + 120] == 60_b);  // lastMessageTypeId
    CHECK(dataField[11 + 121] == 61_b);  // lastMessageTypeId

    dataField.clear();
    addToResult = report.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 3_b);   // Service type ID
    CHECK(dataField[2] == 25_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);   // Message type counter (low byte)
}


TEST_CASE("Parameter value report")
{
    using sts1cobcsw::Parameter;
    using sts1cobcsw::ParameterValueReport;

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto parameterId = Parameter::Id::rxDataRate;
    auto parameterValue = 9600U;
    auto report = ParameterValueReport(parameterId, parameterValue);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto addToResult = report.AddTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(addToResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength + 1
              + totalSerialSize<Parameter::Id, Parameter::Value>));
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
    CHECK(parameterValue == (Deserialize<Parameter::Value, 13>(dataField)));

    dataField.clear();
    auto parameters = etl::vector<sts1cobcsw::Parameter, sts1cobcsw::maxNParameters>{
        sts1cobcsw::Parameter{Parameter::Id::rxDataRate,         9600U   },
        sts1cobcsw::Parameter{Parameter::Id::txDataRate,         115'200U},
        sts1cobcsw::Parameter{Parameter::Id::maxEduIdleDuration, 17U     }
    };
    report = ParameterValueReport(parameters);
    addToResult = report.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength + 1
              + parameters.size() * totalSerialSize<Parameter::Id, Parameter::Value>));
    // Packet secondary header
    CHECK(dataField[1] == 20_b);  // Service type ID
    CHECK(dataField[2] == 2_b);   // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);   // Message type counter (low byte)
    // Number of parameters
    CHECK(dataField[11] == 3_b);
    // Parameters
    CHECK(dataField[12] == static_cast<Byte>(Parameter::Id::rxDataRate));
    CHECK(parameters[0].value == (Deserialize<Parameter::Value, 13>(dataField)));
    CHECK(dataField[17] == static_cast<Byte>(Parameter::Id::txDataRate));
    CHECK(parameters[1].value == (Deserialize<Parameter::Value, 18>(dataField)));
    CHECK(dataField[22] == static_cast<Byte>(Parameter::Id::maxEduIdleDuration));
    CHECK(parameters[2].value == (Deserialize<Parameter::Value, 23>(dataField)));

    dataField.clear();
    addToResult = report.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 20_b);  // Service type ID
    CHECK(dataField[2] == 2_b);   // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 2_b);   // Message type counter (low byte)
}


TEST_CASE("File attribute report")
{
    using sts1cobcsw::FileAttributeReport;
    using sts1cobcsw::LockState;

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto filePath = fs::Path("/results/12345_67890.zip");
    auto fileSize = 0xDEAD'BEEFU;
    auto lockState = LockState::locked;
    auto report = FileAttributeReport(filePath, fileSize, lockState);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto addToResult = report.AddTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(addToResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength
              + totalSerialSize<decltype(filePath), decltype(fileSize), decltype(lockState)>));
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
    static constexpr auto iLockState = 11 + totalSerialSize<decltype(filePath), decltype(fileSize)>;
    CHECK(lockState == (Deserialize<LockState, iLockState>(dataField)));

    dataField.clear();
    addToResult = report.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 23_b);  // Service type ID
    CHECK(dataField[2] == 4_b);   // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);   // Message type counter (low byte)
}


TEST_CASE("Repository content summary report")
{
    using sts1cobcsw::FileSystemObject;
    using sts1cobcsw::RepositoryContentSummaryReport;
    using Type = sts1cobcsw::FileSystemObject::Type;

    static constexpr auto maxNObjectsPerPacket =
        RepositoryContentSummaryReport::maxNObjectsPerPacket;

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    auto repositoryPath = fs::Path("/programs");
    auto objects = etl::vector<FileSystemObject, maxNObjectsPerPacket>{
        {Type::directory, fs::Path("/programs/.")         },
        {Type::directory, fs::Path("/programs/..")        },
        {Type::file,      fs::Path("/programs/00001")     },
        {Type::file,      fs::Path("/programs/00001.lock")}
    };
    auto nObjects = static_cast<std::uint8_t>(objects.size());
    auto report = RepositoryContentSummaryReport(repositoryPath, nObjects, objects);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto addToResult = report.AddTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(addToResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength
              + totalSerialSize<decltype(repositoryPath), decltype(nObjects)>
              + nObjects * (totalSerialSize<FileSystemObject>)));
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
    for(auto i = 0U; i < objects.size(); ++i)
    {
        CHECK(dataField[47 + 36 * i] == static_cast<Byte>(objects[i].type));
        CHECK(std::equal(objects[i].name.begin(),
                         objects[i].name.end(),
                         dataField.begin() + 48 + 36 * i,
                         [](char c, Byte b) { return c == static_cast<char>(b); }));
    }

    dataField.clear();
    addToResult = report.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 23_b);  // Service type ID
    CHECK(dataField[2] == 13_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);   // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);   // Message type counter (low byte)
}


TEST_CASE("Dumped raw memory data report")
{
    using sts1cobcsw::DumpedRawMemoryDataReport;

    auto dataField = etl::vector<Byte, sts1cobcsw::tm::maxPacketDataLength>{};
    std::uint8_t nDataBlocks = 17;
    auto startAddress = sts1cobcsw::fram::Address(0x1234);
    auto dumpedData = etl::vector<Byte, sts1cobcsw::maxDumpedDataLength>{
        0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0x00_b, 0x11_b, 0x22_b, 0x33_b, 0x44_b, 0x55_b,
        0x66_b, 0x77_b, 0x88_b, 0x99_b, 0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b, 0xEE_b, 0xFF_b};
    auto report = DumpedRawMemoryDataReport(nDataBlocks, startAddress, dumpedData);
    auto tBeforeWrite = sts1cobcsw::CurrentRealTime();
    auto addToResult = report.AddTo(&dataField);
    auto tAfterWrite = sts1cobcsw::CurrentRealTime();
    CHECK(addToResult.has_error() == false);
    CHECK(dataField.size() == report.Size());
    CHECK(report.Size()
          == (sts1cobcsw::tm::packetSecondaryHeaderLength
              + totalSerialSize<decltype(nDataBlocks), decltype(startAddress), std::uint8_t>
              + dumpedData.size()));
    // Packet secondary header
    CHECK(dataField[0] == 0x20_b);  // PUS version number, spacecraft time reference status
    CHECK(dataField[1] == 6_b);     // Service type ID
    CHECK(dataField[2] == 6_b);     // Submessage type ID
    CHECK(dataField[3] == 0_b);     // Message type counter (high byte)
    CHECK(dataField[4] == 0_b);     // Message type counter (low byte)
    CHECK(dataField[5] == 0xAA_b);  // Destination ID (high byte)
    CHECK(dataField[6] == 0x33_b);  // Destination ID (low byte)
    auto timestamp = Deserialize<RealTime, 7>(dataField);
    CHECK(tBeforeWrite <= timestamp);
    CHECK(timestamp <= tAfterWrite);
    // Number of data blocks
    CHECK(dataField[11] == static_cast<Byte>(nDataBlocks));
    // Start address
    CHECK(startAddress == (Deserialize<decltype(startAddress), 12>(dataField)));
    // Dumped data length
    CHECK(dataField[16] == static_cast<Byte>(dumpedData.size()));
    // Dumped data
    CHECK(std::equal(dumpedData.begin(), dumpedData.end(), dataField.begin() + 17));

    dataField.clear();
    addToResult = report.AddTo(&dataField);
    CHECK(addToResult.has_error() == false);
    // Packet secondary header
    CHECK(dataField[1] == 6_b);  // Service type ID
    CHECK(dataField[2] == 6_b);  // Submessage type ID
    CHECK(dataField[3] == 0_b);  // Message type counter (high byte)
    CHECK(dataField[4] == 1_b);  // Message type counter (low byte)
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

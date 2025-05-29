#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Reports.hpp>
#include <Sts1CobcSw/RfProtocols/SpacePacket.hpp>
#include <Sts1CobcSw/RfProtocols/TmTransferFrame.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>


namespace sts1cobcsw
{
using std::literals::operator""s;


constexpr auto stackSize = 4000U;


auto WriteToFileAsFrame(Payload const & report, std::string const & filename) -> void;


class ReportFramesTest : public RODOS::StaticThread<stackSize>
{
public:
    ReportFramesTest() : StaticThread("ReportFramesTest")
    {
    }


private:
    auto init() -> void override
    {
    }


    auto run() -> void override
    {
        RODOS::PRINTF("Generating TM Transfer Frames for all reports\n\n");

        static auto const outputDir = "/workspaces/STS1_COBC_SW/build/reports/"s;
        std::filesystem::create_directories(outputDir);

        // Successful acceptance verification reports
        {
            auto reportName = "SuccessfulAcceptanceVerificationReport.bin"s;
            auto requestId = sts1cobcsw::RequestId{
                .packetVersionNumber = sts1cobcsw::packetVersionNumber,
                .packetType = sts1cobcsw::packettype::telecommand,
                .secondaryHeaderFlag = 1,
                .apid = sts1cobcsw::normalApid,
                .sequenceFlags = 0b11,
                .packetSequenceCount = 0,
            };
            auto report = SuccessfulVerificationReport<VerificationStage::acceptance>(requestId);
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        // Successful completion of execution verification reports
        {
            auto reportName = "SuccessfulCompletionOfExecutionVerificationReport.bin"s;
            auto requestId = sts1cobcsw::RequestId{
                .packetVersionNumber = sts1cobcsw::packetVersionNumber,
                .packetType = sts1cobcsw::packettype::telecommand,
                .secondaryHeaderFlag = 1,
                .apid = sts1cobcsw::normalApid,
                .sequenceFlags = 0b11,
                .packetSequenceCount = 0,
            };
            auto report =
                SuccessfulVerificationReport<VerificationStage::completionOfExecution>(requestId);
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        // Failed acceptance verification reports
        {
            auto reportName = "FailedAcceptanceVerificationReport.bin"s;
            auto requestId = sts1cobcsw::RequestId{
                .packetVersionNumber = sts1cobcsw::packetVersionNumber,
                .packetType = sts1cobcsw::packettype::telecommand,
                .secondaryHeaderFlag = 1,
                .apid = sts1cobcsw::normalApid,
                .sequenceFlags = 0b11,
                .packetSequenceCount = 3,
            };
            auto report = FailedVerificationReport<VerificationStage::acceptance>(
                requestId, ErrorCode::invalidSpacePacket);
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        // Failed completion of execution verification reports
        {
            auto reportName = "FailedCompletionOfExecutionVerificationReport.bin"s;
            auto requestId = sts1cobcsw::RequestId{
                .packetVersionNumber = sts1cobcsw::packetVersionNumber,
                .packetType = sts1cobcsw::packettype::telecommand,
                .secondaryHeaderFlag = 1,
                .apid = sts1cobcsw::normalApid,
                .sequenceFlags = 0b11,
                .packetSequenceCount = 4,
            };
            auto report = FailedVerificationReport<VerificationStage::completionOfExecution>(
                requestId, ErrorCode::timeout);
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        // Wait a second to ensure that the time fields are not all the same
        SuspendFor(1 * s);

        // Housekeeping parameter report
        {
            auto reportName = "HousekeepingParameterReport.bin"s;
            auto record = sts1cobcsw::TelemetryRecord{
                .eduShouldBePowered = 1,
                .eduIsAlive = 1,
                .newEduResultIsAvailable = 1,
                .antennasShouldBeDeployed = 1,
                .framIsWorking = 1,
                .epsIsWorking = 1,
                .flashIsWorking = 1,
                .rfIsWorking = 1,
                .lastTelecommandIdWasInvalid = 1,
                .lastTelecommandArgumentsWereInvalid = 1,
                .nTotalResets = 1U,
                .nResetsSinceRf = 2U,
                .activeSecondaryFwPartition = 3,
                .backupSecondaryFwPartition = 4,
                .eduProgramQueueIndex = 5U,
                .programIdOfCurrentEduProgramQueueEntry = sts1cobcsw::ProgramId(6),
                .nEduCommunicationErrors = 7U,
                .lastResetReason = 8U,
                .rodosTimeInSeconds = 9,
                .realTime = sts1cobcsw::RealTime(10),
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
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        // Parameter value report
        {
            auto reportName = "ParameterValueReport.bin"s;
            auto parameterId = ParameterId::rxBaudRate;
            auto parameterValue = 9600U;
            auto report = ParameterValueReport(parameterId, parameterValue);
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        // File attribute report
        {
            auto reportName = "FileAttributeReport.bin"s;
            auto filePath = fs::Path("/results/12345_67890.zip");
            auto fileSize = 0xDEADBEEFU;
            auto fileStatus = FileStatus::locked;
            auto report = FileAttributeReport(filePath, fileSize, fileStatus);
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        // Repository content summary report
        {
            auto reportName = "RepositoryContentSummaryReport.bin"s;
            auto repositoryPath = fs::Path("/programs");
            static constexpr auto maxNObjectsPerPacket =
                RepositoryContentSummaryReport::maxNObjectsPerPacket;
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
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        // Dumped raw memory data report
        {
            auto reportName = "DumpedRawMemoryDataReport.bin"s;
            std::uint8_t nDataBlocks = 17;
            auto startAddress = sts1cobcsw::fram::Address(0x1234);
            auto dumpedData = etl::vector<Byte, sts1cobcsw::maxDumpedDataLength>{
                0xDE_b, 0xAD_b, 0xBE_b, 0xEF_b, 0x00_b, 0x11_b, 0x22_b, 0x33_b, 0x44_b, 0x55_b,
                0x66_b, 0x77_b, 0x88_b, 0x99_b, 0xAA_b, 0xBB_b, 0xCC_b, 0xDD_b, 0xEE_b, 0xFF_b};
            auto report = DumpedRawMemoryDataReport(nDataBlocks, startAddress, dumpedData);
            WriteToFileAsFrame(report, outputDir + reportName);
        }

        RODOS::PRINTF("\nDone, ");
        RODOS::hwResetAndReboot();
    }
} reportFramesTest;


auto WriteToFileAsFrame(Payload const & report, std::string const & filename) -> void
{
    auto buffer = std::array<Byte, tm::transferFrameLength>{};
    auto frame = tm::TransferFrame(Span(&buffer));
    frame.StartNew(pusVcid);
    auto addSpacePacketResult =
        sts1cobcsw::AddSpacePacketTo(&frame.GetDataField(), sts1cobcsw::normalApid, report);
    if(addSpacePacketResult.has_error())
    {
        RODOS::PRINTF("Failed to add Space Packet for %s\n", filename.c_str());
        return;
    }
    frame.Finish();

    // Write underlying buffer of frame to file
    auto file = std::ofstream(filename, std::ios::binary | std::ios::trunc);
    if(!file)
    {
        RODOS::PRINTF("Failed to open/create %s\n", filename.c_str());
        return;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    file.write(reinterpret_cast<char const *>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));
    if(file.good())
    {
        RODOS::PRINTF("Successfully generated %s\n", filename.c_str());
    }
    else
    {
        RODOS::PRINTF("Failed to generate     %s\n", filename.c_str());
    }
}
}

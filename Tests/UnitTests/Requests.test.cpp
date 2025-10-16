#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Requests.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>

#include <strong_type/type.hpp>

#include <etl/string.h>
#include <etl/vector.h>

#include <cstdint>
#include <span>
#include <utility>


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


TEST_CASE("LoadRawMemoryDataAreasRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};

    buffer.resize(6);
    buffer[0] = 0x01_b;  // Number of data areas
    // A start address of 0x0400 = 1024 should lie within the test memory section
    buffer[1] = 0x00_b;        // Start address (high byte)
    buffer[2] = 0x00_b;        // Start address
    buffer[3] = 0x04_b;        // Start address
    buffer[4] = 0x00_b;        // Start address (low byte)
    buffer[5] = 0x02_b;        // Data Length
    buffer.push_back(0xAA_b);  // Data
    buffer.push_back(0xBB_b);  // Data

    auto parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.nDataAreas == 0x1);
    CHECK(request.startAddress.value_of() == 0x0000'0400U);
    CHECK(request.dataLength == 0x02);
    CHECK(request.data[0] == 0xAA_b);
    CHECK(request.data[1] == 0xBB_b);

    // Minimum buffer size needs to be LoadRawMemoryDataAreasHeader
    auto smallerBuffer = etl::vector<Byte, 6 - 1>{};
    smallerBuffer.resize(6 - 1);
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(smallerBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);

    buffer[0] = 0x2_b;  // Only one data area allowed
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[0] = 0x01_b;

    buffer[5] = 0xFF_b;  // Data length > 189 is not allowed
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataArea);
    buffer[5] = 0x02_b;

    buffer[3] = 0x00_b;  // Start address 0x0000'0000 is not within the test memory section
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataArea);
    buffer[3] = 0x04_b;

    // Buffer size must be LoadRawMemoryDataAreasHeader + DataLength
    buffer.resize(buffer.size() + 1);
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
}


TEST_CASE("DumpRawMemoryDataRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(11);
    buffer[0] = 0x02_b;   // Number of data areas
    buffer[1] = 0xAA_b;   // Start address 1 (high byte)
    buffer[2] = 0xBB_b;   // Start address 1
    buffer[3] = 0xCC_b;   // Start address 1
    buffer[4] = 0xDD_b;   // Start address 1 (low byte)
    buffer[5] = 0x02_b;   // Data Length 1
    buffer[6] = 0xBB_b;   // Start address 2 (high byte)
    buffer[7] = 0xCC_b;   // Start address 2
    buffer[8] = 0xDD_b;   // Start address 2
    buffer[9] = 0xEE_b;   // Start address 2 (low byte)
    buffer[10] = 0x01_b;  // Data Length 2

    auto parseResult = sts1cobcsw::ParseAsDumpRawMemoryDataRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.nDataAreas == 0x2);
    CHECK(request.dataAreas[0].startAddress.value_of() == 0xAABB'CCDDULL);
    CHECK(request.dataAreas[0].length == 0x02);
    CHECK(request.dataAreas[1].startAddress.value_of() == 0xBBCC'DDEEULL);
    CHECK(request.dataAreas[1].length == 0x01);

    buffer[0] = 40_b;  // No more than 39 data areas are allowed
    parseResult = sts1cobcsw::ParseAsDumpRawMemoryDataRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[0] = 0x02_b;

    buffer[5] = 195_b;  // maxDumpedDataLength is 194
    parseResult = sts1cobcsw::ParseAsDumpRawMemoryDataRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataArea);
    buffer[5] = 0x02_b;

    // Minimum buffer size needs to be DumpRawMemoryDataHeader + DataArea * Number Of Areas
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(1);
    smallBuffer[0] = 0x02_b;
    parseResult = sts1cobcsw::ParseAsDumpRawMemoryDataRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);

    // Minimum buffer size needs to be DumpRawMemoryDataHeader
    auto smallerBuffer = etl::vector<Byte, 1>{};
    smallerBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsDumpRawMemoryDataRequest(smallerBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("PerformAFunctionRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(1);
    buffer[0] = 0x01_b;        // FunctionID
    buffer.push_back(0xAA_b);  // Data

    auto parseResult = sts1cobcsw::ParseAsPerformAFunctionRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.functionId == sts1cobcsw::FunctionId::stopAntennaDeployment);
    CHECK(request.dataField[0] == 0xAA_b);

    // Minimum buffer size needs to be 1
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsPerformAFunctionRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("ReportParameterValuesRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(5);
    buffer[0] = 0x04_b;  // Number of ParameterIDs
    buffer[1] = 0x01_b;  // ParameterID 1
    buffer[2] = 0x02_b;  // ParameterID 2
    buffer[3] = 0x03_b;  // ParameterID 3
    buffer[4] = 0x04_b;  // ParameterID 4

    auto parseResult = sts1cobcsw::ParseAsReportParameterValuesRequest(buffer);
    REQUIRE(parseResult.has_value());
    auto const & request = parseResult.value();

    CHECK(request.nParameters == 0x04);
    CHECK(request.parameterIds[0] == sts1cobcsw::Parameter::Id::rxDataRate);
    CHECK(request.parameterIds[1] == sts1cobcsw::Parameter::Id::txDataRate);
    CHECK(request.parameterIds[2] == sts1cobcsw::Parameter::Id::newEduResultIsAvailable);
    CHECK(request.parameterIds[3] == sts1cobcsw::Parameter::Id::maxEduIdleDuration);

    // No more than 5 Parameters are allowed
    buffer[0] = 0x06_b;
    parseResult = sts1cobcsw::ParseAsReportParameterValuesRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[0] = 0x04_b;

    // For 4 Parameter, a buffer of 5 is required
    auto smallerBuffer = etl::vector<Byte, 5>{};
    smallerBuffer.resize(4);
    smallerBuffer[0] = 0x04_b;  // Number of ParameterIDs
    smallerBuffer[1] = 0x01_b;  // ParameterID 1
    smallerBuffer[2] = 0x02_b;  // ParameterID 2
    smallerBuffer[3] = 0x03_b;  // ParameterID 3
    smallerBuffer[4] = 0x04_b;  // ParameterID 4
    parseResult = sts1cobcsw::ParseAsReportParameterValuesRequest(smallerBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);

    // Minimum buffer size needs to be 1
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsReportParameterValuesRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("SetParameterValuesRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(21);
    buffer[0] = 0x04_b;   // Number of ParameterIDs
    buffer[1] = 0x01_b;   // ParameterId 1
    buffer[2] = 0xAA_b;   // ParameterValue 1 (high byte)
    buffer[3] = 0xBB_b;   // ParameterValue 1
    buffer[4] = 0xCC_b;   // ParameterValue 1
    buffer[5] = 0xDD_b;   // ParameterValue 1 (low byte)
    buffer[6] = 0x02_b;   // ParameterId 2
    buffer[7] = 0xBB_b;   // ParameterValue 2 (high byte)
    buffer[8] = 0xCC_b;   // ParameterValue 2
    buffer[9] = 0xDD_b;   // ParameterValue 2
    buffer[10] = 0xEE_b;  // ParameterValue 2 (low byte)
    buffer[11] = 0x03_b;  // ParameterId 3
    buffer[16] = 0x04_b;  // ParameterId 4

    auto parseResult = sts1cobcsw::ParseAsSetParameterValuesRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.nParameters == 0x04);
    CHECK(request.parameters[0].id == sts1cobcsw::Parameter::Id::rxDataRate);
    CHECK(request.parameters[0].value == 0xAABB'CCDD);
    CHECK(request.parameters[1].id == sts1cobcsw::Parameter::Id::txDataRate);
    CHECK(request.parameters[1].value == 0xBBCC'DDEE);

    // Minimum buffer size needs to be 1
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsSetParameterValuesRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);

    // No more than 5 Parameters are allowed
    buffer[0] = 0x06_b;
    parseResult = sts1cobcsw::ParseAsSetParameterValuesRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[0] = 0x04_b;

    // For 4 Parameter, a buffer of 25 is required
    auto smallerBuffer = etl::vector<Byte, 24>{};
    smallerBuffer.resize(24);
    smallerBuffer[0] = 0x04_b;  // Number of ParameterIDs
    parseResult = sts1cobcsw::ParseAsSetParameterValuesRequest(smallerBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);

    // All parameter IDs must be valid
    buffer[1] = 0xFF_b;  // Invalid ParameterId
    parseResult = sts1cobcsw::ParseAsSetParameterValuesRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidParameterId);
}


TEST_CASE("DeleteAFileRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(35);
    buffer[0] = 0x2f_b;
    buffer[1] = 0x70_b;
    buffer[2] = 0x72_b;
    buffer[3] = 0x6f_b;
    buffer[4] = 0x67_b;
    buffer[5] = 0x72_b;
    buffer[6] = 0x61_b;
    buffer[7] = 0x6d_b;
    buffer[8] = 0x31_b;
    buffer[9] = 0x00_b;  // Null

    auto parseResult = sts1cobcsw::ParseAsDeleteAFileRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.filePath == "/program1");

    // A empty string is not allowed
    buffer[0] = 0x00_b;
    parseResult = sts1cobcsw::ParseAsDeleteAFileRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::emptyFilePath);
    buffer[0] = 0x2f_b;

    // Minimum buffer size needs to be 35
    auto smallBuffer = etl::vector<Byte, 34>{};
    smallBuffer.resize(34);
    parseResult = sts1cobcsw::ParseAsDeleteAFileRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("ReportTheAttributesOfAFileRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(35);
    buffer[0] = 0x2f_b;
    buffer[1] = 0x70_b;
    buffer[2] = 0x72_b;
    buffer[3] = 0x6f_b;
    buffer[4] = 0x67_b;
    buffer[5] = 0x72_b;
    buffer[6] = 0x61_b;
    buffer[7] = 0x6d_b;
    buffer[8] = 0x31_b;
    buffer[9] = 0x00_b;  // Null

    auto parseResult = sts1cobcsw::ParseAsReportTheAttributesOfAFileRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.filePath == "/program1");

    // A empty string is not allowed
    buffer[0] = 0x00_b;
    parseResult = sts1cobcsw::ParseAsReportTheAttributesOfAFileRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::emptyFilePath);
    buffer[0] = 0x2f_b;

    // Minimum buffer size needs to be 35
    auto smallBuffer = etl::vector<Byte, 34>{};
    smallBuffer.resize(34);
    parseResult = sts1cobcsw::ParseAsReportTheAttributesOfAFileRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("SummaryReportTheContentOfARepositoryRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(35);
    buffer[0] = 0x2f_b;
    buffer[1] = 0x70_b;
    buffer[2] = 0x72_b;
    buffer[3] = 0x6f_b;
    buffer[4] = 0x67_b;
    buffer[5] = 0x72_b;
    buffer[6] = 0x61_b;
    buffer[7] = 0x6d_b;
    buffer[8] = 0x31_b;
    buffer[9] = 0x00_b;  // Null

    auto parseResult = sts1cobcsw::ParseAsSummaryReportTheContentOfARepositoryRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.repositoryPath == "/program1");

    // A empty string is not allowed
    buffer[0] = 0x00_b;
    parseResult = sts1cobcsw::ParseAsSummaryReportTheContentOfARepositoryRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::emptyFilePath);
    buffer[0] = 0x2f_b;

    // Minimum buffer size needs to be 35
    auto smallBuffer = etl::vector<Byte, 34>{};
    smallBuffer.resize(34);
    parseResult = sts1cobcsw::ParseAsSummaryReportTheContentOfARepositoryRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("CopyAFileRequest")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(75);
    buffer[0] = 0x0F_b;  // OperationId
    buffer[1] = 0x2f_b;  // Source File Path
    buffer[2] = 0x70_b;
    buffer[3] = 0x72_b;
    buffer[4] = 0x6f_b;
    buffer[5] = 0x67_b;
    buffer[6] = 0x72_b;
    buffer[7] = 0x61_b;
    buffer[8] = 0x6d_b;
    buffer[9] = 0x31_b;
    buffer[10] = 0x00_b;  // Null
    buffer[36] = 0x2f_b;  // Target File Path
    buffer[37] = 0x70_b;
    buffer[38] = 0x72_b;
    buffer[39] = 0x6f_b;
    buffer[40] = 0x67_b;
    buffer[41] = 0x72_b;
    buffer[42] = 0x61_b;
    buffer[43] = 0x6d_b;
    buffer[44] = 0x32_b;
    buffer[45] = 0x00_b;  // Null
    buffer[71] = 0x12_b;  // file size (high byte)
    buffer[72] = 0x34_b;
    buffer[73] = 0x56_b;
    buffer[74] = 0x78_b;  // file size (low byte)

    auto parseResult = sts1cobcsw::ParseAsCopyAFileRequest(buffer);
    REQUIRE(parseResult.has_value());
    auto request = parseResult.value();
    CHECK(request.operationId == sts1cobcsw::copyOperationId);
    CHECK(request.sourceFilePath == "/program1");
    CHECK(request.targetFilePath == "/program2");
    CHECK(request.fileSize == 0x1234'5678U);

    // OperationId needs to be 0x0F
    buffer[0] = 0xF0_b;
    parseResult = sts1cobcsw::ParseAsCopyAFileRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[0] = 0x0F_b;

    // A empty string is not allowed as source
    buffer[1] = 0x00_b;
    parseResult = sts1cobcsw::ParseAsCopyAFileRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::emptyFilePath);
    buffer[1] = 0x2f_b;

    // A empty string is not allowed as target
    buffer[36] = 0x00_b;
    parseResult = sts1cobcsw::ParseAsCopyAFileRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::emptyFilePath);
    buffer[36] = 0x2f_b;

    // Minimum buffer size needs to be 71
    auto smallBuffer = etl::vector<Byte, 70>{};
    smallBuffer.resize(70);
    parseResult = sts1cobcsw::ParseAsCopyAFileRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
}


TEST_CASE("ReportHousekeepingParameterReportFunction")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(4);
    buffer[0] = 0x00_b;  // First Report Index (high byte)
    buffer[1] = 0x01_b;  // First Report Index (low byte)
    buffer[2] = 0x00_b;  // Last Report Index (high byte)
    buffer[3] = 0x02_b;  // Last Report Index (low byte)

    auto parseResult = sts1cobcsw::ParseAsReportHousekeepingParameterReportFunction(buffer);
    CHECK(parseResult.has_value());
    auto function = parseResult.value();

    CHECK(function.firstReportIndex == 0x01);
    CHECK(function.lastReportIndex == 0x02);

    // Last index needs to be equal or greater than first index
    buffer[1] = 0x03_b;
    parseResult = sts1cobcsw::ParseAsReportHousekeepingParameterReportFunction(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[1] = 0x01_b;

    // Minimum buffer size needs to be 4
    auto smallBuffer = etl::vector<Byte, 4>{};
    smallBuffer.resize(3);
    parseResult = sts1cobcsw::ParseAsReportHousekeepingParameterReportFunction(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
}


TEST_CASE("ParseAsEnableFileTransferFunction")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(2);
    buffer[0] = 0xAA_b;  // Duration (high byte)
    buffer[1] = 0xBB_b;  // Duration (low byte)

    auto parseResult = sts1cobcsw::ParseAsEnableFileTransferFunction(buffer);
    CHECK(parseResult.has_value());
    auto function = parseResult.value();

    CHECK(function.durationInS == 0xAABB);

    // Minimum buffer size needs to be 2
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(1);
    parseResult = sts1cobcsw::ParseAsEnableFileTransferFunction(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
}


TEST_CASE("SynchronizeTimeFunction")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(4);
    buffer[0] = 0x0A_b;  // Real time (high byte)
    buffer[1] = 0x0B_b;  // Real time
    buffer[2] = 0x0C_b;  // Real time
    buffer[3] = 0x0D_b;  // Real time (low byte)

    auto parseResult = sts1cobcsw::ParseAsSynchronizeTimeFunction(buffer);
    CHECK(parseResult.has_value());
    auto function = parseResult.value();

    CHECK(value_of(function.realTime) == 0x0A0B'0C0D);

    // Buffer size must be 4
    buffer.resize(5);
    parseResult = sts1cobcsw::ParseAsSynchronizeTimeFunction(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
    buffer.resize(3);
    parseResult = sts1cobcsw::ParseAsSynchronizeTimeFunction(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
}


TEST_CASE("UpdateEduQueueFunction")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(9);
    buffer[0] = 0x01_b;  // nQueueEntries
    buffer[1] = 0xAA_b;  // ProgramId (high byte)
    buffer[2] = 0xBB_b;  // ProgramId (low byte)
    buffer[3] = 0xAA_b;  // Start Time (high byte)
    buffer[4] = 0xBB_b;  // Start Time
    buffer[5] = 0xCC_b;  // Start Time
    buffer[6] = 0xDD_b;  // Start Time (low byte)
    buffer[7] = 0xAB_b;  // Timeout (high byte)
    buffer[8] = 0xCD_b;  // Timeout (low byte)

    auto parseResult = sts1cobcsw::ParseAsUpdateEduQueueFunction(buffer);
    CHECK(parseResult.has_value());
    auto function = parseResult.value();

    CHECK(function.nQueueEntries == 0x01);
    CHECK(value_of(function.queueEntries[0].programId) == 0xAABB);
    CHECK(value_of(function.queueEntries[0].startTime) == static_cast<std::int32_t>(0xAABB'CCDD));
    CHECK(function.queueEntries[0].timeout == static_cast<std::int16_t>(0xABCD));

    // Only 24 entries allowed
    buffer[0] = 25_b;
    parseResult = sts1cobcsw::ParseAsUpdateEduQueueFunction(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[0] = 0x01_b;

    // One entry needs 8 bytes + 1 for nQueueEntries
    auto smallerBuffer = etl::vector<Byte, 8>{};
    smallerBuffer.resize(8);
    parseResult = sts1cobcsw::ParseAsUpdateEduQueueFunction(smallerBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);

    // Minimum buffer size needs to be 1
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsUpdateEduQueueFunction(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}


TEST_CASE("SetActiveFirmwareFunction")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(1);
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary1);

    auto parseResult = sts1cobcsw::ParseAsSetActiveFirmwareFunction(buffer);
    CHECK(parseResult.has_value());
    auto function = parseResult.value();

    CHECK(function.partitionId == sts1cobcsw::PartitionId::secondary1);

    // Check with secondaryFwPartition2
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary2);
    parseResult = sts1cobcsw::ParseAsSetActiveFirmwareFunction(buffer);
    CHECK(parseResult.has_value());
    function = parseResult.value();
    CHECK(function.partitionId == sts1cobcsw::PartitionId::secondary2);

    // Only secondary partitions are allowed
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::primary);
    parseResult = sts1cobcsw::ParseAsSetActiveFirmwareFunction(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidPartitionId);
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary1);

    // Minimum buffer size needs to be 1
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsSetActiveFirmwareFunction(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
}


TEST_CASE("SetBackupFirmwareFunction")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(1);
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary1);

    auto parseResult = sts1cobcsw::ParseAsSetBackupFirmwareFunction(buffer);
    CHECK(parseResult.has_value());
    auto function = parseResult.value();

    CHECK(function.partitionId == sts1cobcsw::PartitionId::secondary1);

    // Check with secondaryFwPartition2
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary2);
    parseResult = sts1cobcsw::ParseAsSetBackupFirmwareFunction(buffer);
    CHECK(parseResult.has_value());
    function = parseResult.value();
    CHECK(function.partitionId == sts1cobcsw::PartitionId::secondary2);

    // Only secondary partitions are allowed
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::primary);
    parseResult = sts1cobcsw::ParseAsSetBackupFirmwareFunction(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidPartitionId);
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary1);

    // Minimum buffer size needs to be 1
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsSetBackupFirmwareFunction(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
}


TEST_CASE("CheckFirmwareIntegrityFunction")
{
    auto buffer = etl::vector<Byte, sts1cobcsw::tc::maxPacketLength>{};
    buffer.resize(1);
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary1);

    auto parseResult = sts1cobcsw::ParseAsCheckFirmwareIntegrityFunction(buffer);
    CHECK(parseResult.has_value());
    auto function = parseResult.value();

    CHECK(function.partitionId == sts1cobcsw::PartitionId::secondary1);

    // Check with secondaryFwPartition2
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary2);
    parseResult = sts1cobcsw::ParseAsCheckFirmwareIntegrityFunction(buffer);
    CHECK(parseResult.has_value());
    function = parseResult.value();
    CHECK(function.partitionId == sts1cobcsw::PartitionId::secondary2);

    // Check with primaryFwPartition
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::primary);
    parseResult = sts1cobcsw::ParseAsCheckFirmwareIntegrityFunction(buffer);
    CHECK(parseResult.has_value());
    function = parseResult.value();
    CHECK(function.partitionId == sts1cobcsw::PartitionId::primary);

    // Only valid partitionIds are allowed
    buffer[0] = 0xAB_b;
    parseResult = sts1cobcsw::ParseAsCheckFirmwareIntegrityFunction(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidPartitionId);
    buffer[0] = static_cast<Byte>(sts1cobcsw::PartitionId::secondary1);

    // Minimum buffer size needs to be 1
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsCheckFirmwareIntegrityFunction(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);
}

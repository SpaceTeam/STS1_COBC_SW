#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Requests.hpp>
#include <Sts1CobcSw/RfProtocols/TcSpacePacketSecondaryHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <etl/vector.h>

#include <span>


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
    buffer[0] = 0x01_b;        // Number of data areas
    buffer[1] = 0xAA_b;        // Start address (high byte)
    buffer[2] = 0xBB_b;        // Start address
    buffer[3] = 0xCC_b;        // Start address
    buffer[4] = 0xDD_b;        // Start address (low byte)
    buffer[5] = 0x02_b;        // Data Length
    buffer.push_back(0xAA_b);  // Data
    buffer.push_back(0xBB_b);  // Data

    auto parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(buffer);
    CHECK(parseResult.has_value());
    auto request = parseResult.value();

    CHECK(request.nDataAreas == 0x1);
    CHECK(request.startAddress.value_of() == 0xAABB'CCDDULL);
    CHECK(request.dataLength == 0x02);
    CHECK(request.data[0] == 0xAA_b);
    CHECK(request.data[1] == 0xBB_b);

    buffer[0] = 0x2_b;  // Only one data area allowed
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[0] = 0x01_b;

    buffer[5] = 0xFF_b;  // Data length > 189 is not allowed
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[5] = 0x02_b;

    // Minimum buffer size needs to be LoadRawMemoryDataAreasHeader + DataLength
    auto smallBuffer = etl::vector<Byte, 6>{};
    smallBuffer.resize(6);
    smallBuffer[0] = 0x01_b;  // Number of data areas
    smallBuffer[1] = 0xAA_b;  // Start address (high byte)
    smallBuffer[2] = 0xBB_b;  // Start address
    smallBuffer[3] = 0xCC_b;  // Start address
    smallBuffer[4] = 0xDD_b;  // Start address (low byte)
    smallBuffer[5] = 0x02_b;  // Data Length
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidDataLength);

    // Minimum buffer size needs to be LoadRawMemoryDataAreasHeader
    auto smallerBuffer = etl::vector<Byte, 6 - 1>{};
    smallerBuffer.resize(6 - 1);
    parseResult = sts1cobcsw::ParseAsLoadRawMemoryDataAreasRequest(smallerBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
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

    buffer[0] = 39_b;  // No more than 38 data areas are allowed
    parseResult = sts1cobcsw::ParseAsDumpRawMemoryDataRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
    buffer[0] = 0x02_b;

    buffer[5] = 195_b;  // maxDumpedDataLength is 194
    parseResult = sts1cobcsw::ParseAsDumpRawMemoryDataRequest(buffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::invalidApplicationData);
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

    CHECK(request.functionId == sts1cobcsw::tc::FunctionId::stopAntennaDeployment);
    CHECK(request.dataField[0] == 0xAA_b);

    // Minimum buffer size needs to be 1
    auto smallBuffer = etl::vector<Byte, 1>{};
    smallBuffer.resize(0);
    parseResult = sts1cobcsw::ParseAsPerformAFunctionRequest(smallBuffer);
    CHECK(parseResult.has_error());
    CHECK(parseResult.error() == ErrorCode::bufferTooSmall);
}

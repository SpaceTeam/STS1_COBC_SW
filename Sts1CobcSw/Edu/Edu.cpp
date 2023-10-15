#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Names.hpp>
#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>

#include <type_safe/types.hpp>

#include <algorithm>
#include <array>
#include <cstddef>


namespace sts1cobcsw
{
edu::Edu eduUnit;
}


namespace sts1cobcsw::edu
{
using sts1cobcsw::serial::operator""_b;
using sts1cobcsw::serial::Byte;

namespace ts = type_safe;
using ts::operator""_u16;
using ts::operator""_usize;


// TODO: Turn this into Bytes, maybe even an enum class : Byte
// CEP basic commands (see EDU PDD)
constexpr auto cmdAck = 0xd7_b;   //! Acknowledging a data packet
constexpr auto cmdNack = 0x27_b;  //! Not Acknowledging a (invalid) data packet
constexpr auto cmdEof = 0x59_b;   //! Transmission of multiple packets is complete
// TODO: Use this
//! Transmission of multiple packets should be stopped
[[maybe_unused]] constexpr auto cmdStop = 0xb4_b;
constexpr auto cmdData = 0x8b_b;  //! Data packet format is used (not a command packet!)

// GetStatus result types
constexpr auto noEventCode = 0x00_b;
constexpr auto programFinishedCode = 0x01_b;
constexpr auto resultsReadyCode = 0x02_b;

// Status types byte counts
constexpr auto nNoEventBytes = 1;
constexpr auto nProgramFinishedBytes = 6;
constexpr auto nResultsReadyBytes = 5;

// TODO: Check real timeouts
// Max. time for the EDU to respond to a request
constexpr auto eduTimeout = 1 * RODOS::SECONDS;
// Timeout used when flushing the UART receive buffer
constexpr auto flushTimeout = 1 * RODOS::MILLISECONDS;
// UART flush garbage buffer size
constexpr auto garbageBufferSize = 128;

// TODO: choose proper values
// Max. amount of send retries after receiving NACK
constexpr auto maxNNackRetries = 10;
// Max. number of data packets for a single command
constexpr auto maxNPackets = 100_usize;
// Max. length of a single data packet
constexpr auto maxDataLength = 32768;
// Data buffer for potentially large data sizes (ReturnResult and StoreArchive)
auto cepDataBuffer = std::array<Byte, maxDataLength>{};


auto Print(std::span<Byte> data, int nRows = 30) -> void;  // NOLINT


//! @brief  Must be called in an init() function of a thread.
auto Edu::Initialize() -> void
{
    eduEnableGpioPin_.Direction(hal::PinDirection::out);
    // TODO: I think we should actually read from persistent state to determine whether the EDU
    // should be powered or not. We do have a separate EDU power management thread which though.
    TurnOff();

    // TODO: Test how high we can set the baudrate without problems (bit errors, etc.)
    constexpr auto baudRate = 921'600;
    uart_.init(baudRate);
}


auto Edu::TurnOn() -> void
{
    // Set EduShouldBePowered to True, persistentstate is initialized in
    // EduPowerManagementThread.cpp
    periphery::persistentstate::EduShouldBePowered(true);
    eduEnableGpioPin_.Set();
}


auto Edu::TurnOff() -> void
{
    // Set EduShouldBePowered to False, persistentstate is initialized in
    // EduPowerManagementThread.cpp
    periphery::persistentstate::EduShouldBePowered(false);
    eduEnableGpioPin_.Reset();
}


// TODO: Implement this
auto Edu::StoreArchive([[maybe_unused]] StoreArchiveData const & data) -> Result<std::int32_t>
{
    return 0;
}


//! @brief Issues a command to execute a student program on the EDU.
//!
//! Execute Program (COBC <-> EDU):
//! -> [DATA]
//! -> [Command Header]
//! -> [Program ID]
//! -> [Queue ID]
//! -> [Timeout]
//! <- [N/ACK]
//! <- [N/ACK]
//!
//! The first N/ACK confirms a valid data packet,
//! the second N/ACK confirms that the program has been started.
//!
//! @param programId The student program ID
//! @param queueId The student program queue ID
//! @param timeout The available execution time for the student program
//!
//! @returns A relevant error code
auto Edu::ExecuteProgram(ExecuteProgramData const & data) -> Result<void>
{
    RODOS::PRINTF("ExecuteProgram(programId = %d, queueId = %d, timeout = %d)\n",
                  data.programId.get(),
                  data.queueId.get(),
                  data.timeout.get());
    // Check if data command was successful
    auto serialData = serial::Serialize(data);
    OUTCOME_TRY(SendData(serialData));

    // eduTimeout != timeout argument for data!
    // timeout specifies the time the student program has to execute
    // eduTimeout is the max. allowed time to reveice N/ACK from EDU
    auto answer = 0x00_b;
    uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

    auto nReadBytes = uart_.read(&answer, 1);
    if(nReadBytes == 0)
    {
        return ErrorCode::timeout;
    }
    switch(answer)
    {
        case cmdAck:
        {
            return ErrorCode::success;
        }
        case cmdNack:
        {
            return ErrorCode::nack;
        }
        default:
        {
            return ErrorCode::invalidResult;
        }
    }
}


//! @brief Issues a command to stop the currently running EDU program.
//! If there is no active program, the EDU will return ACK anyway.
//!
//! Stop Program:
//! -> [DATA]
//! -> [Command Header]
//! <- [N/ACK]
//! <- [N/ACK]
//! @returns A relevant error code
auto Edu::StopProgram() -> ErrorCode
{
    return ErrorCode::success;
    // std::array<std::uint8_t, 3> dataBuf = {stopProgram};
    // auto errorCode = SendData(dataBuf);

    // if(errorCode != EduErrorCode::success)
    // {
    //     return errorCode;
    // }

    // // Receive second N/ACK to see if program is successfully stopped
    // std::array<std::uint8_t, 1> recvBuf = {};
    // return UartReceive(recvBuf, 1);
}


//! @brief Issues a command to get the student program status.
//!
//! Possible statuses:
//! No event: [1 byte: 0x00]
//! Program finished: [1 byte: 0x01][2 bytes: Program ID][2 bytes: Queue ID][1 byte: Exit Code]
//! Results ready: [1 byte: 0x02][2 bytes: Program ID][2 bytes: Queue ID]
//!
//! Get Status (COBC <-> EDU):
//! -> [DATA]
//! -> [Command Header]
//! <- [N/ACK]
//! <- [DATA]
//! <- [Status]
//! -> [N/ACK]
//!
//! @returns A status containing (Status Type, [Program ID], [Queue ID], [Exit Code], Error
//!          Code). Values in square brackets are only valid if the relevant Status Type is
//!          returned.
auto Edu::GetStatus() -> Result<Status>
{
    RODOS::PRINTF("GetStatus()\n");
    auto serialData = serial::Serialize(getStatusId);
    auto sendDataResult = SendData(serialData);
    if(sendDataResult.has_error())
    {
        RODOS::PRINTF("  Returned .errorCode = %d\n", static_cast<int>(sendDataResult.error()));
        return sendDataResult.error();
    }

    Result<Status> status = ErrorCode::noErrorCodeSet;
    std::size_t errorCount = 0;
    do
    {
        status = GetStatusCommunication();
        if(status.has_value())
        {
            SendCommand(cmdAck);
            break;
        }
        FlushUartBuffer();
        SendCommand(cmdNack);
    } while(errorCount++ < maxNNackRetries);

    if(status.has_value())
    {
        RODOS::PRINTF(
            "  .statusType = %d\n  .programId = %d\n  .queueId = %d\n  exitCode = "
            "%d\n",
            status.value().statusType,
            status.value().programId,
            status.value().queueId,
            status.value().exitCode);
    }
    else
    {
        RODOS::PRINTF("  .errorCode = %d\n  = %d\n", status.error());
    }
    return status;
}


//! @brief Communication function for GetStatus() to separate a single try from
//! retry logic.
//! @returns The received EDU status
auto Edu::GetStatusCommunication() -> Result<Status>
{
    // Get header data
    auto headerBuffer = serial::SerialBuffer<HeaderData>{};
    auto headerReceiveResult = UartReceive(headerBuffer);
    auto headerData = serial::Deserialize<HeaderData>(headerBuffer);

    if(headerReceiveResult.has_error())
    {
        return headerReceiveResult.error();
    }

    if(headerData.command != cmdData)
    {
        return ErrorCode::invalidCommand;
    }

    if(headerData.length == 0_u16)
    {
        return ErrorCode::invalidLength;
    }

    // Get the status type code
    auto statusType = 0_b;
    auto statusTypeResult = UartReceive(&statusType);

    if(statusTypeResult)
    {
        return statusTypeResult.error();
    }

    if(statusType == noEventCode)
    {
        if(headerData.length != nNoEventBytes)
        {
            return ErrorCode::invalidLength;
        }

        std::array<Byte, 1> statusTypeArray = {statusType};
        auto crc32Error = CheckCrc32(std::span<Byte>(statusTypeArray));
        if(crc32Error.has_error())
        {
            return crc32Error.error();
        }

        return Status{
            .statusType = StatusType::noEvent, .programId = 0, .queueId = 0, .exitCode = 0};
    }

    if(statusType == programFinishedCode)
    {
        if(headerData.length != nProgramFinishedBytes)
        {
            return ErrorCode::invalidLength;
        }

        auto dataBuffer = serial::SerialBuffer<ProgramFinishedStatus>{};
        auto programFinishedResult = UartReceive(dataBuffer);

        if(programFinishedResult.has_error())
        {
            return programFinishedResult.error();
        }

        // Create another Buffer which includes the status type that was received beforehand because
        // it is needed to calculate the CRC32 checksum
        auto fullDataBuffer = std::array<Byte, dataBuffer.size() + 1>{};
        fullDataBuffer[0] = statusType;
        std::copy(dataBuffer.begin(), dataBuffer.end(), fullDataBuffer.begin() + 1);
        auto crc32Result = CheckCrc32(fullDataBuffer);
        if(crc32Result.has_error())
        {
            return crc32Result.error();
        }

        auto programFinishedData = serial::Deserialize<ProgramFinishedStatus>(dataBuffer);
        return Status{.statusType = StatusType::programFinished,
                      .programId = programFinishedData.programId,
                      .queueId = programFinishedData.queueId,
                      .exitCode = programFinishedData.exitCode};
    }

    if(statusType == resultsReadyCode)
    {
        if(headerData.length != nResultsReadyBytes)
        {
            return ErrorCode::invalidLength;
        }

        auto dataBuffer = serial::SerialBuffer<ResultsReadyStatus>{};
        auto resultsReadyError = UartReceive(dataBuffer);
        if(resultsReadyError.has_error())
        {
            return resultsReadyError.error();
        }

        // Create another Buffer which includes the status type that was received beforehand because
        // it is needed to calculate the CRC32 checksum
        auto fullDataBuffer = std::array<Byte, dataBuffer.size() + 1>{};
        fullDataBuffer[0] = statusType;
        std::copy(dataBuffer.begin(), dataBuffer.end(), fullDataBuffer.begin() + 1);
        auto crc32Result = CheckCrc32(fullDataBuffer);
        if(crc32Result)
        {
            return crc32Result.error();
        }
        auto resultsReadyData = serial::Deserialize<ResultsReadyStatus>(dataBuffer);
        return Status{.statusType = StatusType::resultsReady,
                      .programId = resultsReadyData.programId,
                      .queueId = resultsReadyData.queueId};
    }

    return ErrorCode::invalidStatusType;
}


auto Edu::ReturnResult() -> ResultInfo
{
    // DEBUG
    RODOS::PRINTF("ReturnResult()\n");
    // END DEBUG

    // Send command
    auto serialCommand = serial::Serialize(returnResultId);
    auto sendCommandResult = SendData(serialCommand);
    if(sendCommandResult.has_error())
    {
        return ResultInfo{.errorCode = sendCommandResult.error(), .resultSize = 0U};
    }

    // DEBUG
    // RODOS::PRINTF("\nStart receiving result\n");
    // END DEBUG

    ts::size_t totalResultSize = 0_usize;
    ts::size_t packets = 0_usize;
    Result<ts::size_t> result = ErrorCode::noErrorCodeSet;
    auto errorCode = ErrorCode::success;
    // TODO: Turn into for loop
    while(packets < maxNPackets)
    {
        // DEBUG
        // RODOS::PRINTF("\nPacket %d\n", static_cast<int>(packets.get()));
        // END DEBUG
        result = ReturnResultRetry();
        if(result.has_error())
        {
            errorCode = result.error();
            RODOS::PRINTF(" ResultResultRetry() resulted in an error : %d",
                          static_cast<int>(errorCode));
            break;
        }
        // RODOS::PRINTF("\nWriting to file...\n");
        // TODO: Actually write to a file

        RODOS::PRINTF(" ResultResultRetry() resulted in a success and returned  %d ",
                      static_cast<int>(result.value().get()));
        totalResultSize += result.value();
        packets++;
    }
    return ResultInfo{.errorCode = errorCode, .resultSize = totalResultSize};
}


//! @brief This function handles the retry logic for a single transmission round and is called by
//! the actual ReturnResult function. The communication happens in ReturnResultCommunication.
//!
//! @returns An error code and the number of received bytes in ResultInfo
auto Edu::ReturnResultRetry() -> Result<ts::size_t>
{
    Result<ts::size_t> result = ErrorCode::noErrorCodeSet;
    std::size_t errorCount = 0U;
    do
    {
        result = ReturnResultCommunication();
        if(result.has_value() or (result.has_error() and result.error() == ErrorCode::successEof))
        {
            SendCommand(cmdNack);
            // Return ts::size_t
            return result;
        }
        FlushUartBuffer();
        SendCommand(cmdNack);
    } while(errorCount++ < maxNNackRetries);
    // Return an error
    return result;
}


// This function writes the result to the COBC file system (flash). Maybe it doesn't do that
// directly and instead writes to a non-primary RAM bank as an intermediate step.
//
// Simple results -> 1 round should work with DMA to RAM
auto Edu::ReturnResultCommunication() -> Result<ts::size_t>
{
    // Receive command
    // If no result is available, the command will be NACK,
    // otherwise DATA
    Byte command = 0_b;
    auto commandResult = UartReceive(&command);
    if(commandResult.has_error())
    {
        return commandResult.error();
    }
    if(command == cmdNack)
    {
        // TODO: necessary to differentiate errors or just return success with resultSize 0?
        // return ResultInfo{.errorCode = ErrorCode::noResultAvailable, .resultSize = 0U};
        return ErrorCode::noResultAvailable;
    }
    if(command == cmdEof)
    {
        // return ResultInfo{.errorCode = ErrorCode::successEof, .resultSize = 0U};
        return ErrorCode::successEof;
    }
    if(command != cmdData)
    {
        // DEBUG
        RODOS::PRINTF("\nNot DATA command\n");
        // END DEBUG
        return ErrorCode::invalidCommand;
    }

    // DEBUG
    // RODOS::PRINTF("\nGet Length\n");
    // END DEBUG

    auto dataLengthBuffer = serial::SerialBuffer<ts::uint16_t>{};
    auto lengthResult = UartReceive(dataLengthBuffer);
    if(lengthResult.has_error())
    {
        return lengthResult.error();
    }

    auto actualDataLength = serial::Deserialize<ts::uint16_t>(dataLengthBuffer);
    if(actualDataLength == 0U or actualDataLength > maxDataLength)
    {
        return ErrorCode::invalidLength;
    }

    // DEBUG
    // RODOS::PRINTF("\nGet Data\n");
    // END DEBUG

    // Get the actual data
    auto dataResult = UartReceive(
        std::span<Byte>(cepDataBuffer.begin(), cepDataBuffer.begin() + actualDataLength.get()));

    if(dataResult.has_error())
    {
        return dataResult.error();
    }

    // DEBUG
    // RODOS::PRINTF("\nCheck CRC\n");
    // END DEBUG

    auto crc32Result = CheckCrc32(
        std::span<Byte>(cepDataBuffer.begin(), cepDataBuffer.begin() + actualDataLength.get()));

    if(crc32Result.has_error())
    {
        return crc32Result.error();
    }

    // DEBUG
    RODOS::PRINTF("\nSuccess\n");
    // END DEBUG

    return actualDataLength;
}


//! @brief Issues a command to update the EDU time.
//!
//! Update Time:
//! -> [DATA]
//! -> [Command Header]
//! -> [Timestamp]
//! <- [N/ACK]
//! <- [N/ACK]
//!
//! The first N/ACK confirms a valid data packet,
//! the second N/ACK confirms the time update.
//!
//! @param timestamp A unix timestamp
//!
//! @returns A relevant error code
auto Edu::UpdateTime(UpdateTimeData const & data) -> Result<void>
{
    RODOS::PRINTF("UpdateTime()\n");
    auto serialData = serial::Serialize(data);
    OUTCOME_TRY(SendData(serialData))
    // if(errorCode != ErrorCode::success)
    //{
    //     return errorCode;
    // }

    // On success, wait for second N/ACK
    // TODO: (Daniel) Change to UartReceive()
    // TODO: Refactor this common pattern into a function
    // TODO: Implement read functions that return a type and internally use Deserialize<T>()
    auto answer = 0x00_b;
    uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

    auto nReadBytes = uart_.read(&answer, 1);
    if(nReadBytes == 0)
    {
        return ErrorCode::timeout;
    }
    switch(answer)
    {
        case cmdAck:
        {
            return ErrorCode::success;
        }
        case cmdNack:
        {
            return ErrorCode::nack;
        }
        default:
        {
            return ErrorCode::invalidResult;
        }
    }
}


//! @brief Send a CEP command to the EDU.
//!
//! @param cmd The command
void Edu::SendCommand(Byte commandId)
{
    auto data = std::array{commandId};
    // TODO: ambiguity when using arrays directly with Write operations (Communication.hpp)
    hal::WriteTo(&uart_, std::span(data));
}


//! @brief Send a data packet over UART to the EDU.
//!
//! @param data The data to be sent
auto Edu::SendData(std::span<Byte> data) -> Result<void>
{
    std::size_t const nBytes = data.size();
    if(nBytes >= maxDataLength)
    {
        return ErrorCode::sendDataTooLong;
    }

    // Casting size_t to uint16_t is safe since nBytes is checked against maxDataLength
    std::array<std::uint16_t, 1> len{static_cast<std::uint16_t>(nBytes)};
    std::array<std::uint32_t, 1> crc{utility::Crc32(data)};

    int nackCount = 0;
    while(nackCount < maxNNackRetries)
    {
        SendCommand(cmdData);
        hal::WriteTo(&uart_, std::span<std::uint16_t>(len));
        hal::WriteTo(&uart_, data);
        hal::WriteTo(&uart_, std::span<std::uint32_t>(crc));

        // TODO: Refactor this common pattern into a function
        // Data is always answered by N/ACK
        auto answer = 0xAA_b;  // Why is this set to 0xAA?
        uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

        auto nReadBytes = uart_.read(&answer, 1);
        if(nReadBytes == 0)
        {
            return ErrorCode::timeout;
        }
        // RODOS::PRINTF("[Edu] answer in sendData is now : %c\n", static_cast<char>(answer));
        switch(answer)
        {
            case cmdAck:
            {
                return ErrorCode::success;
            }
            case cmdNack:
            {
                nackCount++;
                continue;
            }
            default:
            {
                return ErrorCode::invalidResult;
            }
        }
    }
    return ErrorCode::tooManyNacks;
}


//! @brief Receive nBytes bytes over the EDU UART in a single round.
//!
//! @param destination The destination container
//!
//! @returns A relevant EDU error code
// TODO: Use hal::ReadFrom()
auto Edu::UartReceive(std::span<Byte> destination) -> Result<void>
{
    if(size(destination) > maxDataLength)
    {
        return ErrorCode::receiveDataTooLong;
    }

    std::size_t totalReceivedBytes = 0U;
    const auto destinationSize = size(destination);
    while(totalReceivedBytes < destinationSize)
    {
        uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
        auto nReceivedBytes = uart_.read(data(destination) + totalReceivedBytes,
                                         destinationSize - totalReceivedBytes);
        if(nReceivedBytes == 0)
        {
            return ErrorCode::timeout;
        }
        totalReceivedBytes += nReceivedBytes;
    }
}


//! @brief Receive a single byte over the EDU UART.
//!
//! @param destination The destination byte
//!
//! @returns A relevant EDU error code
// TODO: Use hal::ReadFrom()
auto Edu::UartReceive(void * destination) -> Result<void>
{
    uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
    auto nReceivedBytes = uart_.read(destination, 1);
    if(nReceivedBytes == 0)
    {
        return ErrorCode::timeout;
    }
}


//! @brief Flush the EDU UART read buffer.
//!
//! This can be used to clear all buffer data after an error to request a resend.
auto Edu::FlushUartBuffer() -> void
{
    auto garbageBuffer = std::array<Byte, garbageBufferSize>{};
    ts::bool_t dataReceived = true;

    // Keep reading until no data is coming for flushTimeout
    while(dataReceived)
    {
        uart_.suspendUntilDataReady(RODOS::NOW() + flushTimeout);
        auto nReceivedBytes = uart_.read(garbageBuffer.data(), garbageBufferSize);
        if(nReceivedBytes == 0)
        {
            dataReceived = false;
        }
    }
}


auto Edu::CheckCrc32(std::span<Byte> data) -> Result<void>
{
    auto const computedCrc32 = utility::Crc32(data);

    // DEBUG
    // RODOS::PRINTF("\nComputed CRC: ");
    // auto crcSerial = serial::Serialize(computedCrc32);
    // Print(crcSerial);
    // RODOS::PRINTF("\n");
    // END DEBUG


    auto crc32Buffer = serial::SerialBuffer<ts::uint32_t>{};
    auto receiveResult = UartReceive(crc32Buffer);

    // DEBUG
    // RODOS::PRINTF("Received CRC: ");
    // Print(crc32Buffer);
    // RODOS::PRINTF("\n");
    // END DEBUG

    if(receiveResult.has_error())
    {
        return receiveResult;
    }
    if(computedCrc32 != serial::Deserialize<ts::uint32_t>(crc32Buffer))
    {
        return ErrorCode::wrongChecksum;
    }
}


auto Print(std::span<Byte> data, int nRows) -> void
{
    auto iRows = 0;
    for(auto byte : data)
    {
        RODOS::PRINTF("%c", static_cast<char>(byte));
        iRows++;
        if(iRows == nRows)
        {
            RODOS::PRINTF("\n");
            iRows = 0;
        }
    }
    RODOS::PRINTF("\n");
}
}

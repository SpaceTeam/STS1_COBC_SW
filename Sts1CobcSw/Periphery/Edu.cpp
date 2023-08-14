#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::operator""_b;
using sts1cobcsw::serial::Byte;

using ts::operator""_u16;
using ts::operator""_usize;


Edu edu = Edu();


// TODO: Turn this into Bytes, maybe even an enum class : Byte
// CEP basic commands (see EDU PDD)
constexpr auto cmdAck = 0xd7_b;   //! Acknowledging a data packet
constexpr auto cmdNack = 0x27_b;  //! Not Acknowledging a (invalid) data packet
constexpr auto cmdEof = 0x59_b;   //! Transmission of multiple packets is complete
constexpr auto cmdStop = 0xb4_b;  //! Transmission of multiple packets should be stopped
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
auto Edu::StoreArchive(StoreArchiveData const & data) -> std::int32_t
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
//! @returns A relevant EduErrorCode
auto Edu::ExecuteProgram(ExecuteProgramData const & data) -> EduErrorCode
{
    RODOS::PRINTF("ExecuteProgram(programId = %d, queueId = %d, timeout = %d)\n",
                  data.programId,
                  data.queueId,
                  data.timeout);
    // Check if data command was successful
    auto serialData = serial::Serialize(data);
    auto errorCode = SendData(serialData);
    if(errorCode != EduErrorCode::success)
    {
        return errorCode;
    }

    // eduTimeout != timeout argument for data!
    // timeout specifies the time the student program has to execute
    // eduTimeout is the max. allowed time to reveice N/ACK from EDU
    auto answer = 0x00_b;
    uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

    auto nReadBytes = uart_.read(&answer, 1);
    if(nReadBytes == 0)
    {
        return EduErrorCode::timeout;
    }
    switch(answer)
    {
        case cmdAck:
        {
            return EduErrorCode::success;
        }
        case cmdNack:
        {
            return EduErrorCode::nack;
        }
        default:
        {
            return EduErrorCode::invalidResult;
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
auto Edu::StopProgram() -> EduErrorCode
{
    return EduErrorCode::success;
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
auto Edu::GetStatus() -> EduStatus
{
    RODOS::PRINTF("GetStatus()\n");
    auto serialData = serial::Serialize(getStatusId);
    auto sendDataError = SendData(serialData);
    if(sendDataError != EduErrorCode::success)
    {
        RODOS::PRINTF("  Returned .statusType = %d, .errorCode = %d\n",
                      EduStatusType::invalid,
                      sendDataError);
        return EduStatus{.statusType = EduStatusType::invalid, .errorCode = sendDataError};
    }

    EduStatus status;
    std::size_t errorCount = 0;
    do
    {
        status = GetStatusCommunication();
        if(status.errorCode == EduErrorCode::success)
        {
            SendCommand(cmdAck);
            break;
        }
        FlushUartBuffer();
        SendCommand(cmdNack);
    } while(errorCount++ < maxNNackRetries);

    RODOS::PRINTF(
        "  .statusType = %d\n  .errorCode = %d\n  .programId = %d\n  .queueId = %d\n  exitCode = "
        "%d\n",
        status.statusType,
        status.errorCode,
        status.programId,
        status.queueId,
        status.exitCode);
    return status;
}


//! @brief Communication function for GetStatus() to separate a single try from
//! retry logic.
//! @returns The received EDU status
auto Edu::GetStatusCommunication() -> EduStatus
{
    // Get header data
    auto headerBuffer = serial::SerialBuffer<HeaderData>{};
    auto headerReceiveError = UartReceive(headerBuffer);
    auto headerData = serial::Deserialize<HeaderData>(headerBuffer);

    if(headerReceiveError != EduErrorCode::success)
    {
        return EduStatus{.statusType = EduStatusType::invalid, .errorCode = headerReceiveError};
    }

    if(headerData.command != cmdData)
    {
        return EduStatus{.statusType = EduStatusType::invalid,
                         .errorCode = EduErrorCode::invalidCommand};
    }

    if(headerData.length == 0_u16)
    {
        return EduStatus{.statusType = EduStatusType::invalid,
                         .errorCode = EduErrorCode::invalidLength};
    }

    // Get the status type code
    auto statusType = 0_b;
    auto statusErrorCode = UartReceive(&statusType);

    if(statusErrorCode != EduErrorCode::success)
    {
        return EduStatus{.statusType = EduStatusType::invalid, .errorCode = statusErrorCode};
    }

    if(statusType == noEventCode)
    {
        if(headerData.length != nNoEventBytes)
        {
            return EduStatus{.statusType = EduStatusType::invalid,
                             .errorCode = EduErrorCode::invalidLength};
        }

        std::array<Byte, 1> statusTypeArray = {statusType};
        auto crc32Error = CheckCrc32(std::span<Byte>(statusTypeArray));
        if(crc32Error != EduErrorCode::success)
        {
            return EduStatus{.statusType = EduStatusType::invalid, .errorCode = crc32Error};
        }

        return EduStatus{.statusType = EduStatusType::noEvent,
                         .programId = 0,
                         .queueId = 0,
                         .exitCode = 0,
                         .errorCode = EduErrorCode::success};
    }

    if(statusType == programFinishedCode)
    {
        if(headerData.length != nProgramFinishedBytes)
        {
            return EduStatus{.statusType = EduStatusType::invalid,
                             .errorCode = EduErrorCode::invalidLength};
        }

        auto dataBuffer = serial::SerialBuffer<ProgramFinishedStatus>{};
        auto programFinishedError = UartReceive(dataBuffer);

        if(programFinishedError != EduErrorCode::success)
        {
            return EduStatus{.statusType = EduStatusType::invalid,
                             .errorCode = programFinishedError};
        }

        // Create another Buffer which includes the status type that was received beforehand because
        // it is needed to calculate the CRC32 checksum
        auto fullDataBuffer = std::array<Byte, dataBuffer.size() + 1>{};
        fullDataBuffer[0] = statusType;
        std::copy(dataBuffer.begin(), dataBuffer.end(), fullDataBuffer.begin() + 1);
        auto crc32Error = CheckCrc32(fullDataBuffer);
        if(crc32Error != EduErrorCode::success)
        {
            return EduStatus{.statusType = EduStatusType::invalid, .errorCode = crc32Error};
        }

        auto programFinishedData = serial::Deserialize<ProgramFinishedStatus>(dataBuffer);
        return EduStatus{.statusType = EduStatusType::programFinished,
                         .programId = programFinishedData.programId,
                         .queueId = programFinishedData.queueId,
                         .exitCode = programFinishedData.exitCode,
                         .errorCode = EduErrorCode::success};
    }

    if(statusType == resultsReadyCode)
    {
        if(headerData.length != nResultsReadyBytes)
        {
            return EduStatus{.statusType = EduStatusType::invalid,
                             .errorCode = EduErrorCode::invalidLength};
        }

        auto dataBuffer = serial::SerialBuffer<ResultsReadyStatus>{};
        auto resultsReadyError = UartReceive(dataBuffer);
        if(resultsReadyError != EduErrorCode::success)
        {
            return EduStatus{.statusType = EduStatusType::invalid, .errorCode = resultsReadyError};
        }

        // Create another Buffer which includes the status type that was received beforehand because
        // it is needed to calculate the CRC32 checksum
        auto fullDataBuffer = std::array<Byte, dataBuffer.size() + 1>{};
        fullDataBuffer[0] = statusType;
        std::copy(dataBuffer.begin(), dataBuffer.end(), fullDataBuffer.begin() + 1);
        auto crc32Error = CheckCrc32(fullDataBuffer);
        if(crc32Error != EduErrorCode::success)
        {
            return EduStatus{.statusType = EduStatusType::invalid, .errorCode = crc32Error};
        }
        auto resultsReadyData = serial::Deserialize<ResultsReadyStatus>(dataBuffer);
        return EduStatus{.statusType = EduStatusType::resultsReady,
                         .programId = resultsReadyData.programId,
                         .queueId = resultsReadyData.queueId,
                         .errorCode = EduErrorCode::success};
    }

    return EduStatus{.statusType = EduStatusType::invalid,
                     .errorCode = EduErrorCode::invalidStatusType};
}


auto Edu::ReturnResult() -> ResultInfo
{
    // DEBUG
    RODOS::PRINTF("ReturnResult()\n");
    // END DEBUG

    // Send command
    auto serialCommand = serial::Serialize(returnResultId);
    auto commandError = SendData(serialCommand);
    if(commandError != EduErrorCode::success)
    {
        return ResultInfo{.errorCode = commandError, .resultSize = 0U};
    }

    // DEBUG
    // RODOS::PRINTF("\nStart receiving result\n");
    // END DEBUG

    ts::size_t totalResultSize = 0_usize;
    ts::size_t packets = 0_usize;
    ResultInfo resultInfo;
    // TODO: Turn into for loop
    while(packets < maxNPackets)
    {
        // DEBUG
        // RODOS::PRINTF("\nPacket %d\n", static_cast<int>(packets.get()));
        // END DEBUG
        resultInfo = ReturnResultRetry();
        // DEBUG
        RODOS::PRINTF("ResultInfo{errorCode = %d, resultSize = %d}\n",
                      static_cast<int>(resultInfo.errorCode),
                      static_cast<int>(resultInfo.resultSize.get()));
        // END DEBUG
        if(resultInfo.errorCode != EduErrorCode::success)
        {
            break;
        }
        // RODOS::PRINTF("\nWriting to file...\n");
        // TODO: Actually write to a file

        totalResultSize += resultInfo.resultSize;
        packets++;
    }
    return ResultInfo{.errorCode = resultInfo.errorCode, .resultSize = totalResultSize};
}


//! @brief This function handles the retry logic for a single transmission round and is called by
//! the actual ReturnResult function. The communication happens in ReturnResultCommunication.
//!
//! @returns An error code and the number of received bytes in ResultInfo
auto Edu::ReturnResultRetry() -> ResultInfo
{
    ResultInfo resultInfo;
    std::size_t errorCount = 0U;
    do
    {
        resultInfo = ReturnResultCommunication();
        if(resultInfo.errorCode == EduErrorCode::success
           or resultInfo.errorCode == EduErrorCode::successEof)
        {
            SendCommand(cmdAck);
            return resultInfo;
        }
        FlushUartBuffer();
        SendCommand(cmdNack);
    } while(errorCount++ < maxNNackRetries);
    return resultInfo;
}


// This function writes the result to the COBC file system (flash). Maybe it doesn't do that
// directly and instead writes to a non-primary RAM bank as an intermediate step.
//
// Simple results -> 1 round should work with DMA to RAM
auto Edu::ReturnResultCommunication() -> ResultInfo
{
    // Receive command
    // If no result is available, the command will be NACK,
    // otherwise DATA
    Byte command = 0_b;
    auto commandError = UartReceive(&command);
    if(commandError != EduErrorCode::success)
    {
        return ResultInfo{.errorCode = commandError, .resultSize = 0U};
    }
    if(command == cmdNack)
    {
        // TODO: necessary to differentiate errors or just return success with resultSize 0?
        return ResultInfo{.errorCode = EduErrorCode::noResultAvailable, .resultSize = 0U};
    }
    if(command == cmdEof)
    {
        return ResultInfo{.errorCode = EduErrorCode::successEof, .resultSize = 0U};
    }
    if(command != cmdData)
    {
        // DEBUG
        RODOS::PRINTF("\nNot DATA command\n");
        // END DEBUG
        return ResultInfo{.errorCode = EduErrorCode::invalidCommand, .resultSize = 0U};
    }

    // DEBUG
    // RODOS::PRINTF("\nGet Length\n");
    // END DEBUG

    auto dataLengthBuffer = serial::SerialBuffer<ts::uint16_t>{};
    auto lengthError = UartReceive(dataLengthBuffer);
    if(lengthError != EduErrorCode::success)
    {
        return ResultInfo{.errorCode = lengthError, .resultSize = 0U};
    }

    auto actualDataLength = serial::Deserialize<ts::uint16_t>(dataLengthBuffer);
    if(actualDataLength == 0U or actualDataLength > maxDataLength)
    {
        return ResultInfo{.errorCode = EduErrorCode::invalidLength, .resultSize = 0U};
    }

    // DEBUG
    // RODOS::PRINTF("\nGet Data\n");
    // END DEBUG

    // Get the actual data
    auto dataError = UartReceive(
        std::span<Byte>(cepDataBuffer.begin(), cepDataBuffer.begin() + actualDataLength.get()));

    if(dataError != EduErrorCode::success)
    {
        return ResultInfo{.errorCode = dataError, .resultSize = 0U};
    }

    // DEBUG
    // RODOS::PRINTF("\nCheck CRC\n");
    // END DEBUG

    auto crc32Error = CheckCrc32(
        std::span<Byte>(cepDataBuffer.begin(), cepDataBuffer.begin() + actualDataLength.get()));

    if(crc32Error != EduErrorCode::success)
    {
        return ResultInfo{.errorCode = crc32Error, .resultSize = 0U};
    }

    // DEBUG
    RODOS::PRINTF("\nSuccess\n");
    // END DEBUG

    return {EduErrorCode::success, actualDataLength.get()};
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
auto Edu::UpdateTime(UpdateTimeData const & data) -> EduErrorCode
{
    RODOS::PRINTF("UpdateTime()\n");
    auto serialData = serial::Serialize(data);
    auto errorCode = SendData(serialData);
    if(errorCode != EduErrorCode::success)
    {
        return errorCode;
    }

    // On success, wait for second N/ACK
    // TODO: (Daniel) Change to UartReceive()
    // TODO: Refactor this common pattern into a function
    // TODO: Implement read functions that return a type and internally use Deserialize<T>()
    auto answer = 0x00_b;
    uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

    auto nReadBytes = uart_.read(&answer, 1);
    if(nReadBytes == 0)
    {
        return EduErrorCode::timeout;
    }
    switch(answer)
    {
        case cmdAck:
        {
            return EduErrorCode::success;
        }
        case cmdNack:
        {
            return EduErrorCode::nack;
        }
        default:
        {
            return EduErrorCode::invalidResult;
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
auto Edu::SendData(std::span<Byte> data) -> EduErrorCode
{
    std::size_t nBytes = data.size();
    if(nBytes >= maxDataLength)
    {
        return EduErrorCode::sendDataTooLong;
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
        auto answer = 0xAA_b;
        uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

        auto nReadBytes = uart_.read(&answer, 1);
        if(nReadBytes == 0)
        {
            return EduErrorCode::timeout;
        }
        // RODOS::PRINTF("[Edu] answer in sendData is now : %c\n", static_cast<char>(answer));
        switch(answer)
        {
            case cmdAck:
            {
                return EduErrorCode::success;
            }
            case cmdNack:
            {
                nackCount++;
                continue;
            }
            default:
            {
                return EduErrorCode::invalidResult;
            }
        }
    }
    return EduErrorCode::tooManyNacks;
}


//! @brief Receive nBytes bytes over the EDU UART in a single round.
//!
//! @param destination The destination container
//!
//! @returns A relevant EDU error code
// TODO: Use hal::ReadFrom()
auto Edu::UartReceive(std::span<Byte> destination) -> EduErrorCode
{
    if(size(destination) > maxDataLength)
    {
        return EduErrorCode::receiveDataTooLong;
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
            return EduErrorCode::timeout;
        }
        totalReceivedBytes += nReceivedBytes;
    }
    return EduErrorCode::success;
}


//! @brief Receive a single byte over the EDU UART.
//!
//! @param destination The destination byte
//!
//! @returns A relevant EDU error code
// TODO: Use hal::ReadFrom()
auto Edu::UartReceive(void * destination) -> EduErrorCode
{
    uart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
    auto nReceivedBytes = uart_.read(destination, 1);
    if(nReceivedBytes == 0)
    {
        return EduErrorCode::timeout;
    }
    return EduErrorCode::success;
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


auto Edu::CheckCrc32(std::span<Byte> data) -> EduErrorCode
{
    uint32_t computedCrc32 = utility::Crc32(data);

    // DEBUG
    // RODOS::PRINTF("\nComputed CRC: ");
    // auto crcSerial = serial::Serialize(computedCrc32);
    // Print(crcSerial);
    // RODOS::PRINTF("\n");
    // END DEBUG


    auto crc32Buffer = serial::SerialBuffer<ts::uint32_t>{};
    auto receiveError = UartReceive(crc32Buffer);

    // DEBUG
    // RODOS::PRINTF("Received CRC: ");
    // Print(crc32Buffer);
    // RODOS::PRINTF("\n");
    // END DEBUG

    if(receiveError != EduErrorCode::success)
    {
        return receiveError;
    }
    if(computedCrc32 != serial::Deserialize<ts::uint32_t>(crc32Buffer))
    {
        return EduErrorCode::wrongChecksum;
    }
    return EduErrorCode::success;
}


auto Print(std::span<Byte> data, int nRows) -> void
{
    auto iRows = 0;
    for(auto x : data)
    {
        RODOS::PRINTF("%c", static_cast<char>(x));
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

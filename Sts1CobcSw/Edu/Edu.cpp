#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Names.hpp>
#include <Sts1CobcSw/Edu/Structs.hpp>
#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstddef>


namespace sts1cobcsw::edu
{
auto eduEnableGpioPin = hal::GpioPin(hal::eduEnablePin);
auto uart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);

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
constexpr std::size_t maxNPackets = 100;
// Max. length of a single data packet
constexpr auto maxDataLength = 32768;
// Data buffer for potentially large data sizes (ReturnResult and StoreArchive)
auto cepDataBuffer = std::array<Byte, maxDataLength>{};


// TODO: Rework -> Send(EduBasicCommand command) -> void;
auto SendCommand(Byte commandId) -> void;
[[nodiscard]] auto SendData(std::span<Byte> data) -> Result<void>;
// TODO: Make this read and return a Type instead of having to provide a destination. Use
// Deserialize<>() internally.
[[nodiscard]] auto UartReceive(std::span<Byte> destination) -> Result<void>;
[[nodiscard]] auto UartReceive(void * destination) -> Result<void>;
auto FlushUartBuffer() -> void;
[[nodiscard]] auto CheckCrc32(std::span<Byte> data) -> Result<void>;
[[nodiscard]] auto GetStatusCommunication() -> Result<Status>;
[[nodiscard]] auto ReturnResultCommunication() -> Result<ResultInfo>;
[[nodiscard]] auto ReturnResultRetry() -> Result<ResultInfo>;

void MockWriteToFile(std::span<Byte> data);
auto Print(std::span<Byte> data, int nRows = 30) -> void;  // NOLINT


//! @brief  Must be called in an init() function of a thread.
auto Initialize() -> void
{
    eduEnableGpioPin.Direction(hal::PinDirection::out);
    // TODO: I think we should actually read from persistent state to determine whether the EDU
    // should be powered or not. We do have a separate EDU power management thread which though.
    TurnOff();
}


auto TurnOn() -> void
{
    persistentstate::EduShouldBePowered(/*value=*/true);
    eduEnableGpioPin.Set();

    // TODO: Test how high we can set the baudrate without problems (bit errors, etc.)
    constexpr auto baudRate = 921'600;
    uart.init(baudRate);
}


auto TurnOff() -> void
{
    persistentstate::EduShouldBePowered(/*value=*/false);
    eduEnableGpioPin.Reset();
    uart.reset();
}


// TODO: Implement this
auto StoreArchive([[maybe_unused]] StoreArchiveData const & data) -> Result<std::int32_t>
{
    return 0;
}


//! @brief Issues a command to execute a student program on the EDU.
//!
//! Execute Program (COBC <-> EDU):
//! -> [DATA]
//! -> [Command Header]
//! -> [Program ID]
//! -> [Start Time]
//! -> [Timeout]
//! <- [N/ACK]
//! <- [N/ACK]
//!
//! The first N/ACK confirms a valid data packet,
//! the second N/ACK confirms that the program has been started.
//!
//! @param programId The student program ID
//! @param startTime The student program start time
//! @param timeout The available execution time for the student program
//!
//! @returns A relevant error code
auto ExecuteProgram(ExecuteProgramData const & data) -> Result<void>
{
    RODOS::PRINTF("ExecuteProgram(programId = %d, startTime = %" PRIi32 ", timeout = %d)\n",
                  data.programId,
                  data.startTime,
                  data.timeout);
    // Check if data command was successful
    auto serialData = Serialize(data);
    OUTCOME_TRY(SendData(serialData));

    // eduTimeout != timeout argument for data!
    // timeout specifies the time the student program has to execute
    // eduTimeout is the max. allowed time to reveice N/ACK from EDU
    auto answer = 0x00_b;
    uart.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

    auto nReadBytes = uart.read(&answer, 1);
    if(nReadBytes == 0)
    {
        return ErrorCode::timeout;
    }
    switch(answer)
    {
        case cmdAck:
        {
            return outcome_v2::success();
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
auto StopProgram() -> Result<void>
{
    return outcome_v2::success();
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
auto GetStatus() -> Result<Status>
{
    RODOS::PRINTF("GetStatus()\n");
    auto serialData = Serialize(getStatusId);
    OUTCOME_TRY(SendData(serialData));

    std::size_t errorCount = 0;

    while(true)
    {
        auto status = GetStatusCommunication();
        if(status)
        {
            SendCommand(cmdAck);
            RODOS::PRINTF(
                "  .statusType = %d\n  .programId = %d\n  .startTime = %d\n  exitCode = %d\n",
                status.value().statusType,
                status.value().programId,
                status.value().startTime,
                status.value().exitCode);
            return status;
        }
        // Error in GetStatusCommunication()
        FlushUartBuffer();
        SendCommand(cmdNack);
        errorCount++;

        if(errorCount >= maxNNackRetries)
        {
            RODOS::PRINTF("  .errorCode = %d\n", status.error());
            return status.error();
        }
    }
}


//! @brief Communication function for GetStatus() to separate a single try from
//! retry logic.
//! @returns The received EDU status
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto GetStatusCommunication() -> Result<Status>
{
    // Get header data
    auto headerBuffer = Buffer<HeaderData>{};
    OUTCOME_TRY(UartReceive(headerBuffer));
    auto headerData = Deserialize<HeaderData>(headerBuffer);

    if(headerData.command != cmdData)
    {
        return ErrorCode::invalidCommand;
        // return Status{.statusType = StatusType::invalid, .errorCode = ErrorCode::invalidCommand};
    }

    if(headerData.length == 0U)
    {
        return ErrorCode::invalidLength;
    }

    // Get the status type code
    auto statusType = 0_b;
    OUTCOME_TRY(UartReceive(&statusType));

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
            .statusType = StatusType::noEvent, .programId = 0, .startTime = 0, .exitCode = 0};
    }

    if(statusType == programFinishedCode)
    {
        if(headerData.length != nProgramFinishedBytes)
        {
            return ErrorCode::invalidLength;
        }

        auto dataBuffer = Buffer<ProgramFinishedStatus>{};
        OUTCOME_TRY(UartReceive(dataBuffer));

        // Create another Buffer which includes the status type that was received beforehand because
        // it is needed to calculate the CRC32 checksum
        auto fullDataBuffer = std::array<Byte, dataBuffer.size() + 1>{};
        fullDataBuffer[0] = statusType;
        std::copy(dataBuffer.begin(), dataBuffer.end(), fullDataBuffer.begin() + 1);
        OUTCOME_TRY(CheckCrc32(fullDataBuffer));

        auto programFinishedData = Deserialize<ProgramFinishedStatus>(dataBuffer);
        return Status{.statusType = StatusType::programFinished,
                      .programId = programFinishedData.programId,
                      .startTime = programFinishedData.startTime,
                      .exitCode = programFinishedData.exitCode};
    }

    if(statusType == resultsReadyCode)
    {
        if(headerData.length != nResultsReadyBytes)
        {
            return ErrorCode::invalidLength;
        }

        auto dataBuffer = Buffer<ResultsReadyStatus>{};
        auto resultsReadyError = UartReceive(dataBuffer);
        OUTCOME_TRY(UartReceive(dataBuffer));

        // Create another Buffer which includes the status type that was received beforehand because
        // it is needed to calculate the CRC32 checksum
        auto fullDataBuffer = std::array<Byte, dataBuffer.size() + 1>{};
        fullDataBuffer[0] = statusType;
        std::copy(dataBuffer.begin(), dataBuffer.end(), fullDataBuffer.begin() + 1);
        OUTCOME_TRY(CheckCrc32(fullDataBuffer));

        auto resultsReadyData = Deserialize<ResultsReadyStatus>(dataBuffer);
        return Status{.statusType = StatusType::resultsReady,
                      .programId = resultsReadyData.programId,
                      .startTime = resultsReadyData.startTime};
    }

    return ErrorCode::invalidStatusType;
}


auto ReturnResult() -> Result<ResultInfo>
{
    // DEBUG
    RODOS::PRINTF("ReturnResult()\n");
    // END DEBUG

    // Send command
    auto serialCommand = Serialize(returnResultId);
    auto commandError = SendData(serialCommand);
    if(commandError.has_error())
    {
        return commandError.error();
    }

    // DEBUG
    // RODOS::PRINTF("\nStart receiving result\n");
    // END DEBUG

    std::size_t totalResultSize = 0U;
    std::size_t packets = 0U;
    //  TODO: Turn into for loop
    while(packets < maxNPackets)
    {
        // DEBUG
        // RODOS::PRINTF("\nPacket %d\n", static_cast<int>(packets));
        // END DEBUG
        auto resultInfo = ReturnResultRetry();
        // TYPE Result<something>
        // DEBUG

        // Break if returned an error or reached EOF
        if(resultInfo.has_error())
        {
            auto errorCode = resultInfo.error();
            RODOS::PRINTF(" ResultResultRetry() resulted in an error : %d",
                          static_cast<int>(errorCode));
            return resultInfo.error();
        }
        if(resultInfo.value().eofIsReached)
        {
            RODOS::PRINTF(" ResultResultRetry() reached EOF\n");
            return ResultInfo{.eofIsReached = true, .resultSize = totalResultSize};
        }

        // END DEBUG
        // RODOS::PRINTF("\nWriting to file...\n");
        // TODO: Actually write to a file

        totalResultSize += resultInfo.value().resultSize;
        packets++;
    }
    return ResultInfo{.eofIsReached = false, .resultSize = totalResultSize};
}


//! @brief This function handles the retry logic for a single transmission round and is called by
//! the actual ReturnResult function. The communication happens in ReturnResultCommunication.
//!
//! @returns An error code and the number of received bytes in ResultInfo
auto ReturnResultRetry() -> Result<ResultInfo>
{
    std::size_t errorCount = 0U;

    // TODO: infinite loop could be avoided by setting
    // errorCount <= maxNNackRetries as the termination condition
    while(true)
    {
        auto resultInfo = ReturnResultCommunication();
        if(resultInfo.has_value())
        {
            SendCommand(cmdAck);
            // returns {eofIsReached, resultSize}
            return resultInfo.value();
        }

        // Error in ReturnResultCommunication()
        FlushUartBuffer();
        SendCommand(cmdNack);
        errorCount++;
        if(errorCount == maxNNackRetries)
        {
            return resultInfo.error();
        }
    }


    // Result<ResultInfo> result = ErrorCode::noErrorCodeSet;
    // std::size_t errorCount = 0U;
    // // TODO: CHange this
    // do
    // {
    //     result = ReturnResultCommunication();
    //     // Could have reached EOF or not
    //     if(result.has_value())
    //     {
    //         SendCommand(cmdAck);
    //         return result;
    //     }
    //     FlushUartBuffer();
    //     SendCommand(cmdNack);
    // } while(errorCount++ < maxNNackRetries);
    // return result.value();
}


// This function writes the result to the COBC file system (flash). Maybe it doesn't do that
// directly and instead writes to a non-primary RAM bank as an intermediate step.
//
// Simple results -> 1 round should work with DMA to RAM
auto ReturnResultCommunication() -> Result<edu::ResultInfo>
{
    // Receive command
    // If no result is available, the command will be NACK,
    // otherwise DATA
    Byte command = 0_b;
    auto commandError = UartReceive(&command);
    if(commandError.has_error())
    {
        return commandError.error();
    }
    if(command == cmdNack)
    {
        // TODO: necessary to differentiate errors or just return success with resultSize 0?
        return ErrorCode::noResultAvailable;
    }
    if(command == cmdEof)
    {
        return ResultInfo{.eofIsReached = true, .resultSize = 0U};
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

    auto dataLengthBuffer = Buffer<std::uint16_t>{};
    OUTCOME_TRY(UartReceive(dataLengthBuffer));

    auto actualDataLength = Deserialize<std::uint16_t>(dataLengthBuffer);
    if(actualDataLength == 0U or actualDataLength > maxDataLength)
    {
        return ErrorCode::invalidLength;
    }

    // DEBUG
    // RODOS::PRINTF("\nGet Data\n");
    // END DEBUG

    // Get the actual data
    auto dataError = UartReceive(
        std::span<Byte>(cepDataBuffer.begin(), cepDataBuffer.begin() + actualDataLength));

    // TODO: OUTCOME_TRY
    if(dataError.has_error())
    {
        return dataError.error();
    }

    // DEBUG
    // RODOS::PRINTF("\nCheck CRC\n");
    // END DEBUG

    auto crc32Error = CheckCrc32(
        std::span<Byte>(cepDataBuffer.begin(), cepDataBuffer.begin() + actualDataLength));

    if(crc32Error.has_error())
    {
        return crc32Error.error();
    }

    // DEBUG
    RODOS::PRINTF("\nSuccess\n");
    // END DEBUG

    return ResultInfo{.eofIsReached = false, .resultSize = actualDataLength};
}


//! @brief Issues a command to update the EDU time.
//!
//! Update Time:
//! -> [DATA]
//! -> [Command Header]
//! -> [Current Time]
//! <- [N/ACK]
//! <- [N/ACK]
//!
//! The first N/ACK confirms a valid data packet,
//! the second N/ACK confirms the time update.
//!
//! @param currentTime A unix timestamp
//!
//! @returns A relevant error code
auto UpdateTime(UpdateTimeData const & data) -> Result<void>
{
    RODOS::PRINTF("UpdateTime()\n");
    auto serialData = Serialize(data);
    OUTCOME_TRY(SendData(serialData));

    // On success, wait for second N/ACK
    // TODO: (Daniel) Change to UartReceive()
    // TODO: Refactor this common pattern into a function
    // TODO: Implement read functions that return a type and internally use Deserialize<T>()
    auto answer = 0x00_b;
    uart.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

    auto nReadBytes = uart.read(&answer, 1);
    if(nReadBytes == 0)
    {
        return ErrorCode::timeout;
    }
    switch(answer)
    {
        // case cmdAck:
        //{
        //     return ErrorCode::success;
        // }
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
void SendCommand(Byte commandId)
{
    auto data = std::array{commandId};
    // TODO: ambiguity when using arrays directly with Write operations (Communication.hpp)
    hal::WriteTo(&uart, std::span(data));
}


//! @brief Send a data packet over UART to the EDU.
//!
//! @param data The data to be sent
auto SendData(std::span<Byte> data) -> Result<void>
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
        hal::WriteTo(&uart, std::span<std::uint16_t>(len));
        hal::WriteTo(&uart, data);
        hal::WriteTo(&uart, std::span<std::uint32_t>(crc));

        // TODO: Refactor this common pattern into a function
        // Data is always answered by N/ACK
        auto answer = 0xAA_b;  // Why is this set to 0xAA?
        uart.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

        auto nReadBytes = uart.read(&answer, 1);
        if(nReadBytes == 0)
        {
            return ErrorCode::timeout;
        }
        // RODOS::PRINTF("[Edu] answer in sendData is now : %c\n", static_cast<char>(answer));
        switch(answer)
        {
            // TODO: Return outcome::success
            case cmdAck:
            {
                return outcome_v2::success();
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
auto UartReceive(std::span<Byte> destination) -> Result<void>
{
    if(size(destination) > maxDataLength)
    {
        return ErrorCode::receiveDataTooLong;
    }

    std::size_t totalReceivedBytes = 0U;
    const auto destinationSize = size(destination);
    while(totalReceivedBytes < destinationSize)
    {
        uart.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
        auto nReceivedBytes =
            uart.read(data(destination) + totalReceivedBytes, destinationSize - totalReceivedBytes);
        if(nReceivedBytes == 0)
        {
            return ErrorCode::timeout;
        }
        totalReceivedBytes += nReceivedBytes;
    }
    return outcome_v2::success();
}


//! @brief Receive a single byte over the EDU UART.
//!
//! @param destination The destination byte
//!
//! @returns A relevant EDU error code
// TODO: Use hal::ReadFrom()
auto UartReceive(void * destination) -> Result<void>
{
    uart.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
    auto nReceivedBytes = uart.read(destination, 1);
    if(nReceivedBytes == 0)
    {
        return ErrorCode::timeout;
    }
    return outcome_v2::success();
}


//! @brief Flush the EDU UART read buffer.
//!
//! This can be used to clear all buffer data after an error to request a resend.
auto FlushUartBuffer() -> void
{
    auto garbageBuffer = std::array<Byte, garbageBufferSize>{};
    auto dataReceived = true;

    // Keep reading until no data is coming for flushTimeout
    while(dataReceived)
    {
        uart.suspendUntilDataReady(RODOS::NOW() + flushTimeout);
        auto nReceivedBytes = uart.read(garbageBuffer.data(), garbageBufferSize);
        if(nReceivedBytes == 0)
        {
            dataReceived = false;
        }
    }
}


auto CheckCrc32(std::span<Byte> data) -> Result<void>
{
    auto const computedCrc32 = utility::Crc32(data);

    // DEBUG
    // RODOS::PRINTF("\nComputed CRC: ");
    // auto crcSerial = Serialize(computedCrc32);
    // Print(crcSerial);
    // RODOS::PRINTF("\n");
    // END DEBUG


    auto crc32Buffer = Buffer<std::uint32_t>{};
    OUTCOME_TRY(UartReceive(crc32Buffer));

    // DEBUG
    // RODOS::PRINTF("Received CRC: ");
    // Print(crc32Buffer);
    // RODOS::PRINTF("\n");
    // END DEBUG

    if(computedCrc32 != Deserialize<std::uint32_t>(crc32Buffer))
    {
        return ErrorCode::wrongChecksum;
    }
    return outcome_v2::success();
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

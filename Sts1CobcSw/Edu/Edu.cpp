#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Structs.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstddef>


namespace sts1cobcsw::edu
{
auto eduEnableGpioPin = hal::GpioPin(hal::eduEnablePin);
auto uart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);

// TODO: To be able to better (or at all) distinguish the low-level protocol commands (cmdAck, ...)
// and the high-level EDU commands (update time, ...) rename them CEP commands and EDU commands,
// respectively.

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
constexpr auto nProgramFinishedBytes = 8;
constexpr auto nResultsReadyBytes = 7;

// TODO: Check real timeouts
// Max. time for the EDU to respond to a request
constexpr auto communicationTimeout = 1 * RODOS::SECONDS;
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
constexpr auto maxDataLength = 32 * 1024;
// Data buffer for potentially large data sizes (ReturnResult and StoreProgram)
auto cepDataBuffer = std::array<Byte, maxDataLength>{};


// TODO: Rework -> Send(EduBasicCommand command) -> void;
auto SendCommand(Byte commandId) -> void;
[[nodiscard]] auto SendData(std::span<Byte const> data) -> Result<void>;
// TODO: Make this read and return a Type instead of having to provide a destination. Use
// Deserialize<>() internally.
[[nodiscard]] auto UartReceive(std::span<Byte> destination) -> Result<void>;
[[nodiscard]] auto UartReceive(void * destination) -> Result<void>;
auto FlushUartBuffer() -> void;
[[nodiscard]] auto CheckCrc32(std::span<Byte const> data) -> Result<void>;
[[nodiscard]] auto GetStatusCommunication() -> Result<Status>;
[[nodiscard]] auto ReturnResultCommunication() -> Result<ResultInfo>;
[[nodiscard]] auto ReturnResultRetry() -> Result<ResultInfo>;


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
    auto const baudRate = 115'200;
    hal::Initialize(&uart, baudRate);
}


auto TurnOff() -> void
{
    persistentstate::EduShouldBePowered(/*value=*/false);
    eduEnableGpioPin.Reset();
    uart.reset();
}


// TODO: Implement this
auto StoreProgram([[maybe_unused]] StoreProgramData const & data) -> Result<std::int32_t>
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
    uart.suspendUntilDataReady(RODOS::NOW() + communicationTimeout);

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
    OUTCOME_TRY(SendData(Span(getStatusId)));

    std::size_t nErrors = 0;
    while(true)
    {
        auto getStatusCommunicationResult = GetStatusCommunication();
        if(getStatusCommunicationResult.has_value())
        {
            auto status = getStatusCommunicationResult.value();
            SendCommand(cmdAck);
            RODOS::PRINTF("  .statusType = %d\n  .programId = %d\n  .startTime = %" PRIi32
                          "\n  exitCode = %d\n",
                          static_cast<int>(status.statusType),
                          status.programId,
                          status.startTime,
                          status.exitCode);
            return status;
        }
        FlushUartBuffer();
        SendCommand(cmdNack);
        nErrors++;
        if(nErrors >= maxNNackRetries)
        {
            RODOS::PRINTF("  .errorCode = %d\n",
                          static_cast<int>(getStatusCommunicationResult.error()));
            return getStatusCommunicationResult.error();
        }
    }
}


//! @brief Communication function for GetStatus() to separate a single try from retry logic.
//!
//! @returns The received EDU status
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto GetStatusCommunication() -> Result<Status>
{
    auto headerBuffer = Buffer<HeaderData>{};
    OUTCOME_TRY(UartReceive(headerBuffer));
    auto headerData = Deserialize<HeaderData>(headerBuffer);

    if(headerData.command != cmdData)
    {
        return ErrorCode::invalidCommand;
    }
    if(headerData.length == 0U)
    {
        return ErrorCode::invalidLength;
    }

    auto statusType = 0x00_b;
    OUTCOME_TRY(UartReceive(&statusType));

    if(statusType == noEventCode)
    {
        if(headerData.length != nNoEventBytes)
        {
            return ErrorCode::invalidLength;
        }
        OUTCOME_TRY(CheckCrc32(Span(statusType)));
        return Status{.statusType = StatusType::noEvent};
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


auto ReturnResult(ReturnResultData const & data) -> Result<ResultInfo>
{
    // DEBUG
    RODOS::PRINTF("ReturnResult()\n");
    // END DEBUG

    OUTCOME_TRY(SendData(Serialize(data)));

    // DEBUG
    // RODOS::PRINTF("\nStart receiving result\n");
    // END DEBUG

    std::size_t totalResultSize = 0U;
    std::size_t nPackets = 0U;
    //  TODO: Turn into for loop
    while(nPackets < maxNPackets)
    {
        // DEBUG
        // RODOS::PRINTF("\nPacket %d\n", static_cast<int>(packets));
        // END DEBUG
        auto returnResultRetryResult = ReturnResultRetry();
        // TYPE Result<something>
        // DEBUG

        if(returnResultRetryResult.has_error())
        {
            auto errorCode = returnResultRetryResult.error();
            RODOS::PRINTF(" ReturnResultRetry() resulted in an error : %d",
                          static_cast<int>(errorCode));
            return errorCode;
        }
        if(returnResultRetryResult.value().eofIsReached)
        {
            RODOS::PRINTF(" ReturnResultRetry() reached EOF\n");

            // TODO: This is a dummy implementation. Store the result instead.
            RODOS::AT(RODOS::NOW() + 1 * RODOS::MILLISECONDS);
            SendCommand(cmdAck);

            return ResultInfo{.eofIsReached = true, .resultSize = totalResultSize};
        }

        // END DEBUG
        // RODOS::PRINTF("\nWriting to file...\n");
        // TODO: Actually write to a file

        totalResultSize += returnResultRetryResult.value().resultSize;
        nPackets++;
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
    // TODO: infinite loop could be avoided by setting errorCount <= maxNNackRetries as the
    // termination condition
    while(true)
    {
        auto returnResultCommunicationResult = ReturnResultCommunication();
        if(returnResultCommunicationResult.has_value())
        {
            SendCommand(cmdAck);
            return returnResultCommunicationResult.value();
        }
        FlushUartBuffer();
        SendCommand(cmdNack);
        errorCount++;
        if(errorCount == maxNNackRetries)
        {
            return returnResultCommunicationResult.error();
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
    Byte command = 0x00_b;
    OUTCOME_TRY(UartReceive(&command));
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
    OUTCOME_TRY(UartReceive(Span(&cepDataBuffer).first(actualDataLength)));

    // DEBUG
    // RODOS::PRINTF("\nCheck CRC\n");
    // END DEBUG

    OUTCOME_TRY(CheckCrc32(Span(cepDataBuffer).first(actualDataLength)));

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
    OUTCOME_TRY(SendData(Serialize(data)));

    // TODO: Refactor this common pattern into a function
    // TODO: Implement read functions that return a type and internally use Deserialize<T>()
    auto answer = 0x00_b;
    OUTCOME_TRY(UartReceive(&answer));
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


//! @brief Send a CEP command to the EDU.
//!
//! @param cmd The command
inline auto SendCommand(Byte commandId) -> void
{
    hal::WriteTo(&uart, Span(commandId));
}


//! @brief Send a data packet over UART to the EDU.
//!
//! @param data The data to be sent
auto SendData(std::span<Byte const> data) -> Result<void>
{
    if(data.size() >= maxDataLength)
    {
        return ErrorCode::sendDataTooLong;
    }
    // Casting the size to uint16_t is safe since it is checked against maxDataLength
    auto length = Serialize(static_cast<std::uint16_t>(data.size()));
    auto checksum = Serialize(utility::ComputeCrc32(data));

    auto nNacks = 0;
    while(nNacks < maxNNackRetries)
    {
        SendCommand(cmdData);
        hal::WriteTo(&uart, Span(length));
        hal::WriteTo(&uart, data);
        hal::WriteTo(&uart, Span(checksum));

        // TODO: Refactor this common pattern into a function
        // Data is always answered by N/ACK
        auto answer = 0xAA_b;  // TODO: Why is this set to 0xAA?
        // TODO: Why do we first suspend and then read?
        uart.suspendUntilDataReady(RODOS::NOW() + communicationTimeout);
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
                nNacks++;
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
auto UartReceive(std::span<Byte> destination) -> Result<void>
{
    if(size(destination) > maxDataLength)
    {
        return ErrorCode::receiveDataTooLong;
    }

    std::size_t totalReceivedBytes = 0U;
    auto destinationSize = size(destination);
    while(totalReceivedBytes < destinationSize)
    {
        uart.suspendUntilDataReady(RODOS::NOW() + communicationTimeout);
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
auto UartReceive(void * destination) -> Result<void>
{
    uart.suspendUntilDataReady(RODOS::NOW() + communicationTimeout);
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
    // Keep reading until no data is coming for flushTimeout
    while(true)
    {
        uart.suspendUntilDataReady(RODOS::NOW() + flushTimeout);
        auto nReceivedBytes = uart.read(garbageBuffer.data(), garbageBufferSize);
        if(nReceivedBytes == 0)
        {
            break;
        }
    }
}


// TODO: A parameter pack of spans would be very convenient
auto CheckCrc32(std::span<Byte const> data) -> Result<void>
{
    auto computedCrc32 = utility::ComputeCrc32(data);
    auto crc32Buffer = Buffer<std::uint32_t>{};
    OUTCOME_TRY(UartReceive(crc32Buffer));

    if(computedCrc32 != Deserialize<std::uint32_t>(crc32Buffer))
    {
        return ErrorCode::wrongChecksum;
    }
    return outcome_v2::success();
}
}

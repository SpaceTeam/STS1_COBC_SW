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
// --- Globals ---

// Low-level CEP commands
constexpr auto cepAck = 0xd7_b;   //! Acknowledging a data packet
constexpr auto cepNack = 0x27_b;  //! Not Acknowledging an (invalid) data packet
constexpr auto cepEof = 0x59_b;   //! Transmission of multiple packets is complete
constexpr auto cepData = 0x8b_b;  //! Data packet format is used (not a command packet!)

// TODO: Turn these into <Status>Data structs like the EDU commands
// GetStatus result types
constexpr auto noEventCode = 0x00_b;
constexpr auto programFinishedCode = 0x01_b;
constexpr auto resultsReadyCode = 0x02_b;

// Status types byte counts
constexpr auto nNoEventBytes = 1;
constexpr auto nProgramFinishedBytes = 8;
constexpr auto nResultsReadyBytes = 7;

// TODO: Check real timeouts
// Max. time for EDU to respond to a request
constexpr auto communicationTimeout = 1 * RODOS::SECONDS;

// TODO: Choose proper values
// Max. amount of send retries after receiving NACK
constexpr auto maxNNackRetries = 10;
// Max. number of data packets for a single command
constexpr std::size_t maxNPackets = 100;
// Max. length of a single data packet
constexpr auto maxDataLength = 11 * 1024;
// Data buffer for potentially large data sizes (ReturnResult and StoreProgram)
auto cepDataBuffer = std::array<Byte, maxDataLength>{};

auto eduEnableGpioPin = hal::GpioPin(hal::eduEnablePin);
auto uart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);


// --- Private function declarations ---

// TODO: Rework -> Send(CepCommand command) -> void;
auto SendCommand(Byte commandId) -> void;
[[nodiscard]] auto Send(std::span<Byte const> data) -> Result<void>;
[[nodiscard]] auto Receive(std::span<Byte> data) -> Result<void>;
template<typename T>
[[nodiscard]] auto Receive() -> Result<T>;
template<>
[[nodiscard]] auto Receive<Byte>() -> Result<Byte>;
auto FlushUartReceiveBuffer() -> void;
[[nodiscard]] auto CheckCrc32(std::span<Byte const> data) -> Result<void>;
[[nodiscard]] auto GetStatusCommunication() -> Result<Status>;
[[nodiscard]] auto ReturnResultCommunication() -> Result<ResultInfo>;
[[nodiscard]] auto ReturnResultRetry() -> Result<ResultInfo>;


// --- Function definitions ---

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
    hal::Deinitialize(&uart);
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
    // Check if data command was successful
    OUTCOME_TRY(Send(Serialize(data)));
    OUTCOME_TRY(auto answer, Receive<Byte>());
    switch(answer)
    {
        case cepAck:
        {
            return outcome_v2::success();
        }
        case cepNack:
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
    OUTCOME_TRY(Send(Span(getStatusId)));
    std::size_t nErrors = 0;
    while(true)
    {
        auto getStatusCommunicationResult = GetStatusCommunication();
        if(getStatusCommunicationResult.has_value())
        {
            auto status = getStatusCommunicationResult.value();
            SendCommand(cepAck);
            return status;
        }
        FlushUartReceiveBuffer();
        SendCommand(cepNack);
        nErrors++;
        if(nErrors >= maxNNackRetries)
        {
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
    OUTCOME_TRY(auto answer, Receive<Byte>());
    if(answer != cepData)
    {
        return ErrorCode::invalidCommand;
    }

    OUTCOME_TRY(auto dataLength, Receive<std::int32_t>());
    if(dataLength == 0)
    {
        return ErrorCode::invalidLength;
    }

    OUTCOME_TRY(auto statusType, Receive<Byte>());
    if(statusType == noEventCode)
    {
        if(dataLength != nNoEventBytes)
        {
            return ErrorCode::invalidLength;
        }
        OUTCOME_TRY(CheckCrc32(Span(statusType)));
        return Status{.statusType = StatusType::noEvent};
    }
    if(statusType == programFinishedCode)
    {
        if(dataLength != nProgramFinishedBytes)
        {
            return ErrorCode::invalidLength;
        }

        auto dataBuffer = Buffer<ProgramFinishedStatus>{};
        OUTCOME_TRY(Receive(dataBuffer));

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
        if(dataLength != nResultsReadyBytes)
        {
            return ErrorCode::invalidLength;
        }

        auto dataBuffer = Buffer<ResultsReadyStatus>{};
        OUTCOME_TRY(Receive(dataBuffer));

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
    OUTCOME_TRY(Send(Serialize(data)));

    std::size_t totalResultSize = 0;
    std::size_t nPackets = 0;
    //  TODO: Turn into for loop
    while(nPackets < maxNPackets)
    {
        auto returnResultRetryResult = ReturnResultRetry();
        if(returnResultRetryResult.has_error())
        {
            auto errorCode = returnResultRetryResult.error();
            return errorCode;
        }
        if(returnResultRetryResult.value().eofIsReached)
        {
            // TODO: This is a dummy implementation. Store the result instead.
            RODOS::AT(RODOS::NOW() + 1 * RODOS::MILLISECONDS);
            SendCommand(cepAck);
            return ResultInfo{.eofIsReached = true, .resultSize = totalResultSize};
        }
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
    std::size_t errorCount = 0;
    // TODO: infinite loop could be avoided by setting errorCount <= maxNNackRetries as the
    // termination condition
    while(true)
    {
        auto returnResultCommunicationResult = ReturnResultCommunication();
        if(returnResultCommunicationResult.has_value())
        {
            SendCommand(cepAck);
            return returnResultCommunicationResult.value();
        }
        FlushUartReceiveBuffer();
        SendCommand(cepNack);
        errorCount++;
        if(errorCount == maxNNackRetries)
        {
            return returnResultCommunicationResult.error();
        }
    }
}


// This function writes the result to the COBC file system (flash). Maybe it doesn't do that
// directly and instead writes to a non-primary RAM bank as an intermediate step.
//
// Simple results -> 1 round should work with DMA to RAM
auto ReturnResultCommunication() -> Result<edu::ResultInfo>
{
    OUTCOME_TRY(auto answer, Receive<Byte>());
    if(answer == cepNack)
    {
        // TODO: necessary to differentiate errors or just return success with resultSize 0?
        return ErrorCode::noResultAvailable;
    }
    if(answer == cepEof)
    {
        return ResultInfo{.eofIsReached = true, .resultSize = 0};
    }
    if(answer != cepData)
    {
        return ErrorCode::invalidCommand;
    }

    OUTCOME_TRY(auto dataLength, Receive<std::uint32_t>());
    if(dataLength == 0 or dataLength > maxDataLength)
    {
        return ErrorCode::invalidLength;
    }

    OUTCOME_TRY(Receive(Span(&cepDataBuffer).first(dataLength)));
    OUTCOME_TRY(CheckCrc32(Span(cepDataBuffer).first(dataLength)));
    return ResultInfo{.eofIsReached = false, .resultSize = dataLength};
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
    OUTCOME_TRY(Send(Serialize(data)));
    OUTCOME_TRY(auto answer, Receive<Byte>());
    switch(answer)
    {
        case cepAck:
        {
            return outcome_v2::success();
        }
        case cepNack:
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
auto Send(std::span<Byte const> data) -> Result<void>
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
        SendCommand(cepData);
        hal::WriteTo(&uart, Span(length));
        hal::WriteTo(&uart, data);
        hal::WriteTo(&uart, Span(checksum));

        OUTCOME_TRY(auto answer, Receive<Byte>());
        switch(answer)
        {
            case cepAck:
            {
                return outcome_v2::success();
            }
            case cepNack:
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
auto Receive(std::span<Byte> data) -> Result<void>
{
    if(data.size() > maxDataLength)
    {
        return ErrorCode::receiveDataTooLong;
    }
    auto readFromResult = hal::ReadFrom(&uart, data, communicationTimeout);
    if(readFromResult.has_error())
    {
        return ErrorCode::timeout;
    }
    return outcome_v2::success();
}


template<typename T>
auto Receive() -> Result<T>
{
    auto buffer = Buffer<T>{};
    OUTCOME_TRY(Receive(buffer));
    return Deserialize<T>(buffer);
}


template<>
auto Receive<Byte>() -> Result<Byte>
{
    auto byte = 0x00_b;
    OUTCOME_TRY(Receive(Span(&byte)));
    return byte;
}


//! @brief Flush the EDU UART read buffer.
//!
//! This can be used to clear all buffer data after an error to request a resend.
auto FlushUartReceiveBuffer() -> void
{
    auto garbageBuffer = std::array<Byte, 32>{};  // NOLINT(*magic-numbers)
    // TODO: Can we choose a smaller value?
    auto flushTimeout = 1 * RODOS::MILLISECONDS;
    while(true)
    {
        auto readFromResult = hal::ReadFrom(&uart, Span(&garbageBuffer), flushTimeout);
        if(readFromResult.has_error())
        {
            break;
        }
    }
}


// TODO: A parameter pack of spans would be very convenient
auto CheckCrc32(std::span<Byte const> data) -> Result<void>
{
    auto computedCrc32 = utility::ComputeCrc32(data);
    OUTCOME_TRY(auto receivedCrc32, Receive<std::uint32_t>());
    if(computedCrc32 != receivedCrc32)
    {
        return ErrorCode::wrongChecksum;
    }
    return outcome_v2::success();
}
}

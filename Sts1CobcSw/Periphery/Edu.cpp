#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/UtilityNames.hpp>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::operator""_b;
using sts1cobcsw::serial::Byte;

using ts::operator""_i8;
using ts::operator""_u8;
using ts::operator""_i16;
using ts::operator""_u16;
using ts::operator""_i32;
using ts::operator""_u32;
using ts::operator""_i64;
using ts::operator""_u64;


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

// Max. length for a single round data field
constexpr auto maxDataLength = 32768;

// TODO: Check real timeouts
// Max. time for the EDU to respond to a request
constexpr auto eduTimeout = 5 * RODOS::SECONDS;
// Timeout used when flushing the UART receive buffer
constexpr auto flushTimeout = 1 * RODOS::MILLISECONDS;
// UART flush garbage buffer size
constexpr auto garbageBufferSize = 128;

// GetStatus Constants
// Max. amount of bytes for result of "Get Status" EDU command
constexpr auto maxNStatusBytes = 6;
// Amount of bytes for the length field of a data command
constexpr std::size_t nLengthBytes = 2;
// Amount of bytes for a basic command or a high level command header
constexpr std::size_t nCommandBytes = 1;
// Max. amount of send retries after receiving NACK
constexpr auto maxNNackRetries = 10;


//! @brief  Must be called in an init() function of a thread.
auto Edu::Initialize() -> void
{
    eduEnabledGpioPin_.Direction(hal::PinDirection::out);
    // TODO: I think we should actually read from persistent state to determine whether the EDU
    // should be powered or not. We do have a separate EDU power management thread which though.
    TurnOff();

    constexpr auto baudRate = 115'200;
    uart_.init(baudRate);
}


auto Edu::TurnOn() -> void
{
    // Set EduShouldBePowered to True, persistentstate is initialized in
    // EduPowerManagementThread.cpp
    periphery::persistentstate::EduShouldBePowered(true);
    // Edu enabled pin uses inverted logic
    eduEnabledGpioPin_.Reset();
}


auto Edu::TurnOff() -> void
{
    // Set EduShouldBePowered to False, persistentstate is initialized in
    // EduPowerManagementThread.cpp
    periphery::persistentstate::EduShouldBePowered(false);
    // Edu enabled pin uses inverted logic
    eduEnabledGpioPin_.Set();
}


// TODO: Implement this
[[nodiscard]] auto Edu::StoreArchive(StoreArchiveData const & data) -> std::int32_t
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
[[nodiscard]] auto Edu::ExecuteProgram(ExecuteProgramData const & data) -> EduErrorCode
{
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
[[nodiscard]] auto Edu::StopProgram() -> EduErrorCode
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
[[nodiscard]] auto Edu::GetStatus() -> EduStatus
{
    auto serialData = serial::Serialize(getStatusId);
    auto sendDataError = SendData(serialData);
    if(sendDataError != EduErrorCode::success)
    {
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
            return status;
        }
        FlushUartBuffer();
        SendCommand(cmdNack);
    } while(errorCount++ < maxNNackRetries);

    return status;
}


//! @brief Communication function for GetStatus() to separate a single try from
//! retry logic.
//! @returns The received EDU status
[[nodiscard]] auto Edu::GetStatusCommunication() -> EduStatus
{
    // Get header data
    serial::SerialBuffer<HeaderData> headerBuffer = {};
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
        if(headerData.length != 1)
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

        serial::SerialBuffer<ProgramFinishedStatus> dataBuffer = {};
        auto programFinishedError = UartReceive(dataBuffer);

        if(programFinishedError != EduErrorCode::success)
        {
            return EduStatus{.statusType = EduStatusType::invalid,
                             .errorCode = programFinishedError};
        }

        // Create another Buffer which includes the status type that was received beforehand because
        // it is needed to calculate the CRC32 checksum
        std::array<Byte, dataBuffer.size() + 1> fullDataBuffer = {};
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

        serial::SerialBuffer<ResultsReadyStatus> dataBuffer = {};
        auto resultsReadyError = UartReceive(dataBuffer);
        if(resultsReadyError != EduErrorCode::success)
        {
            return EduStatus{.statusType = EduStatusType::invalid, .errorCode = resultsReadyError};
        }

        // Create another Buffer which includes the status type that was received beforehand because
        // it is needed to calculate the CRC32 checksum
        std::array<Byte, dataBuffer.size() + 1> fullDataBuffer = {};
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


// This function writes the result to the COBC file system (flash). Maybe it doesn't do that
// directly and instead writes to a non-primary RAM bank as an intermediate step.
//
// Simple results -> 1 round should work with DMA to RAM
[[nodiscard]] auto Edu::ReturnResult() -> ResultInfo
{
    return {EduErrorCode::success, 0};
    // If this is the initial call, send the header
    // if(!mResultPending_)
    // {
    //     std::array<std::uint8_t, 1> header = {returnResult};
    //     auto headerErrorCode = SendData(header);
    //     if(headerErrorCode != EduErrorCode::success)
    //     {
    //         return {headerErrorCode, 0};
    //     }
    // }

    // // Afterwards, only process a single data packet at a time, since we
    // // can't guarantee that it will use up all the memory

    // // Start error while loop
    // std::size_t errorCnt = 0;
    // EduStatusType statusTypeRet;
    // bool succesfulRecv = false;
    // while(!succesfulRecv)
    // {
    //     // Receive the header (data command and length)
    //     std::array<std::uint8_t, cmdBytes + lenBytes> recvHeader = {};
    //     auto headerError = UartReceive(recvHeader, cmdBytes + lenBytes);
    //     if(headerError != EduErrorCode::success)
    //     {
    //         // Only retry on timeout errors (-> invalid format after all)
    //         FlushUartBuffer();
    //         if(errorCnt++ < maxNackRetries and headerError == EduErrorCode::errorTimeout)
    //         {
    //             SendCommand(cmdNack);
    //             continue;
    //         }
    //         return {headerError, 0};
    //     }
    //     if(recvHeader[0] == cmdEof)
    //     {
    //         return {EduErrorCode::successEof, 0};
    //     }
    //     if(recvHeader[0] != cmdData)
    //     {
    //         // Invalid header, flush, send NACK, and retry
    //         FlushUartBuffer();
    //         if(errorCnt++ < maxNackRetries)
    //         {
    //             SendCommand(cmdNack);
    //             continue;
    //         }
    //         return {EduErrorCode::errorInvalidResult, 0};
    //     }

    //     // Create 2 byte length from single received bytes
    //     auto const len = utility::BytesTouint16(recvHeader[1], recvHeader[2]);
    //     if(len > maxDataLen)
    //     {
    //         // Invalid length, flush, send NACK, and retry
    //         FlushUartBuffer();
    //         if(errorCnt++ < maxNackRetries)
    //         {
    //             SendCommand(cmdNack);
    //             continue;
    //         }
    //         return {EduErrorCode::errorRecvDataTooLong, 0};
    //     }

    //     // Receive actual status data
    //     // For data, reserve the max. possible bytes
    //     std::array<std::uint8_t, maxDataLen> recvDataBuf = {};
    //     auto recvDataError = UartReceive(recvDataBuf, len);
    //     if(recvDataError != EduErrorCode::success)
    //     {
    //         // Only retry on timeout errors (-> invalid format after all)
    //         FlushUartBuffer();
    //         if(errorCnt++ < maxNackRetries and recvDataError == EduErrorCode::errorTimeout)
    //         {
    //             SendCommand(cmdNack);
    //             continue;
    //         }
    //         return {recvDataError, 0};
    //     }

    //     // Receive checksum
    //     std::array<std::uint8_t, 4> crc32Buf = {};
    //     auto crc32Error = UartReceive(crc32Buf, crc32Buf.size());
    //     if(crc32Error != EduErrorCode::success)
    //     {
    //         // Only retry on timeout errors (-> invalid format after all)
    //         FlushUartBuffer();
    //         if(errorCnt++ < maxNackRetries and crc32Error == EduErrorCode::errorTimeout)
    //         {
    //             SendCommand(cmdNack);
    //             continue;
    //         }
    //         return {crc32Error, 0};
    //     }

    //     // Assemble checksum
    //     auto crc32Recv = utility::BytesTouint32(crc32Buf[0], crc32Buf[1], crc32Buf[2],
    //     crc32Buf[3]); auto crc32Calc = utility::Crc32(recvDataBuf);

    //     // Check checksum against own calculation
    //     if(crc32Recv != crc32Calc)
    //     {
    //         // Checksums don't match, flush, send NACK, and retry
    //         FlushUartBuffer();
    //         if(errorCnt++ < maxNackRetries)
    //         {
    //             SendCommand(cmdNack);
    //             continue;
    //         }
    //         return {EduErrorCode::errorChecksum, 0};
    //     }

    //     // Everything worked, so return success and the number of received bytes
    //     return {EduErrorCode::success, len};

    // }  // End while
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
[[nodiscard]] auto Edu::UpdateTime(UpdateTimeData const & data) -> EduErrorCode
{
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
[[nodiscard]] auto Edu::SendData(std::span<Byte> data) -> EduErrorCode
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
[[nodiscard]] auto Edu::UartReceive(std::span<Byte> destination) -> EduErrorCode
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
[[nodiscard]] auto Edu::UartReceive(Byte * destination) -> EduErrorCode
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
    std::array<Byte, garbageBufferSize> garbageBuffer = {};
    ts::bool_t dataReceived = true;

    // Keep reading until no data is coming for flushTimeout (10 ms)
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
    uint32_t crc32Calculated = utility::Crc32(data);
    serial::SerialBuffer<ts::uint32_t> crc32Buffer = {};
    auto receiveError = UartReceive(crc32Buffer);
    if(receiveError != EduErrorCode::success)
    {
        return receiveError;
    }
    if(crc32Calculated != serial::Deserialize<ts::uint32_t>(crc32Buffer))
    {
        return EduErrorCode::wrongChecksum;
    }
    return EduErrorCode::success;
}
}

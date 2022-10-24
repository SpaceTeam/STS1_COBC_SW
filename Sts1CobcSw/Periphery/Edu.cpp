#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::operator""_b;
using sts1cobcsw::serial::Byte;


// TODO: Turn this into Bytes, maybe even an enum class : Byte
// CEP basic commands (see EDU PDD)
constexpr auto cmdAck = 0xd7_b;   //! Acknowledging a data packet
constexpr auto cmdNack = 0x27_b;  //! Not Acknowledging a (invalid) data packet
constexpr auto cmdEof = 0x59_b;   //! Transmission of multiple packets is complete
constexpr auto cmdStop = 0xb4_b;  //! Transmission of multiple packets should be stopped
constexpr auto cmdData = 0x8b_b;  //! Data packet format is used (not a command packet!)

// GetStatus result types
constexpr auto noEventCode = 0x00;
constexpr auto programFinishedCode = 0x01;
constexpr auto resultsReadyCode = 0x02;

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
constexpr auto maxStatusBytes = 6;
// Amount of bytes for the length field of a data command
constexpr size_t lenBytes = 2;
// Amount of bytes for a basic command or a high level command header
constexpr size_t cmdBytes = 1;
// Max. amount of send retries after receiving NACK
constexpr auto maxNackRetries = 10;


Edu::Edu()
{
    constexpr auto baudRate = 115'200;
    mEduUart_.init(baudRate);
}


// TODO: Implement this
[[nodiscard]] auto Edu::StoreArchive(StoreArchiveData const & data) -> int32_t
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
    mEduUart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

    auto nReadBytes = mEduUart_.read(&answer, 1);
    if(nReadBytes == 0)
    {
        return EduErrorCode::errorTimeout;
    }
    switch(answer)
    {
        case cmdAck:
        {
            return EduErrorCode::success;
        }
        case cmdNack:
        {
            return EduErrorCode::errorNack;
        }
        default:
        {
            return EduErrorCode::errorInvalidResult;
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
    // std::array<uint8_t, 3> dataBuf = {stopProgram};
    // auto errorCode = SendData(dataBuf);

    // if(errorCode != EduErrorCode::success)
    // {
    //     return errorCode;
    // }

    // // Receive second N/ACK to see if program is successfully stopped
    // std::array<uint8_t, 1> recvBuf = {};
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
// TODO: (Daniel) refactor, too complex
// TODO: error handling?
[[nodiscard]] auto Edu::GetStatus() -> EduStatus
{
    // // Values to be returned
    // uint8_t statusType = invalidStatus;
    // uint16_t programId = 0;
    // uint16_t queueId = 0;
    // uint8_t exitCode = 0;

    // std::array<uint8_t, 1> sendHeader = {getStatus};

    // // Send the Get Status command
    // EduErrorCode sendDataError = SendData(sendHeader);
    // if(sendDataError != EduErrorCode::success)
    // {
    //     return {EduStatusType::invalid, programId, queueId, exitCode, sendDataError};
    // }

    // // Start error while loop
    // bool succesfulRecv = false;
    // size_t errorCnt = 0;
    // EduStatusType statusTypeRet;
    // while(!succesfulRecv)
    // {
    //     // Receive the header (data command and length)
    //     std::array<uint8_t, cmdBytes + lenBytes> recvHeader = {};
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
    //         return {EduStatusType::invalid, programId, queueId, exitCode, headerError};
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
    //         return {EduStatusType::invalid,
    //                 programId,
    //                 queueId,
    //                 exitCode,
    //                 EduErrorCode::errorInvalidResult};
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
    //         return {EduStatusType::invalid,
    //                 programId,
    //                 queueId,
    //                 exitCode,
    //                 EduErrorCode::errorRecvDataTooLong};
    //     }

    //     // Receive actual status data
    //     // For data, reserve the max. possible status bytes
    //     std::array<uint8_t, maxStatusBytes> recvDataBuf = {};
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
    //         return {EduStatusType::invalid, programId, queueId, exitCode, recvDataError};
    //     }

    //     // Receive checksum
    //     std::array<uint8_t, 4> crc32Buf = {};
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
    //         return {EduStatusType::invalid, programId, queueId, exitCode, crc32Error};
    //     }

    //     // Assemble checksum
    //     auto crc32Recv =
    //         utility::BytesTouint32(crc32Buf[0], crc32Buf[1], crc32Buf[2], crc32Buf[3]);
    //     auto crc32Calc = utility::Crc32(recvDataBuf);

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
    //         return {
    //             EduStatusType::invalid, programId, queueId, exitCode,
    //             EduErrorCode::errorChecksum};
    //     }

    //     // If checksum is good, get proper values from the data byte array
    //     statusType = recvDataBuf[0];

    //     // TODO: (Patrick) disable hicpp-signed-bitwise? According to the links below the
    //     // implementation is bad. This would eliminate some weird instances of static_cast
    //     // when using bit operations.
    //     //
    // https
    //     :  //
    //     stackoverflow.com/questions/50399090/use-of-a-signed-integer-operand-with-a-binary-bitwise-operator-when-using-un
    //     // https://bugs.llvm.org/show_bug.cgi?id=36961#c9
    //     switch(statusType)
    //     {
    //         case noEventCode:
    //             statusTypeRet = EduStatusType::noEvent;
    //             succesfulRecv = true;
    //             break;

    //         case programFinishedCode:
    //             programId = utility::BytesTouint16(recvDataBuf[1], recvDataBuf[2]);
    //             queueId = utility::BytesTouint16(recvDataBuf[3], recvDataBuf[4]);
    //             exitCode = *(recvDataBuf.end() - 1);
    //             statusTypeRet = EduStatusType::programFinished;
    //             succesfulRecv = true;
    //             break;

    //         case resultsReadyCode:
    //             programId = utility::BytesTouint16(recvDataBuf[1], recvDataBuf[2]);
    //             queueId = utility::BytesTouint16(recvDataBuf[3], recvDataBuf[4]);
    //             statusTypeRet = EduStatusType::resultsReady;
    //             succesfulRecv = true;
    //             break;

    //         default:
    //             // Invalid status type, flush, send NACK, and retry
    //             FlushUartBuffer();
    //             if(errorCnt++ < maxNackRetries)
    //             {
    //                 SendCommand(cmdNack);
    //                 continue;
    //             }
    //             return {EduStatusType::invalid,
    //                     programId,
    //                     queueId,
    //                     exitCode,
    //                     EduErrorCode::errorInvalidResult};
    //             break;
    //     }
    // }
    // SendCommand(cmdAck);
    // return {statusTypeRet, programId, queueId, exitCode, EduErrorCode::success};

    return EduStatus{};
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
    //     std::array<uint8_t, 1> header = {returnResult};
    //     auto headerErrorCode = SendData(header);
    //     if(headerErrorCode != EduErrorCode::success)
    //     {
    //         return {headerErrorCode, 0};
    //     }
    // }

    // // Afterwards, only process a single data packet at a time, since we
    // // can't guarantee that it will use up all the memory

    // // Start error while loop
    // size_t errorCnt = 0;
    // EduStatusType statusTypeRet;
    // bool succesfulRecv = false;
    // while(!succesfulRecv)
    // {
    //     // Receive the header (data command and length)
    //     std::array<uint8_t, cmdBytes + lenBytes> recvHeader = {};
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
    //     std::array<uint8_t, maxDataLen> recvDataBuf = {};
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
    //     std::array<uint8_t, 4> crc32Buf = {};
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
    mEduUart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

    auto nReadBytes = mEduUart_.read(&answer, 1);
    if(nReadBytes == 0)
    {
        return EduErrorCode::errorTimeout;
    }
    switch(answer)
    {
        case cmdAck:
        {
            return EduErrorCode::success;
        }
        case cmdNack:
        {
            return EduErrorCode::errorNack;
        }
        default:
        {
            return EduErrorCode::errorInvalidResult;
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
    hal::WriteTo(&mEduUart_, std::span(data));
}


//! @brief Send a data packet over UART to the EDU.
//!
//! @param data The data to be sent
[[nodiscard]] auto Edu::SendData(std::span<Byte> data) -> EduErrorCode
{
    size_t nBytes = data.size();
    if(nBytes >= maxDataLength)
    {
        return EduErrorCode::errorSendDataTooLong;
    }

    // Casting size_t to uint16_t is safe since nBytes is checked against maxDataLength
    std::array<uint16_t, 1> len{static_cast<uint16_t>(nBytes)};
    std::array<uint32_t, 1> crc{utility::Crc32(data)};

    int nackCount = 0;
    while(nackCount < maxNackRetries)
    {
        SendCommand(cmdData);
        hal::WriteTo(&mEduUart_, std::span<uint16_t>(len));
        hal::WriteTo(&mEduUart_, data);
        hal::WriteTo(&mEduUart_, std::span<uint32_t>(crc));

        // TODO: Refactor this common pattern into a function
        // Data is always answered by N/ACK
        auto answer = 0x00_b;
        mEduUart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);

        auto nReadBytes = mEduUart_.read(&answer, 1);
        if(nReadBytes == 0)
        {
            return EduErrorCode::errorTimeout;
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
                return EduErrorCode::errorInvalidResult;
            }
        }
    }
    return EduErrorCode::errorNackRetries;
}


//! @brief Receive nBytes bytes over the EDU UART in a single round.
//!
//! @param dest The destination container
//!
//! @returns A relevant EDU error code
[[nodiscard]] auto Edu::UartReceive(std::span<Byte> destination) -> EduErrorCode
{
    if(size(destination) > maxDataLength)
    {
        return EduErrorCode::errorReceiveDataTooLong;
    }

    std::size_t totalReceivedBytes = 0U;
    const auto destinationSize = size(destination);
    while(totalReceivedBytes < destinationSize)
    {
        mEduUart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
        auto nReceivedBytes = mEduUart_.read(data(destination) + totalReceivedBytes,
                                             destinationSize - totalReceivedBytes);
        if(nReceivedBytes == 0)
        {
            return EduErrorCode::errorTimeout;
        }
        totalReceivedBytes += nReceivedBytes;
    }
    return EduErrorCode::success;
}


//! @brief Flush the EDU UART read buffer.
//!
//! This can be used to clear all buffer data after an error to request a resend.
auto Edu::FlushUartBuffer() -> void
{
    // std::array<uint8_t, garbageBufSize> garbageBuf = {0};
    // bool dataRecvd = true;

    // // Keep reading until no data is coming for flushTimeout (10 ms)
    // while(dataRecvd)
    // {
    //     mEduUart_.suspendUntilDataReady(RODOS::NOW() + flushTimeout);
    //     auto readBytes = mEduUart_.read(garbageBuf.data(), garbageBufSize);
    //     if(readBytes == 0)
    //     {
    //         dataRecvd = false;
    //     }
    // }
}
}
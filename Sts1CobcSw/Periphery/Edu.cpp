#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/UtilityNames.hpp>

#include <array>


namespace sts1cobcsw::periphery
{


using sts1cobcsw::serial::Byte;


Edu::Edu()
{
    // EDU PDD agrees with the default baudrate (115200 bit/s)
    mEduUart_.init();
}


void Edu::FlushUartBuffer()
{
    std::array<uint8_t, garbageBufSize> garbageBuf = {0};
    bool dataRecvd = true;

    // Keep reading until no data is coming for flushTimeout (10 ms)
    while(dataRecvd)
    {
        mEduUart_.suspendUntilDataReady(RODOS::NOW() + flushTimeout);
        auto readBytes = mEduUart_.read(garbageBuf.data(), garbageBufSize);
        if(readBytes == 0)
        {
            dataRecvd = false;
        }
    }
}


auto Edu::UartReceive(std::span<uint8_t> dest, std::size_t nBytes) -> EduErrorCode
{
    if(nBytes > maxDataLen)
    {
        return EduErrorCode::errorRecvDataTooLong;
    }
    if(dest.size() < nBytes)
    {
        return EduErrorCode::errorBufferTooSmall;
    }

    uint16_t readBytesTotal = 0;

    while(readBytesTotal < nBytes)
    {
        mEduUart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
        auto it = dest.begin() + uint16_t(readBytesTotal);
        auto readBytes = mEduUart_.read(&(*it), nBytes - readBytesTotal);
        if(readBytes == 0)
        {
            return EduErrorCode::errorTimeout;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
        // It's probably impossible to avoid a warning here at some point.
        // Since readBytes <= readBytesTotal <= nBytes <= maxDataLen = 32768,
        // as well as readByters + readBytesTotal <= maxDataLen = 32768,
        // it's safe to disable this warning (no loss, no change of sign or anything possible).
        readBytesTotal += readBytes;
#pragma GCC diagnostic pop
    }
    return EduErrorCode::success;
}


void Edu::SendCommand(uint8_t cmd)
{
    std::array<uint8_t, 1> cmdArr{cmd};
    // TODO: ambiguity when using arrays directly with Write operations (Communication.hpp)
    hal::WriteTo(&mEduUart_, std::span<uint8_t>(cmdArr));
}


auto Edu::SendData(std::span<Byte> data) -> EduErrorCode
{
    size_t nBytes = data.size();
    if(nBytes >= maxDataLen)
    {
        return EduErrorCode::errorSendDataTooLong;
    }

    // Casting size_t to uint16_t is safe since nBytes is checked against maxDataLen
    std::array<uint16_t, 1> len{static_cast<uint16_t>(nBytes)};
    std::array<uint32_t, 1> crc{utility::Crc32(data)};

    size_t nackCnt = 0;
    while(nackCnt++ < maxNackRetries)
    {
        SendCommand(cmdData);
        hal::WriteTo(&mEduUart_, std::span<uint16_t>(len));
        hal::WriteTo(&mEduUart_, data);
        hal::WriteTo(&mEduUart_, std::span<uint32_t>(crc));

        // Data is always answered by N/ACK
        uint8_t recvAck = 0U;
        mEduUart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
        mEduUart_.read(&recvAck, 1);

        switch(recvAck)
        {
            case cmdAck:
                return EduErrorCode::success;
                break;

            case cmdNack:
                // If NACK was received, retry
                if(nackCnt < maxNackRetries)
                {
                    continue;
                }
                break;

            case 0:
                return EduErrorCode::errorDataTimeout;
                break;

            default:
                return EduErrorCode::errorDataInvalidResult;
                break;
        }
    }
    return EduErrorCode::errorNackRetries;
}


auto Edu::ExecuteProgram(uint16_t programId, uint16_t queueId, uint16_t timeout) -> EduErrorCode
{
    ExecuteProgramArguments arguments{
        .header = executeProgram, .programId = programId, .queueId = queueId, .timeout = timeout};

    std::array<Byte, sts1cobcsw::serial::serialSize<ExecuteProgramArguments>> data = {};

    // ts::uint8_t header = executeProgram;
    // auto args = std::array<ts::uint16_t, 3>{programId, queueId, timeout};
    // std::array<ts::uint8_t, args.size() * 2> argsts::uint8 = {};
    // utility::Arrayts::uint16Tots::uint8(args, argsts::uint8);

    // std::array<ts::uint8_t, argsts::uint8.size() + 1> dataBuf = {};

    // Create a data packet with the header and the arguments
    // dataBuf[0] = header;
    // for(size_t i = 1; i < dataBuf.size(); i++)
    //{
    //    dataBuf[i] = argsts::uint8[i - 1];
    //}

    // Check if data command was successful
    EduErrorCode dataError = SendData(data);
    if(dataError != EduErrorCode::success)
    {
        return dataError;
    }

    // Receive N/ACK
    uint8_t recvAck = 0;

    // eduTimeout != timeout argument for data!
    // timeout specifies the time the student program has to execute
    // eduTimeout is the max. allowed time to reveice N/ACK from EDU
    mEduUart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
    mEduUart_.read(&recvAck, 1);

    switch(recvAck)
    {
        case cmdAck:
            return EduErrorCode::success;
            break;

        case cmdNack:
            return EduErrorCode::errorNack;
            break;

        case 0:
            return EduErrorCode::errorTimeout;
            break;

        default:
            return EduErrorCode::errorInvalidResult;
            break;
    }
}

/*
// TODO(Daniel): refactor, too complex
auto Edu::GetStatus() -> EduStatus
{
    // Values to be returned
    ts::uint8_t statusType = invalidStatus;
    ts::uint16_t programId = 0;
    ts::uint16_t queueId = 0;
    ts::uint8_t exitCode = 0;

    std::array<ts::uint8_t, 1> sendHeader = {getStatus};

    // Send the Get Status command
    EduErrorCode sendDataError = SendData(sendHeader);
    if(sendDataError != EduErrorCode::success)
    {
        return {EduStatusType::invalid, programId, queueId, exitCode, sendDataError};
    }

    // Start error while loop
    bool succesfulRecv = false;
    size_t errorCnt = 0;
    EduStatusType statusTypeRet;
    while(!succesfulRecv)
    {
        // Receive the header (data command and length)
        std::array<ts::uint8_t, cmdBytes + lenBytes> recvHeader = {};
        auto headerError = UartReceive(recvHeader, cmdBytes + lenBytes);
        if(headerError != EduErrorCode::success)
        {
            // Only retry on timeout errors (-> invalid format after all)
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries and headerError == EduErrorCode::errorTimeout)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {EduStatusType::invalid, programId, queueId, exitCode, headerError};
        }
        if(recvHeader[0] != cmdData)
        {
            // Invalid header, flush, send NACK, and retry
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {EduStatusType::invalid,
                    programId,
                    queueId,
                    exitCode,
                    EduErrorCode::errorInvalidResult};
        }

        // Create 2 byte length from single received bytes
        auto const len = utility::BytesTots::uint16(recvHeader[1], recvHeader[2]);
        if(len > maxDataLen)
        {
            // Invalid length, flush, send NACK, and retry
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {EduStatusType::invalid,
                    programId,
                    queueId,
                    exitCode,
                    EduErrorCode::errorRecvDataTooLong};
        }

        // Receive actual status data
        // For data, reserve the max. possible status bytes
        std::array<ts::uint8_t, maxStatusBytes> recvDataBuf = {};
        auto recvDataError = UartReceive(recvDataBuf, len);
        if(recvDataError != EduErrorCode::success)
        {
            // Only retry on timeout errors (-> invalid format after all)
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries and recvDataError == EduErrorCode::errorTimeout)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {EduStatusType::invalid, programId, queueId, exitCode, recvDataError};
        }

        // Receive checksum
        std::array<ts::uint8_t, 4> crc32Buf = {};
        auto crc32Error = UartReceive(crc32Buf, crc32Buf.size());
        if(crc32Error != EduErrorCode::success)
        {
            // Only retry on timeout errors (-> invalid format after all)
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries and crc32Error == EduErrorCode::errorTimeout)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {EduStatusType::invalid, programId, queueId, exitCode, crc32Error};
        }

        // Assemble checksum
        auto crc32Recv = utility::BytesTots::uint32(crc32Buf[0], crc32Buf[1], crc32Buf[2],
crc32Buf[3]); auto crc32Calc = utility::Crc32(recvDataBuf);

        // Check checksum against own calculation
        if(crc32Recv != crc32Calc)
        {
            // Checksums don't match, flush, send NACK, and retry
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {
                EduStatusType::invalid, programId, queueId, exitCode, EduErrorCode::errorChecksum};
        }

        // If checksum is good, get proper values from the data byte array
        statusType = recvDataBuf[0];

        // TODO: (Patrick) disable hicpp-signed-bitwise? According to the links below the
        // implementation is bad. This would eliminate some weird instances of static_cast
        // when using bit operations.
        //
https://stackoverflow.com/questions/50399090/use-of-a-signed-integer-operand-with-a-binary-bitwise-operator-when-using-un
        // https://bugs.llvm.org/show_bug.cgi?id=36961#c9
        switch(statusType)
        {
            case noEventCode:
                statusTypeRet = EduStatusType::noEvent;
                succesfulRecv = true;
                break;

            case programFinishedCode:
                programId = utility::BytesTots::uint16(recvDataBuf[1], recvDataBuf[2]);
                queueId = utility::BytesTots::uint16(recvDataBuf[3], recvDataBuf[4]);
                exitCode = *(recvDataBuf.end() - 1);
                statusTypeRet = EduStatusType::programFinished;
                succesfulRecv = true;
                break;

            case resultsReadyCode:
                programId = utility::BytesTots::uint16(recvDataBuf[1], recvDataBuf[2]);
                queueId = utility::BytesTots::uint16(recvDataBuf[3], recvDataBuf[4]);
                statusTypeRet = EduStatusType::resultsReady;
                succesfulRecv = true;
                break;

            default:
                // Invalid status type, flush, send NACK, and retry
                FlushUartBuffer();
                if(errorCnt++ < maxNackRetries)
                {
                    SendCommand(cmdNack);
                    continue;
                }
                return {EduStatusType::invalid,
                        programId,
                        queueId,
                        exitCode,
                        EduErrorCode::errorInvalidResult};
                break;
        }
    }
    SendCommand(cmdAck);
    return {statusTypeRet, programId, queueId, exitCode, EduErrorCode::success};
}


auto Edu::UpdateTime(ts::uint32_t timestamp) -> EduErrorCode
{
    std::array<ts::uint32_t, 1> tsts::uint32 = {timestamp};
    std::array<ts::uint8_t, 4> tsts::uint8 = {};
    utility::Arrayts::uint32Tots::uint8(tsts::uint32, tsts::uint8);

    EduErrorCode errorCode;
    for(size_t errorCnt = 0; errorCnt < maxNackRetries; errorCnt++)
    {
        {
            // Send data and check for success, otherwise flush the UART and retry
            errorCode = SendData(tsts::uint8);
            if(errorCode == EduErrorCode::success)
            {
                break;
            }
            FlushUartBuffer();
        }
    }

    // If the error code does not show success, return the error
    if(errorCode != EduErrorCode::success)
    {
        return errorCode;
    }

    // On success, wait for second N/ACK
    // TODO(Daniel): change to UartReceive()
    mEduUart_.suspendUntilDataReady(RODOS::NOW() + eduTimeout);
    ts::uint8_t nackBuf = 0;
    auto retVal = mEduUart_.read(&nackBuf, 1);

    if(retVal == 0)
    {
        return EduErrorCode::errorTimeout;
    }
    if(nackBuf == cmdNack)
    {
        return EduErrorCode::errorNack;
    }
    if(nackBuf != cmdAck)
    {
        return EduErrorCode::errorInvalidResult;
    }

    return EduErrorCode::success;
}


auto Edu::StopProgram() -> EduErrorCode
{
    return EduErrorCode::success;
    std::array<ts::uint8_t, 3> dataBuf = {stopProgram};
    auto errorCode = SendData(dataBuf);

    if(errorCode != EduErrorCode::success)
    {
        return errorCode;
    }

    // Receive second N/ACK to see if program is successfully stopped
    std::array<ts::uint8_t, 1> recvBuf = {};
    return UartReceive(recvBuf, 1);
}


auto Edu::ReturnResult(std::array<ts::uint8_t, maxDataLen> & dest) -> ResultInfo
{
    // If this is the initial call, send the header
    if(!mResultPending_)
    {
        std::array<ts::uint8_t, 1> header = {returnResult};
        auto headerErrorCode = SendData(header);
        if(headerErrorCode != EduErrorCode::success)
        {
            return {headerErrorCode, 0};
        }
    }

    // Afterwards, only process a single data packet at a time, since we
    // can't guarantee that it will use up all the memory

    // Start error while loop
    size_t errorCnt = 0;
    EduStatusType statusTypeRet;
    bool succesfulRecv = false;
    while(!succesfulRecv)
    {
        // Receive the header (data command and length)
        std::array<ts::uint8_t, cmdBytes + lenBytes> recvHeader = {};
        auto headerError = UartReceive(recvHeader, cmdBytes + lenBytes);
        if(headerError != EduErrorCode::success)
        {
            // Only retry on timeout errors (-> invalid format after all)
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries and headerError == EduErrorCode::errorTimeout)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {headerError, 0};
        }
        if(recvHeader[0] == cmdEof)
        {
            return {EduErrorCode::successEof, 0};
        }
        if(recvHeader[0] != cmdData)
        {
            // Invalid header, flush, send NACK, and retry
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {EduErrorCode::errorInvalidResult, 0};
        }

        // Create 2 byte length from single received bytes
        auto const len = utility::BytesTots::uint16(recvHeader[1], recvHeader[2]);
        if(len > maxDataLen)
        {
            // Invalid length, flush, send NACK, and retry
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {EduErrorCode::errorRecvDataTooLong, 0};
        }

        // Receive actual status data
        // For data, reserve the max. possible bytes
        std::array<ts::uint8_t, maxDataLen> recvDataBuf = {};
        auto recvDataError = UartReceive(recvDataBuf, len);
        if(recvDataError != EduErrorCode::success)
        {
            // Only retry on timeout errors (-> invalid format after all)
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries and recvDataError == EduErrorCode::errorTimeout)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {recvDataError, 0};
        }

        // Receive checksum
        std::array<ts::uint8_t, 4> crc32Buf = {};
        auto crc32Error = UartReceive(crc32Buf, crc32Buf.size());
        if(crc32Error != EduErrorCode::success)
        {
            // Only retry on timeout errors (-> invalid format after all)
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries and crc32Error == EduErrorCode::errorTimeout)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {crc32Error, 0};
        }

        // Assemble checksum
        auto crc32Recv = utility::BytesTots::uint32(crc32Buf[0], crc32Buf[1], crc32Buf[2],
crc32Buf[3]); auto crc32Calc = utility::Crc32(recvDataBuf);

        // Check checksum against own calculation
        if(crc32Recv != crc32Calc)
        {
            // Checksums don't match, flush, send NACK, and retry
            FlushUartBuffer();
            if(errorCnt++ < maxNackRetries)
            {
                SendCommand(cmdNack);
                continue;
            }
            return {EduErrorCode::errorChecksum, 0};
        }

        // Everything worked, so return success and the number of received bytes
        return {EduErrorCode::success, len};

    }  // End while
}*/
}
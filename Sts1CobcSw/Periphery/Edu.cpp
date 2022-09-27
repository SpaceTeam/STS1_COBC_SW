#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Util/Util.hpp>
#include <Sts1CobcSw/Util/UtilNames.hpp>

#include <array>
#include <iostream>
#include <vector>

namespace sts1cobcsw::periphery
{

EduUartInterface::EduUartInterface()
{
    // EDU PDD agrees with the default baudrate (115200 bit/s)
    mEduUart_.init();
}

void EduUartInterface::FlushUartBuffer()
{
    std::array<uint8_t, garbageBufSize> garbageBuf = {0};
    bool dataRecvd = true;

    // Keep reading until no data is coming for flushTimeout (10 ms)
    while(dataRecvd)
    {
        mEduUart_.suspendUntilDataReady(NOW() + flushTimeout);
        auto readBytes = mEduUart_.read(garbageBuf.data(), garbageBufSize);
        if(readBytes == 0)
        {
            dataRecvd = false;
        }
    }
}

auto EduUartInterface::UartReceive(std::vector<uint8_t> & recvVec, size_t nBytes) -> EduErrorCode
{
    if(nBytes > maxDataLen)
    {
        return EduErrorCode::errorRecvDataTooLong;
    }
    if(recvVec.size() < nBytes)
    {
        return EduErrorCode::errorBufferTooSmall;
    }

    uint16_t readBytesTotal = 0;

    while(readBytesTotal < nBytes)
    {
        mEduUart_.suspendUntilDataReady(NOW() + eduTimeout);
        auto it = recvVec.begin() + readBytesTotal;
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

void EduUartInterface::SendCommand(uint8_t cmd)
{
    std::array<uint8_t, 1> cmdArr{cmd};
    hal::WriteTo(&mEduUart_, std::span<uint8_t>(cmdArr));
}

auto EduUartInterface::SendData(std::span<uint8_t> data) -> EduErrorCode
{
    size_t nBytes = data.size();
    if(nBytes >= maxDataLen)
    {
        return EduErrorCode::errorSendDataTooLong;
    }

    // Casting size_t to uint16_t is safe since nBytes is checked against maxDataLen
    std::array<uint16_t, 1> len{static_cast<uint16_t>(nBytes)};
    std::array<uint32_t, 1> crc{util::Crc32(data)};

    size_t nackCnt = 0;
    while(nackCnt++ < maxNackRetries)
    {
        SendCommand(cmdData);
        hal::WriteTo(&mEduUart_, std::span<uint16_t>(len));
        hal::WriteTo(&mEduUart_, data);
        hal::WriteTo(&mEduUart_, std::span<uint32_t>(crc));

        std::cout << "Send data:\n";
        std::cout << "Len: " << std::hex << len[0] << "\n";
        for(size_t i = 0; i < nBytes; i++)
        {
            std::cout << "Data[" << i << "]:" << std::hex << static_cast<int>(data[i]) << "\n";
        }
        std::cout << "CRC32: " << std::hex << crc[0] << "\n";

        // Data is always answered by N/ACK
        uint8_t recvAck = 0;
        mEduUart_.suspendUntilDataReady(NOW() + eduTimeout);
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

auto EduUartInterface::ExecuteProgram(uint16_t programId, uint16_t queueId, uint16_t timeout)
    -> EduErrorCode
{
    uint8_t header = executeProgram;
    auto args = std::vector<uint16_t>{programId, queueId, timeout};
    auto argsUint8 = util::VecUint16ToUint8(std::span<uint16_t>(args));

    std::vector<uint8_t> dataBuf;

    // Create a data packet with the header and the arguments
    dataBuf.reserve(argsUint8.size() + 1);
    dataBuf.push_back(header);
    dataBuf.insert(dataBuf.end(), argsUint8.begin(), argsUint8.end());

    // Check if data command was successful
    EduErrorCode dataError = SendData(dataBuf);
    if(dataError != EduErrorCode::success)
    {
        return dataError;
    }

    // Receive N/ACK
    uint8_t recvAck = 0;

    // eduTimeout != timeout argument for data!
    // timeout specifies the time the student program has to execute
    // eduTimeout is the max. allowed time to reveice N/ACK from EDU
    mEduUart_.suspendUntilDataReady(NOW() + eduTimeout);
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

// TODO(Daniel): refactor, too complex
auto EduUartInterface::GetStatus()
    -> std::tuple<EduStatusType, uint16_t, uint16_t, uint8_t, EduErrorCode>
{
    // Values to be returned
    uint8_t statusType = invalidStatus;
    uint16_t programId = 0;
    uint16_t queueId = 0;
    uint8_t exitCode = 0;

    std::array<uint8_t, 1> sendHeader = {getStatus};
    std::span<uint8_t> sendDataBuf = sendHeader;

    // Send the Get Status command
    EduErrorCode sendDataError = SendData(sendDataBuf);
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
        std::vector<uint8_t> recvHeader(cmdBytes + lenBytes, 0);
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
        auto len = util::BytesToUint16(recvHeader[1], recvHeader[2]);
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
        std::vector<uint8_t> recvDataBuf(len, 0);
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
        std::vector<uint8_t> crc32Buf(sizeof(uint32_t), 0);
        auto crc32Error = UartReceive(crc32Buf, sizeof(uint32_t));
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
        auto crc32Recv = util::BytesToUint32(crc32Buf[0], crc32Buf[1], crc32Buf[2], crc32Buf[3]);
        auto crc32Calc = util::Crc32(recvDataBuf);

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

        // If checksum is good, get proper values from the data byte vector
        statusType = recvDataBuf[0];

        // TODO(Patrick): disable hicpp-signed-bitwise? According to the links below the
        // implementation is bad. This would eliminate all static_casts here (and some above).
        // https://stackoverflow.com/questions/50399090/use-of-a-signed-integer-operand-with-a-binary-bitwise-operator-when-using-un
        // https://bugs.llvm.org/show_bug.cgi?id=36961#c9
        switch(statusType)
        {
            case noEvent:
                statusTypeRet = EduStatusType::noEvent;
                break;

            case programFinished:
                programId = util::BytesToUint16(recvDataBuf[1], recvDataBuf[2]);
                queueId = util::BytesToUint16(recvDataBuf[3], recvDataBuf[4]);
                exitCode = *(recvDataBuf.end() - 1);
                statusTypeRet = EduStatusType::programFinished;
                break;

            case resultsReady:
                programId = util::BytesToUint16(recvDataBuf[1], recvDataBuf[2]);
                queueId = util::BytesToUint16(recvDataBuf[3], recvDataBuf[4]);
                statusTypeRet = EduStatusType::resultsReady;
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
}
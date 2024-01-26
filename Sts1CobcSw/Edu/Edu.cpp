#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

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

// The max. data length is 11 KiB. At 115'200 baud, this takes about 1 second to transmit. We use
// 1.5 s just to be sure.
constexpr auto sendTimeout = 1500 * RODOS::MILLISECONDS;
constexpr auto receiveTimeout = 1500 * RODOS::MILLISECONDS;
// TODO: Can we choose a smaller value?
constexpr auto flushReceiveBufferTimeout = 1 * RODOS::MILLISECONDS;

// TODO: Choose proper values
// Max. number of send retries after receiving NACK
constexpr auto maxNNackRetries = 10;
// Max. number of data packets for a single command
constexpr auto maxNPackets = 100;
// Max. length of a single data packet
constexpr auto maxDataLength = 11 * 1024;
// Data buffer for potentially large data packets (ReturnResult and StoreProgram)
auto cepDataBuffer = etl::vector<Byte, maxDataLength>{};

auto eduEnableGpioPin = hal::GpioPin(hal::eduEnablePin);
auto uart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);


// --- Private function declarations ---

[[nodiscard]] auto ReceiveAndParseStatusData() -> Result<Status>;
[[nodiscard]] auto ParseStatusData() -> Result<Status>;

[[nodiscard]] auto SendDataPacket(std::span<Byte const> data) -> Result<void>;
// TODO: Rework -> Send(CepCommand command) -> void;
[[nodiscard]] auto SendCommand(Byte commandId) -> Result<void>;
[[nodiscard]] auto Send(std::span<Byte const> data) -> Result<void>;

[[nodiscard]] auto ReceiveDataPacket() -> Result<void>;
template<typename T>
[[nodiscard]] auto Receive() -> Result<T>;
template<>
[[nodiscard]] auto Receive<Byte>() -> Result<Byte>;
[[nodiscard]] auto Receive(std::span<Byte> data) -> Result<void>;
[[nodiscard]] auto ReceiveAndCheckCrc32(std::span<Byte const> data) -> Result<void>;

template<typename T>
[[nodiscard]] auto Retry(auto(*communicationFunction)()->Result<T>, int nTries) -> Result<T>;
auto FlushUartReceiveBuffer() -> void;


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
    OUTCOME_TRY(SendDataPacket(Serialize(data)));
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
    OUTCOME_TRY(SendDataPacket(Serialize(GetStatusData())));
    OUTCOME_TRY(auto status, Retry(ReceiveAndParseStatusData, maxNNackRetries));
    OUTCOME_TRY(SendCommand(cepAck));
    return status;
}


auto ReceiveAndParseStatusData() -> Result<Status>
{
    OUTCOME_TRY(ReceiveDataPacket());
    return ParseStatusData();
}


auto ParseStatusData() -> Result<Status>
{
    if(cepDataBuffer.empty())
    {
        return ErrorCode::invalidLength;
    }
    auto statusId = cepDataBuffer[0];
    if(statusId == NoEventData::id)
    {
        if(cepDataBuffer.size() != serialSize<NoEventData>)
        {
            return ErrorCode::invalidLength;
        }
        return Status{.statusType = StatusType::noEvent};
    }
    if(statusId == ProgramFinishedData::id)
    {
        if(cepDataBuffer.size() != serialSize<ProgramFinishedData>)
        {
            return ErrorCode::invalidLength;
        }
        auto programFinishedData = Deserialize<ProgramFinishedData>(
            Span(cepDataBuffer).first<serialSize<ProgramFinishedData>>());
        return Status{.statusType = StatusType::programFinished,
                      .programId = programFinishedData.programId,
                      .startTime = programFinishedData.startTime,
                      .exitCode = programFinishedData.exitCode};
    }
    if(statusId == ResultsReadyData::id)
    {
        if(cepDataBuffer.size() != serialSize<ResultsReadyData>)
        {
            return ErrorCode::invalidLength;
        }
        auto resultsReadyData = Deserialize<ResultsReadyData>(
            Span(cepDataBuffer).first<serialSize<ResultsReadyData>>());
        return Status{.statusType = StatusType::resultsReady,
                      .programId = resultsReadyData.programId,
                      .startTime = resultsReadyData.startTime};
    }
    return ErrorCode::invalidStatusType;
}


auto ReturnResult(ReturnResultData const & data) -> Result<void>
{
    OUTCOME_TRY(SendDataPacket(Serialize(data)));
    for(auto nPackets = 0; nPackets < maxNPackets; ++nPackets)
    {
        OUTCOME_TRY(Retry(ReceiveDataPacket, maxNNackRetries));
        // If the buffer is empty we received an EOF and are done with the transmission
        if(cepDataBuffer.empty())
        {
            // TODO: Do we also have a CRC32 over the whole file that needs to be checked here?
            OUTCOME_TRY(SendCommand(cepAck));
            return outcome_v2::success();
        }
        // TODO: Actually store the result in the COBC file system
        RODOS::PRINTF("Pretending to write %d bytes to the file system ...\n",
                      static_cast<int>(cepDataBuffer.size()));
        RODOS::AT(RODOS::NOW() + 3 * RODOS::MILLISECONDS);
        OUTCOME_TRY(SendCommand(cepAck));
    }
    return ErrorCode::tooManyDataPackets;
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
    OUTCOME_TRY(SendDataPacket(Serialize(data)));
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


//! @brief Send a data packet over UART to the EDU.
//!
//! @param data The data to be sent
auto SendDataPacket(std::span<Byte const> data) -> Result<void>
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
        OUTCOME_TRY(SendCommand(cepData));
        OUTCOME_TRY(Send(Span(length)));
        OUTCOME_TRY(Send(data));
        OUTCOME_TRY(Send(Span(checksum)));

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


auto SendCommand(Byte commandId) -> Result<void>
{
    OUTCOME_TRY(Send(Span(commandId)));
    return outcome_v2::success();
}


// This function is mainly here for converting error codes and ensuring that all send operations use
// a timeout.
auto Send(std::span<Byte const> data) -> Result<void>
{
    auto writeToResult = hal::WriteTo(&uart, data, sendTimeout);
    if(writeToResult.has_error())
    {
        return ErrorCode::timeout;
    }
    return outcome_v2::success();
}


auto ReceiveDataPacket() -> Result<void>
{
    OUTCOME_TRY(auto answer, Receive<Byte>());
    if(answer == cepEof)
    {
        cepDataBuffer.clear();
        return outcome_v2::success();
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
    cepDataBuffer.resize(dataLength);

    OUTCOME_TRY(Receive(Span(&cepDataBuffer)));
    OUTCOME_TRY(ReceiveAndCheckCrc32(Span(cepDataBuffer)));
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


auto Receive(std::span<Byte> data) -> Result<void>
{
    if(data.size() > maxDataLength)
    {
        return ErrorCode::receiveDataTooLong;
    }
    auto readFromResult = hal::ReadFrom(&uart, data, receiveTimeout);
    if(readFromResult.has_error())
    {
        return ErrorCode::timeout;
    }
    return outcome_v2::success();
}


// TODO: A parameter pack of spans would be very convenient
auto ReceiveAndCheckCrc32(std::span<Byte const> data) -> Result<void>
{
    auto computedCrc32 = utility::ComputeCrc32(data);
    OUTCOME_TRY(auto receivedCrc32, Receive<std::uint32_t>());
    if(computedCrc32 != receivedCrc32)
    {
        return ErrorCode::wrongChecksum;
    }
    return outcome_v2::success();
}


template<typename T>
auto Retry(auto(*communicationFunction)()->Result<T>, int nTries) -> Result<T>
{
    auto iTries = 0;
    while(true)
    {
        auto result = communicationFunction();
        if(result.has_value())
        {
            // No ACK is sent here. The caller is responsible for that.
            return result;
        }
        FlushUartReceiveBuffer();
        OUTCOME_TRY(SendCommand(cepNack));
        iTries++;
        if(iTries > nTries)
        {
            // TODO: Maybe return tooManyNacks here instead?
            return result.error();
        }
    }
}


//! @brief Flush the EDU UART read buffer.
//!
//! This can be used to clear all buffer data after an error to request a resend.
auto FlushUartReceiveBuffer() -> void
{
    auto garbageBuffer = std::array<Byte, 32>{};  // NOLINT(*magic-numbers)
    while(true)
    {
        auto readFromResult = hal::ReadFrom(&uart, Span(&garbageBuffer), flushReceiveBufferTimeout);
        if(readFromResult.has_error())
        {
            break;
        }
    }
}
}

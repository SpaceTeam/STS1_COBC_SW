#include <Sts1CobcSw/Edu/Edu.hpp>

#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/File.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <littlefs/lfs.h>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/format_spec.h>
#include <etl/to_string.h>
#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>


namespace sts1cobcsw::edu
{
// --- Public globals ---

hal::GpioPin updateGpioPin(hal::eduUpdatePin);
hal::GpioPin dosiEnableGpioPin(hal::dosiEnablePin);


namespace
{
// --- Private globals ---

auto const programsDirectory = fs::Path("programs/");
auto const resultsDirectory = fs::Path("results/");

// Low-level CEP commands
constexpr auto cepAck = 0xd7_b;   //! Acknowledging a data packet
constexpr auto cepNack = 0x27_b;  //! Not Acknowledging an (invalid) data packet
constexpr auto cepEof = 0x59_b;   //! Transmission of multiple packets is complete
constexpr auto cepData = 0x8b_b;  //! Data packet format is used (not a command packet!)

// The max. data length is 11 KiB. At 115'200 baud, this takes about 1 second to transmit. We use
// 1.5 s just to be sure.
constexpr auto sendTimeout = 1500 * ms;
constexpr auto receiveTimeout = 1500 * ms;
// TODO: Can we choose a smaller value?
constexpr auto flushReceiveBufferTimeout = 1 * ms;

// TODO: Choose proper values
// Max. number of send retries after receiving NACK
constexpr auto maxNNackRetries = 4;
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

[[nodiscard]] auto WaitForAck() -> Result<void>;

[[nodiscard]] auto ReceiveDataPacket() -> Result<void>;
template<typename T>
[[nodiscard]] auto Receive() -> Result<T>;
template<>
[[nodiscard]] auto Receive<Byte>() -> Result<Byte>;
[[nodiscard]] auto Receive(std::span<Byte> data) -> Result<void>;
[[nodiscard]] auto ReceiveAndCheckCrc32(std::span<Byte const> data) -> Result<void>;

template<typename T>
[[nodiscard]] auto Retry(auto (*communicationFunction)()->Result<T>, int nTries) -> Result<T>;
auto FlushUartReceiveBuffer() -> void;
}


// --- Public function definitions ---

//! @brief  Must be called in an init() function of a thread.
auto Initialize() -> void
{
    eduEnableGpioPin.SetDirection(hal::PinDirection::out);
    persistentVariables.Load<"eduShouldBePowered">() ? TurnOn() : TurnOff();
}


auto TurnOn() -> void
{
    persistentVariables.Store<"eduShouldBePowered">(true);
    eduEnableGpioPin.Set();

    // TODO: Test how high we can set the baudrate without problems (bit errors, etc.)
    auto const baudRate = 115'200;
    hal::Initialize(&uart, baudRate);
}


auto TurnOff() -> void
{
    persistentVariables.Store<"eduShouldBePowered">(false);
    eduEnableGpioPin.Reset();
    hal::Deinitialize(&uart);
}


auto StoreProgram(StoreProgramData const & data) -> Result<void>
{
    auto path = programsDirectory;
    static constexpr auto width =
        std::numeric_limits<strong::underlying_type_t<ProgramId>>::digits10 + 1;
    etl::to_string(
        value_of(data.programId), path, etl::format_spec().width(width).fill('0'), /*append=*/true);
    // NOLINTNEXTLINE(*signed-bitwise)
    OUTCOME_TRY(auto file, fs::Open(path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC));
    // TODO: Check if program file is not too large
    while(true)
    {
        OUTCOME_TRY(file.Read(Span(&cepDataBuffer)));
        if(cepDataBuffer.empty())
        {
            break;
        }
        auto sendDataPacketResult = SendDataPacket(Span(cepDataBuffer));
        if(sendDataPacketResult.has_error())
        {
            // TODO: Why not return the error from SendDataPacket here, i.e., use OUTCOME_TRY?
            return ErrorCode::nack;
        }
    }
    return WaitForAck();
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
    return WaitForAck();
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
    OUTCOME_TRY(SendDataPacket(Serialize(StopProgramData())));
    return WaitForAck();
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


auto ReturnResult(ReturnResultData const & data) -> Result<void>
{
    OUTCOME_TRY(SendDataPacket(Serialize(data)));
    for(auto nPackets = 0; nPackets < maxNPackets; ++nPackets)
    {
        OUTCOME_TRY(Retry(ReceiveDataPacket, maxNNackRetries));
        // If the buffer is empty we received an EOF and are done with the transmission
        if(cepDataBuffer.empty())
        {
            OUTCOME_TRY(SendCommand(cepAck));
            // TODO: Not sure if we actually have a CRC32 over the whole file that needs to be
            // checked here, but we need to send two ACKs anyway
            DEBUG_PRINT("Pretending to check the whole file's CRC32 ...\n");
            OUTCOME_TRY(SendCommand(cepAck));
            return outcome_v2::success();
        }
        // TODO: Actually store the result in the COBC file system
        DEBUG_PRINT("Pretending to write %d bytes to the file system ...\n",
                    static_cast<int>(cepDataBuffer.size()));
        SuspendFor(3 * ms);
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
    return WaitForAck();
}


auto ProgramsAreAvailableOnCobc() -> bool
{
    auto makeIteratorResult = fs::MakeIterator(edu::programsDirectory);
    if(makeIteratorResult.has_error())
    {
        return false;
    }
    auto & directoryIterator = makeIteratorResult.value();
    return std::any_of(directoryIterator.begin(),
                       directoryIterator.end(),
                       [](auto const & entryResult)
                       {
                           return not entryResult.has_error()
                              and entryResult.value().type == fs::EntryType::file;
                       });
}


// --- Private function definitions ---

namespace
{
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
        if(cepDataBuffer.size() != totalSerialSize<NoEventData>)
        {
            return ErrorCode::invalidLength;
        }
        return Status{.statusType = StatusType::noEvent};
    }
    if(statusId == ProgramFinishedData::id)
    {
        if(cepDataBuffer.size() != totalSerialSize<ProgramFinishedData>)
        {
            return ErrorCode::invalidLength;
        }
        auto programFinishedData = Deserialize<ProgramFinishedData>(
            Span(cepDataBuffer).first<totalSerialSize<ProgramFinishedData>>());
        return Status{.statusType = StatusType::programFinished,
                      .programId = programFinishedData.programId,
                      .startTime = programFinishedData.startTime,
                      .exitCode = programFinishedData.exitCode};
    }
    if(statusId == ResultsReadyData::id)
    {
        if(cepDataBuffer.size() != totalSerialSize<ResultsReadyData>)
        {
            return ErrorCode::invalidLength;
        }
        auto resultsReadyData = Deserialize<ResultsReadyData>(
            Span(cepDataBuffer).first<totalSerialSize<ResultsReadyData>>());
        return Status{.statusType = StatusType::resultsReady,
                      .programId = resultsReadyData.programId,
                      .startTime = resultsReadyData.startTime};
    }
    return ErrorCode::invalidStatusType;
}


//! @brief Send a data packet over UART to the EDU.
//!
//! @param data The data to be sent
auto SendDataPacket(std::span<Byte const> data) -> Result<void>
{
    if(data.size() >= maxDataLength)
    {
        return ErrorCode::dataPacketTooLong;
    }
    // Casting the size to uint16_t is safe since it is checked against maxDataLength
    auto length = Serialize(static_cast<std::uint16_t>(data.size()));
    auto checksum = Serialize(ComputeCrc32(data));

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
                return ErrorCode::invalidAnswer;
            }
        }
    }
    return ErrorCode::tooManyNacks;
}


// TODO: This function became so simple that it might not be necessary anymore
auto SendCommand(Byte commandId) -> Result<void>
{
    return Send(Span(commandId));
}


auto Send(std::span<Byte const> data) -> Result<void>
{
    return hal::WriteTo(&uart, data, sendTimeout);
}


auto WaitForAck() -> Result<void>
{
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
            return ErrorCode::invalidAnswer;
        }
    }
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
        return ErrorCode::invalidAnswer;
    }

    OUTCOME_TRY(auto dataLength, Receive<std::uint16_t>());
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
    auto buffer = SerialBuffer<T>{};
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
        return ErrorCode::dataPacketTooLong;
    }
    return hal::ReadFrom(&uart, data, receiveTimeout);
}


// TODO: A parameter pack of spans would be very convenient
auto ReceiveAndCheckCrc32(std::span<Byte const> data) -> Result<void>
{
    auto computedCrc32 = ComputeCrc32(data);
    OUTCOME_TRY(auto receivedCrc32, Receive<std::uint32_t>());
    if(computedCrc32 != receivedCrc32)
    {
        return ErrorCode::wrongChecksum;
    }
    return outcome_v2::success();
}


template<typename T>
auto Retry(auto (*communicationFunction)()->Result<T>, int nTries) -> Result<T>
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
        if(iTries >= nTries)
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
}

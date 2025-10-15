#include <Sts1CobcSw/Edu/Edu.hpp>

#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/ErrorDetectionAndCorrection/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/File.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <littlefs/lfs.h>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/format_spec.h>
#include <etl/to_string.h>
#include <etl/vector.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <system_error>
#include <utility>


namespace sts1cobcsw::edu
{
// --- Public globals ---

hal::GpioPin updateGpioPin(hal::eduUpdatePin);
hal::GpioPin dosiEnableGpioPin(hal::dosiEnablePin);


namespace
{
// --- Private globals ---

// Low-level CEP commands
constexpr auto cepAck = 0xd7_b;   //! Acknowledging a data packet
constexpr auto cepNack = 0x27_b;  //! Not Acknowledging an (invalid) data packet
constexpr auto cepEof = 0x59_b;   //! Transmission of multiple packets is complete
constexpr auto cepData = 0x8b_b;  //! Data packet format is used (not a command packet!)

// The max. data length is 11 KiB. At 115'200 baud, this takes about 1 second to transmit. We use
// 1.5 s just to be sure.
constexpr auto sendTimeout = 1500 * ms;
constexpr auto receiveTimeout = 1500 * ms;

// Max. number of send retries after receiving NACK
constexpr auto maxNNackRetries = 4;
// Max. number of data packets for a single command
constexpr auto maxNPackets = 100;
// Max. length of a single data packet
constexpr auto maxDataSize = 11 * 1024;
constexpr auto maxFileSize = maxNPackets * maxDataSize;

constexpr auto nProgramIdDigits =
    std::numeric_limits<strong::underlying_type_t<ProgramId>>::digits10 + 1;
constexpr auto nStartTimeDigits =
    std::numeric_limits<strong::underlying_type_t<RealTime>>::digits10 + 1;
constexpr auto * programFileExtension = ".zip";
constexpr auto * resultFileExtension = ".cpio";

// Data buffer for potentially large data packets (ReturnResult and StoreProgram)
auto cepDataBuffer = etl::vector<Byte, maxDataSize>{};

auto eduEnableGpioPin = hal::GpioPin(hal::eduEnablePin);
auto uart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);

auto semaphore = RODOS::Semaphore{};


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
}


// --- Public function definitions ---

auto Initialize() -> void
{
    {
        auto protector = RODOS::ScopeProtector(&semaphore);
        // TODO: Test how high we can set the baudrate without problems (bit errors, etc.)
        auto const baudRate = 115'200;
        hal::Initialize(&uart, baudRate);
        eduEnableGpioPin.SetDirection(hal::PinDirection::out);
    }
    TurnOff();
}


auto TurnOn() -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);
    persistentVariables.Store<"eduShouldBePowered">(true);
    eduEnableGpioPin.Set();
}


auto TurnOff() -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);
    persistentVariables.Store<"eduShouldBePowered">(false);
    eduEnableGpioPin.Reset();
}


//! @brief Issues a command to store a student program on the EDU and transfers it.
//!
//! Store Program (COBC <-> EDU):
//! -> [DATA]
//! -> [Command Header]
//! -> [Program ID]
//! <- [N/ACK]
//! -> [DATA]
//! -> [PROGRAM]
//! -> [EOF]
//! <- [N/ACK]
//! <- [N/ACK]
//!
//! The first N/ACK confirms a valid Program ID received,
//! the second N/ACK confirms that the program was received.
//! the third N/ACK confirms that the program was stored on the EDU successful.
//!
//! @param programId The student program ID
//!
//! @returns A relevant error code
auto StoreProgram(StoreProgramData const & data) -> Result<void>
{
    auto protector = RODOS::ScopeProtector(&semaphore);
    auto path = BuildProgramFilePath(data.programId);
    // NOLINTNEXTLINE(*signed-bitwise)
    OUTCOME_TRY(auto file, fs::Open(path, LFS_O_RDONLY));
    OUTCOME_TRY(auto fileSize, file.Size());
    if(fileSize > maxFileSize)
    {
        DEBUG_PRINT(
            "Program file %s is too large: %i B\n", path.c_str(), static_cast<int>(fileSize));
        return ErrorCode::fileTooLarge;
    }
    OUTCOME_TRY(SendDataPacket(Serialize(data)));
    while(true)
    {
        cepDataBuffer.uninitialized_resize(cepDataBuffer.MAX_SIZE);
        OUTCOME_TRY(auto nReadBytes, file.Read(Span(&cepDataBuffer)));
        cepDataBuffer.resize(static_cast<std::size_t>(nReadBytes));
        if(cepDataBuffer.empty())
        {
            break;
        }
        OUTCOME_TRY(SendDataPacket(Span(cepDataBuffer)));
    }
    OUTCOME_TRY(SendCommand(cepEof));
    OUTCOME_TRY(WaitForAck());
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
    auto protector = RODOS::ScopeProtector(&semaphore);
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
    auto protector = RODOS::ScopeProtector(&semaphore);
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
    auto protector = RODOS::ScopeProtector(&semaphore);
    OUTCOME_TRY(SendDataPacket(Serialize(GetStatusData())));
    OUTCOME_TRY(auto status, Retry(ReceiveAndParseStatusData, maxNNackRetries));
    OUTCOME_TRY(SendCommand(cepAck));
    return status;
}


// The high cognitive complexity comes from the OUTCOME_TRY macros which expand to extra if
// statements. However, you don't see them when reading the code so they don't really count. In
// fact, I think they make the code easier to read.
//
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto ReturnResult(ReturnResultData const & data) -> Result<void>
{
    auto protector = RODOS::ScopeProtector(&semaphore);
    auto path = BuildResultFilePath(data.programId, data.startTime);
    // NOLINTNEXTLINE(*signed-bitwise)
    OUTCOME_TRY(auto file, fs::Open(path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC));
    OUTCOME_TRY(SendDataPacket(Serialize(data)));
    for(auto nPackets = 0; nPackets < maxNPackets; ++nPackets)
    {
        OUTCOME_TRY(Retry(ReceiveDataPacket, maxNNackRetries));
        // If the buffer is empty we received an EOF and are done with the transmission
        if(cepDataBuffer.empty())
        {
            // The first ACK confirms that the packet was received correctly, the second ACK
            // confirms that the whole transmission completed successfully
            OUTCOME_TRY(SendCommand(cepAck));
            OUTCOME_TRY(SendCommand(cepAck));
            return outcome_v2::success();
        }
        auto writeResult = file.Write(Span(cepDataBuffer));
        if(writeResult.has_value())
        {
            OUTCOME_TRY(SendCommand(cepAck));
            continue;
        }
        DEBUG_PRINT("Failed to write %d bytes to %s: %s\n",
                    static_cast<int>(cepDataBuffer.size()),
                    path.c_str(),
                    ToCZString(writeResult.error()));
        OUTCOME_TRY(SendCommand(cepNack));
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
    auto protector = RODOS::ScopeProtector(&semaphore);
    OUTCOME_TRY(SendDataPacket(Serialize(data)));
    return WaitForAck();
}


auto BuildProgramFilePath(ProgramId programId) -> fs::Path
{
    auto path = programsDirectory;
    path.append("/");
    etl::to_string(value_of(programId),
                   path,
                   etl::format_spec().width(nProgramIdDigits).fill('0'),
                   /*append=*/true);
    path.append(programFileExtension);
    return path;
}


auto BuildResultFilePath(ProgramId programId, RealTime startTime) -> fs::Path
{
    auto path = resultsDirectory;
    path.append("/");
    etl::to_string(value_of(programId),
                   path,
                   etl::format_spec().width(nProgramIdDigits).fill('0'),
                   /*append=*/true);
    path.append("_");
    etl::to_string(value_of(startTime),
                   path,
                   etl::format_spec().width(nStartTimeDigits).fill('0'),
                   /*append=*/true);
    path.append(resultFileExtension);
    return path;
}


auto GetProgramId(fs::Path const & filename) -> Result<ProgramId>
{
    static constexpr auto programFilenameLength =
        nProgramIdDigits + std::char_traits<char>::length(programFileExtension);
    if(filename.size() == programFilenameLength and filename.ends_with(programFileExtension))
    {
        std::uint16_t value = 0;
        auto result = std::from_chars(filename.data(), filename.data() + nProgramIdDigits, value);
        if(result.ec == std::errc{})
        {
            return ProgramId(value);
        }
    }
    DEBUG_PRINT("Failed to get EDU program ID from file %s\n", filename.c_str());
    return ErrorCode::invalidEduProgramFilename;
}


auto ProgramsAreAvailableOnCobc() -> bool
{
    auto protector = RODOS::ScopeProtector(&semaphore);
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
                           return entryResult.has_value()
                              and entryResult.value().type == fs::EntryType::file
                              and GetProgramId(entryResult.value().name).has_value();
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
    if(statusId == EnableDosimeterData::id)
    {
        if(cepDataBuffer.size() != totalSerialSize<EnableDosimeterData>)
        {
            return ErrorCode::invalidLength;
        }
        return Status{.statusType = StatusType::enableDosimeter};
    }
    if(statusId == DisableDosimeterData::id)
    {
        if(cepDataBuffer.size() != totalSerialSize<DisableDosimeterData>)
        {
            return ErrorCode::invalidLength;
        }
        return Status{.statusType = StatusType::disableDosimeter};
    }
    return ErrorCode::invalidStatusType;
}


//! @brief Send a data packet over UART to the EDU.
//!
//! @param data The data to be sent
auto SendDataPacket(std::span<Byte const> data) -> Result<void>
{
    if(data.size() >= maxDataSize)
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

        hal::FlushReceiveBuffer(&uart);
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

    OUTCOME_TRY(auto dataSize, Receive<std::uint16_t>());
    if(dataSize == 0 or dataSize > maxDataSize)
    {
        return ErrorCode::invalidLength;
    }
    cepDataBuffer.resize(dataSize);

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
    if(data.size() > maxDataSize)
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
        hal::FlushReceiveBuffer(&uart);
        OUTCOME_TRY(SendCommand(cepNack));
        iTries++;
        if(iTries >= nTries)
        {
            // TODO: Maybe return tooManyNacks here instead?
            return result.error();
        }
    }
}
}
}

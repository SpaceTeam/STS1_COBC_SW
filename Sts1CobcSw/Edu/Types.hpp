#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
namespace edu
{
using sts1cobcsw::operator""_b;


// TODO: Check if all those error codes are really needed. For example invalidResult and
// invalidCommand are used for the same thing.
enum class ErrorCode
{
    invalidResult = 1,
    bufferTooSmall,
    uartNotInitialized,
    timeout,
    nack,
    // Separate errors for the SendData function to differentiate where the error occurred
    invalidDataResult,
    sendDataTooLong,
    receiveDataTooLong,
    tooManyNacks,
    dataTimeout,
    wrongChecksum,
    invalidStatusType,
    invalidLength,
    invalidCommand,
    noResultAvailable
};


enum class StatusType
{
    noEvent,
    programFinished,
    resultsReady,
    invalid,
};


struct StoreProgramData
{
    static constexpr auto id = 0x01_b;
    std::uint16_t programId = 0;
};


struct ExecuteProgramData
{
    static constexpr auto id = 0x02_b;
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
    std::int16_t timeout = 0;
};


struct StopProgramData
{
    static constexpr auto id = 0x03_b;
};


struct GetStatusData
{
    static constexpr auto id = 0x04_b;
};


struct ReturnResultData
{
    static constexpr auto id = 0x05_b;
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
};


struct UpdateTimeData
{
    static constexpr auto id = 0x06_b;
    std::int32_t currentTime = 0;
};


struct Status
{
    StatusType statusType = StatusType::invalid;
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
    std::uint8_t exitCode = 0;
};


// TODO: Rename to ProgramFinishedData, add ID/code, and add NoEventData
struct ProgramFinishedStatus
{
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
    std::uint8_t exitCode = 0;
};


struct ResultsReadyStatus
{
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
};


struct ResultInfo
{
    bool eofIsReached = false;
    std::size_t resultSize = 0;
};
}


template<>
inline constexpr std::size_t serialSize<edu::StoreProgramData> =
    totalSerialSize<decltype(edu::StoreProgramData::id),
                    decltype(edu::StoreProgramData::programId)>;

template<>
inline constexpr std::size_t serialSize<edu::ExecuteProgramData> =
    totalSerialSize<decltype(edu::ExecuteProgramData::id),
                    decltype(edu::ExecuteProgramData::programId),
                    decltype(edu::ExecuteProgramData::startTime),
                    decltype(edu::ExecuteProgramData::timeout)>;

template<>
inline constexpr std::size_t serialSize<edu::StopProgramData> =
    totalSerialSize<decltype(edu::StopProgramData::id)>;

template<>
inline constexpr std::size_t serialSize<edu::GetStatusData> =
    totalSerialSize<decltype(edu::GetStatusData::id)>;

template<>
inline constexpr std::size_t serialSize<edu::ReturnResultData> =
    totalSerialSize<decltype(edu::ReturnResultData::id),
                    decltype(edu::ReturnResultData::programId),
                    decltype(edu::ReturnResultData::startTime)>;

template<>
inline constexpr std::size_t serialSize<edu::UpdateTimeData> =
    totalSerialSize<decltype(edu::UpdateTimeData::id), decltype(edu::UpdateTimeData::currentTime)>;

template<>
inline constexpr std::size_t serialSize<edu::ProgramFinishedStatus> =
    totalSerialSize<decltype(edu::ProgramFinishedStatus::programId),
                    decltype(edu::ProgramFinishedStatus::startTime),
                    decltype(edu::ProgramFinishedStatus::exitCode)>;

template<>
inline constexpr std::size_t serialSize<edu::ResultsReadyStatus> =
    totalSerialSize<decltype(edu::ResultsReadyStatus::programId),
                    decltype(edu::ResultsReadyStatus::startTime)>;


namespace edu
{
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, StoreProgramData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ExecuteProgramData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, StopProgramData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, GetStatusData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ReturnResultData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, UpdateTimeData const & data) -> void *;
template<std::endian endianness>

[[nodiscard]] auto DeserializeFrom(void const * source, ProgramFinishedStatus * data)
    -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ResultsReadyStatus * data) -> void const *;
}
}


#include <Sts1CobcSw/Edu/Types.ipp>

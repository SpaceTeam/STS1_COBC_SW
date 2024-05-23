#pragma once


#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
namespace edu
{
using sts1cobcsw::operator""_b;


enum class ErrorCode
{
    timeout = 1,
    invalidAnswer,
    nack,
    tooManyNacks,
    wrongChecksum,
    dataPacketTooLong,
    invalidStatusType,
    invalidLength,
    tooManyDataPackets,
    fileSystemError
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
    ProgramId programId = ProgramId(0);
};


struct ExecuteProgramData
{
    static constexpr auto id = 0x02_b;
    ProgramId programId = ProgramId(0);
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
    ProgramId programId = ProgramId(0);
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
    ProgramId programId = ProgramId(0);
    std::int32_t startTime = 0;
    std::uint8_t exitCode = 0;
};


struct NoEventData
{
    static constexpr auto id = 0x00_b;
};


struct ProgramFinishedData
{
    static constexpr auto id = 0x01_b;
    ProgramId programId = ProgramId(0);
    std::int32_t startTime = 0;
    std::uint8_t exitCode = 0;
};


struct ResultsReadyData
{
    static constexpr auto id = 0x02_b;
    ProgramId programId = ProgramId(0);
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
inline constexpr std::size_t serialSize<edu::NoEventData> =
    totalSerialSize<decltype(edu::NoEventData::id)>;

template<>
inline constexpr std::size_t serialSize<edu::ProgramFinishedData> =
    totalSerialSize<decltype(edu::ProgramFinishedData::id),
                    decltype(edu::ProgramFinishedData::programId),
                    decltype(edu::ProgramFinishedData::startTime),
                    decltype(edu::ProgramFinishedData::exitCode)>;

template<>
inline constexpr std::size_t serialSize<edu::ResultsReadyData> =
    totalSerialSize<decltype(edu::ResultsReadyData::id),
                    decltype(edu::ResultsReadyData::programId),
                    decltype(edu::ResultsReadyData::startTime)>;


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
[[nodiscard]] auto DeserializeFrom(void const * source, NoEventData * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramFinishedData * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ResultsReadyData * data) -> void const *;
}
}


#include <Sts1CobcSw/Edu/Types.ipp>

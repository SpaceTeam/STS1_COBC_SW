#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
namespace edu
{
using sts1cobcsw::operator""_b;


enum class ProgramStatus : std::uint8_t
{
    programRunning,
    // TODO: The following is unused. Can we remove it or did we forget it somewhere?
    programCouldNotBeStarted,
    programExecutionFailed,
    programExecutionSucceeded,
    resultStoredInFileSystem,
    resultRequestedByGround,
    resultAcknowledgedByGround,
    resultDeleted
};


enum class StatusType : std::uint8_t
{
    noEvent,
    programFinished,
    resultsReady,
    enableDosimeter,
    disableDosimeter,
    invalid,
};


struct ProgramQueueEntry
{
    ProgramId programId = ProgramId(0);
    RealTime startTime = RealTime(0);
    std::int16_t timeout = 0;
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
    RealTime startTime = RealTime(0);
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
    RealTime startTime = RealTime(0);
};


struct UpdateTimeData
{
    static constexpr auto id = 0x06_b;
    RealTime currentTime = RealTime(0);
};


struct Status
{
    StatusType statusType = StatusType::invalid;
    ProgramId programId = ProgramId(0);
    RealTime startTime = RealTime(0);
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
    RealTime startTime = RealTime(0);
    std::uint8_t exitCode = 0;
};


struct ResultsReadyData
{
    static constexpr auto id = 0x02_b;
    ProgramId programId = ProgramId(0);
    RealTime startTime = RealTime(0);
};


struct EnableDosimeterData
{
    static constexpr auto id = 0x03_b;
};


struct DisableDosimeterData
{
    static constexpr auto id = 0x04_b;
};


struct ResultInfo
{
    bool eofIsReached = false;
    std::size_t resultSize = 0;
};
}


template<>
inline constexpr std::size_t serialSize<edu::ProgramQueueEntry> =
    totalSerialSize<decltype(edu::ProgramQueueEntry::programId),
                    decltype(edu::ProgramQueueEntry::startTime),
                    decltype(edu::ProgramQueueEntry::timeout)>;

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

template<>
inline constexpr std::size_t serialSize<edu::EnableDosimeterData> =
    totalSerialSize<decltype(edu::EnableDosimeterData::id)>;

template<>
inline constexpr std::size_t serialSize<edu::DisableDosimeterData> =
    totalSerialSize<decltype(edu::DisableDosimeterData::id)>;


namespace edu
{
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ProgramQueueEntry const & data) -> void *;
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
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramQueueEntry * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, NoEventData * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramFinishedData * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ResultsReadyData * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, EnableDosimeterData * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, DisableDosimeterData * data)
    -> void const *;
}
}


#include <Sts1CobcSw/Edu/Types.ipp>

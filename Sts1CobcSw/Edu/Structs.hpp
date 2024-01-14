#pragma once


#include <Sts1CobcSw/Edu/Enums.hpp>
#include <Sts1CobcSw/Edu/Names.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
namespace edu
{
using sts1cobcsw::operator""_b;


struct HeaderData
{
    Byte command = 0x00_b;
    std::uint16_t length = 0;
};


// TODO: Add = 0; to all std::(u)intxx_t fields


struct StoreProgramData
{
    static constexpr auto id = storeProgramId;
    std::uint16_t programId;
};


struct ExecuteProgramData
{
    static constexpr auto id = executeProgramId;
    std::uint16_t programId;
    std::int32_t startTime;
    std::int16_t timeout;
};


struct ReturnResultData
{
    static constexpr auto id = returnResultId;
    std::uint16_t programId = 0U;
    std::int32_t startTime = 0U;
};


struct UpdateTimeData
{
    static constexpr auto id = updateTimeId;
    std::int32_t currentTime;
};


struct Status
{
    StatusType statusType = StatusType::invalid;
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
    std::uint8_t exitCode = 0;
};


struct ResultsReadyStatus
{
    std::uint16_t programId;
    std::int32_t startTime;
};


struct ProgramFinishedStatus
{
    std::uint16_t programId;
    std::int32_t startTime;
    std::uint8_t exitCode;
};


struct ResultInfo
{
    bool eofIsReached = false;
    std::size_t resultSize = 0;
};
}


template<>
inline constexpr std::size_t serialSize<edu::HeaderData> =
    totalSerialSize<decltype(edu::HeaderData::command), decltype(edu::HeaderData::length)>;

template<>
inline constexpr std::size_t serialSize<edu::ProgramFinishedStatus> =
    totalSerialSize<decltype(edu::ProgramFinishedStatus::programId),
                    decltype(edu::ProgramFinishedStatus::startTime),
                    decltype(edu::ProgramFinishedStatus::exitCode)>;

template<>
inline constexpr std::size_t serialSize<edu::ResultsReadyStatus> =
    totalSerialSize<decltype(edu::ResultsReadyStatus::programId),
                    decltype(edu::ResultsReadyStatus::startTime)>;

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
inline constexpr std::size_t serialSize<edu::ReturnResultData> =
    totalSerialSize<decltype(edu::ReturnResultData::id),
                    decltype(edu::ReturnResultData::programId),
                    decltype(edu::ReturnResultData::startTime)>;

template<>
inline constexpr std::size_t serialSize<edu::UpdateTimeData> =
    totalSerialSize<decltype(edu::UpdateTimeData::id), decltype(edu::UpdateTimeData::currentTime)>;


namespace edu
{
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, HeaderData * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramFinishedStatus * data)
    -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ResultsReadyStatus * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, StoreProgramData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ExecuteProgramData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ReturnResultData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, UpdateTimeData const & data) -> void *;
}
}


#include <Sts1CobcSw/Edu/Structs.ipp>

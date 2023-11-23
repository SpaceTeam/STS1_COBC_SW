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


struct StoreArchiveData
{
    static constexpr auto id = storeArchiveId;
    std::uint16_t programId;
};


struct ExecuteProgramData
{
    static constexpr auto id = executeProgramId;
    std::uint16_t programId;
    std::uint16_t queueId;
    std::int16_t timeout;
};


struct UpdateTimeData
{
    static constexpr auto id = updateTimeId;
    std::int32_t timestamp;
};


struct Status
{
    StatusType statusType = StatusType::invalid;
    std::uint16_t programId = 0;
    std::uint16_t queueId = 0;
    std::uint8_t exitCode = 0;
    ErrorCode errorCode = ErrorCode::noErrorCodeSet;
};


struct ResultsReadyStatus
{
    std::uint16_t programId;
    std::uint16_t queueId;
};


struct ProgramFinishedStatus
{
    std::uint16_t programId;
    std::uint16_t queueId;
    std::uint8_t exitCode;
};


struct ResultInfo
{
    ErrorCode errorCode = ErrorCode::noErrorCodeSet;
    std::size_t resultSize = 0;
};
}


template<>
inline constexpr std::size_t serialSize<edu::HeaderData> =
    totalSerialSize<decltype(edu::HeaderData::command), decltype(edu::HeaderData::length)>;

template<>
inline constexpr std::size_t serialSize<edu::ProgramFinishedStatus> =
    totalSerialSize<decltype(edu::ProgramFinishedStatus::programId),
                    decltype(edu::ProgramFinishedStatus::queueId),
                    decltype(edu::ProgramFinishedStatus::exitCode)>;

template<>
inline constexpr std::size_t serialSize<edu::ResultsReadyStatus> =
    totalSerialSize<decltype(edu::ResultsReadyStatus::programId),
                    decltype(edu::ResultsReadyStatus::queueId)>;

template<>
inline constexpr std::size_t serialSize<edu::StoreArchiveData> =
    totalSerialSize<decltype(edu::StoreArchiveData::id),
                    decltype(edu::StoreArchiveData::programId)>;

template<>
inline constexpr std::size_t serialSize<edu::ExecuteProgramData> =
    totalSerialSize<decltype(edu::ExecuteProgramData::id),
                    decltype(edu::ExecuteProgramData::programId),
                    decltype(edu::ExecuteProgramData::queueId),
                    decltype(edu::ExecuteProgramData::timeout)>;

template<>
inline constexpr std::size_t serialSize<edu::UpdateTimeData> =
    totalSerialSize<decltype(edu::UpdateTimeData::id), decltype(edu::UpdateTimeData::timestamp)>;


namespace edu
{
template<std::endian endianness>
auto DeserializeFrom(void const * source, HeaderData * data) -> void const *;
template<std::endian endianness>
auto DeserializeFrom(void const * source, ProgramFinishedStatus * data) -> void const *;
template<std::endian endianness>
auto DeserializeFrom(void const * source, ResultsReadyStatus * data) -> void const *;
template<std::endian endianness>
auto SerializeTo(void * destination, StoreArchiveData const & data) -> void *;
template<std::endian endianness>
auto SerializeTo(void * destination, ExecuteProgramData const & data) -> void *;
template<std::endian endianness>
auto SerializeTo(void * destination, UpdateTimeData const & data) -> void *;
}
}


#include <Sts1CobcSw/Edu/Structs.ipp>

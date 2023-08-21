#pragma once


#include <Sts1CobcSw/Edu/Enums.hpp>
#include <Sts1CobcSw/Edu/Names.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <type_safe/types.hpp>

#include <cstdint>


namespace sts1cobcsw
{
namespace edu
{
namespace ts = type_safe;

using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::operator""_b;
using ts::operator""_u16;
using ts::operator""_usize;


struct HeaderData
{
    Byte command = 0x00_b;
    ts::uint16_t length = 0_u16;
};


struct StoreArchiveData
{
    static constexpr auto id = storeArchiveId;
    ts::uint16_t programId;
};


struct ExecuteProgramData
{
    static constexpr auto id = executeProgramId;
    ts::uint16_t programId;
    ts::uint16_t queueId;
    ts::int16_t timeout;
};


struct UpdateTimeData
{
    static constexpr auto id = updateTimeId;
    ts::int32_t timestamp;
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
    ts::size_t resultSize = 0_usize;
};
}


namespace serial
{
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
}


namespace edu
{
auto DeserializeFrom(void const * source, HeaderData * data) -> void const *;
auto DeserializeFrom(void const * source, ProgramFinishedStatus * data) -> void const *;
auto DeserializeFrom(void const * source, ResultsReadyStatus * data) -> void const *;
auto SerializeTo(void * destination, StoreArchiveData const & data) -> void *;
auto SerializeTo(void * destination, ExecuteProgramData const & data) -> void *;
auto SerializeTo(void * destination, UpdateTimeData const & data) -> void *;
}
}

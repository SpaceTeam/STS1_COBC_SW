#pragma once

#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <type_safe/types.hpp>

#include <cstdint>


namespace sts1cobcsw
{
namespace periphery
{
namespace ts = type_safe;
using sts1cobcsw::serial::Byte;


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


struct EduStatus
{
    EduStatusType statusType;
    uint16_t programId;
    uint16_t queueId;
    uint8_t exitCode;
    EduErrorCode errorCode;
};


struct ResultsReadyStatus
{
    uint16_t programId;
    uint16_t queueId;
};


struct ProgramFinishedStatus
{
    uint16_t programId;
    uint16_t queueId;
    uint8_t exitCode;
};


struct ResultInfo
{
    periphery::EduErrorCode errorCode;
    size_t resultSize;
};


auto DeserializeFrom(Byte * source, ProgramFinishedStatus * data) -> Byte *;
auto DeserializeFrom(Byte * source, ResultsReadyStatus * data) -> Byte *;
auto SerializeTo(Byte * destination, StoreArchiveData const & data) -> Byte *;
auto SerializeTo(Byte * destination, ExecuteProgramData const & data) -> Byte *;
auto SerializeTo(Byte * destination, UpdateTimeData const & data) -> Byte *;
}


namespace serial
{
template<>
inline constexpr std::size_t serialSize<periphery::ProgramFinishedStatus> =
    totalSerialSize<decltype(periphery::ProgramFinishedStatus::programId),
                    decltype(periphery::ProgramFinishedStatus::queueId),
                    decltype(periphery::ProgramFinishedStatus::exitCode)>;

template<>
inline constexpr std::size_t serialSize<periphery::ResultsReadyStatus> =
    totalSerialSize<decltype(periphery::ResultsReadyStatus::programId),
                    decltype(periphery::ResultsReadyStatus::queueId)>;

template<>
inline constexpr std::size_t serialSize<periphery::StoreArchiveData> =
    totalSerialSize<decltype(periphery::StoreArchiveData::id),
                    decltype(periphery::StoreArchiveData::programId)>;

template<>
inline constexpr std::size_t serialSize<periphery::ExecuteProgramData> =
    totalSerialSize<decltype(periphery::ExecuteProgramData::id),
                    decltype(periphery::ExecuteProgramData::programId),
                    decltype(periphery::ExecuteProgramData::queueId),
                    decltype(periphery::ExecuteProgramData::timeout)>;

template<>
inline constexpr std::size_t serialSize<periphery::UpdateTimeData> =
    totalSerialSize<decltype(periphery::UpdateTimeData::id),
                    decltype(periphery::UpdateTimeData::timestamp)>;
}
}
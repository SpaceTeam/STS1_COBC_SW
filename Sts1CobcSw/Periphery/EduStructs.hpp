#pragma once

#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>


namespace sts1cobcsw
{
namespace periphery
{
using sts1cobcsw::serial::Byte;


struct DataHeader
{
    uint8_t command;
    uint16_t length;
};


struct ResultsReadyStatus
{
    uint16_t programId;
    uint16_t queueId;
};


struct ExecuteProgramData
{
    uint8_t commandType;
    uint16_t programId;
    uint16_t queueId;
    uint16_t timeout;
};


struct UpdateTimeData
{
    uint8_t commandType;
    int32_t timestamp;
};


struct EduStatus
{
    periphery::EduStatusType statusType;
    uint16_t programId;
    uint16_t queueId;
    uint8_t exitCode;
    periphery::EduErrorCode errorCode;
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
inline constexpr std::size_t serialSize<periphery::ExecuteProgramData> =
    totalSerialSize<decltype(periphery::ExecuteProgramData::commandType),
                    decltype(periphery::ExecuteProgramData::programId),
                    decltype(periphery::ExecuteProgramData::queueId),
                    decltype(periphery::ExecuteProgramData::timeout)>;

template<>
inline constexpr std::size_t serialSize<periphery::UpdateTimeData> =
    totalSerialSize<decltype(periphery::UpdateTimeData::commandType),
                    decltype(periphery::UpdateTimeData::timestamp)>;
}
}
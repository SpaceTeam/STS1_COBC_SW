#pragma once

#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>


namespace sts1cobcsw
{
namespace periphery
{
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::DeserializeFrom;
using sts1cobcsw::serial::SerializeTo;


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


struct ExecuteProgramArguments
{
    uint8_t header;
    uint16_t programId;
    uint16_t queueId;
    uint16_t timeout;
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


auto SerializeTo(Byte * destination, ExecuteProgramArguments const & data) -> Byte *;
}


namespace serial
{
template<>
inline constexpr std::size_t serialSize<periphery::ResultsReadyStatus> =
    totalSerialSize<decltype(periphery::ResultsReadyStatus::programId),
                    decltype(periphery::ResultsReadyStatus::queueId)>;


template<>
inline constexpr std::size_t serialSize<periphery::ProgramFinishedStatus> =
    totalSerialSize<decltype(periphery::ProgramFinishedStatus::programId),
                    decltype(periphery::ProgramFinishedStatus::queueId),
                    decltype(periphery::ProgramFinishedStatus::exitCode)>;


template<>
inline constexpr std::size_t serialSize<periphery::ExecuteProgramArguments> =
    totalSerialSize<decltype(periphery::ExecuteProgramArguments::header),
                    decltype(periphery::ExecuteProgramArguments::programId),
                    decltype(periphery::ExecuteProgramArguments::queueId),
                    decltype(periphery::ExecuteProgramArguments::timeout)>;
}
}
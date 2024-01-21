#pragma once


#include <Sts1CobcSw/Edu/Enums.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
namespace edu
{
using sts1cobcsw::operator""_b;


// TODO: Directly define the numbers in the <Command>Data structs

// CEP high-level command headers (see EDU PDD)
inline constexpr auto storeProgramId = 0x01_b;    //! Transfer student programs from COBC to EDU
inline constexpr auto executeProgramId = 0x02_b;  //! Execute student program
inline constexpr auto stopProgramId = 0x03_b;     //! Stop student program
inline constexpr auto getStatusId = 0x04_b;       //! Get the student program status
inline constexpr auto returnResultId = 0x05_b;    //! Request student program result
inline constexpr auto updateTimeId = 0x06_b;      //! Update EDU system time


struct StoreProgramData
{
    static constexpr auto id = storeProgramId;
    std::uint16_t programId = 0;
};


struct ExecuteProgramData
{
    static constexpr auto id = executeProgramId;
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
    std::int16_t timeout = 0;
};


struct ReturnResultData
{
    static constexpr auto id = returnResultId;
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
};


struct UpdateTimeData
{
    static constexpr auto id = updateTimeId;
    std::int32_t currentTime = 0;
};


struct Status
{
    StatusType statusType = StatusType::invalid;
    std::uint16_t programId = 0;
    std::int32_t startTime = 0;
    std::uint8_t exitCode = 0;
};


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
[[nodiscard]] auto SerializeTo(void * destination, StoreProgramData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ExecuteProgramData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ReturnResultData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, UpdateTimeData const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramFinishedStatus * data)
    -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ResultsReadyStatus * data) -> void const *;
template<std::endian endianness>
}
}


#include <Sts1CobcSw/Edu/Structs.ipp>

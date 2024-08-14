#pragma once

#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>


namespace sts1cobcsw
{
//! @brief Given a time in seconds since 01.01.1970, return a time in nanoseconds since 01.01.2000.
inline auto UnixToRodosTime(std::int32_t unixTimeSeconds) -> std::int64_t
{
    return static_cast<std::int64_t>(unixTimeSeconds) * RODOS::SECONDS - rodosUnixOffset;
}


inline auto GetUnixUtc() -> std::int32_t
{
    auto unixUtc = (RODOS::sysTime.getUTC() + rodosUnixOffset) / RODOS::SECONDS;
    return static_cast<std::int32_t>(unixUtc);
}


template<std::endian endianness>
inline auto SerializeTo(void * destination, RealTime const & data) -> void *
{
    return SerializeTo<endianness>(destination, value_of(data));
}


template<std::endian endianness>
inline auto DeserializeFrom(void const * source, RealTime * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(value_of(*data)));
    return source;
}
}

#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
struct Parameter
{
    enum class Id : std::uint8_t
    {
        rxBaudRate = 1,
        txBaudRate,
        realTimeOffsetCorrection,
        newEduResultIsAvailable,
        eduStartDelayLimit,
    };
    using Value = std::uint32_t;

    Id parameterId;
    Value parameterValue;
};


template<>
inline constexpr std::size_t serialSize<Parameter> =
    totalSerialSize<Parameter::Id, Parameter::Value>;


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, Parameter const & parameter) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, Parameter * parameter) -> void const *;
}

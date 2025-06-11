#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
struct MessageTypeIdFields
{
    std::uint8_t serviceTypeId = 0;
    std::uint8_t messageSubtypeId = 0;

    friend constexpr auto operator==(MessageTypeIdFields const &, MessageTypeIdFields const &)
        -> bool = default;
};


template<>
inline constexpr std::size_t serialSize<MessageTypeIdFields> =
    totalSerialSize<decltype(MessageTypeIdFields::serviceTypeId),
                    decltype(MessageTypeIdFields::messageSubtypeId)>;


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
[[nodiscard]] auto SerializeTo(void * destination, MessageTypeIdFields const & fields) -> void *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, Parameter const & parameter) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, MessageTypeIdFields * fields)
    -> void const *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, Parameter * parameter) -> void const *;
}

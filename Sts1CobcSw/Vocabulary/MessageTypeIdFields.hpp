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


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, MessageTypeIdFields const & fields) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, MessageTypeIdFields * fields)
    -> void const *;
}


#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.ipp>  // IWYU pragma: keep

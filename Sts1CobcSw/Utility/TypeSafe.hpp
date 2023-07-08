#pragma once


#include <type_safe/types.hpp>

#include <type_traits>


namespace sts1cobcsw::utility
{
namespace ts = type_safe;

using ts::operator""_i8;
using ts::operator""_u8;
using ts::operator""_i16;
using ts::operator""_u16;
using ts::operator""_i32;
using ts::operator""_u32;
using ts::operator""_i64;
using ts::operator""_u64;


template<typename T>
struct IsTypeSafeInteger : std::false_type
{
};


template<std::integral T>
struct IsTypeSafeInteger<ts::integer<T>> : std::true_type
{
};


template<typename T>
concept TypeSafeInteger = IsTypeSafeInteger<T>::value;


template<typename T>
[[nodiscard]] auto TypeSafeZero() -> T;


template<>
inline auto TypeSafeZero<ts::int8_t>() -> ts::int8_t
{
    return 0_i8;
}


template<>
inline auto TypeSafeZero<ts::uint8_t>() -> ts::uint8_t
{
    return 0_u8;
}


template<>
inline auto TypeSafeZero<ts::int16_t>() -> ts::int16_t
{
    return 0_i16;
}


template<>
inline auto TypeSafeZero<ts::uint16_t>() -> ts::uint16_t
{
    return 0_u16;
}


template<>
inline auto TypeSafeZero<ts::int32_t>() -> ts::int32_t
{
    return 0_i32;
}


template<>
inline auto TypeSafeZero<ts::uint32_t>() -> ts::uint32_t
{
    return 0_u32;
}


template<>
inline auto TypeSafeZero<ts::int64_t>() -> ts::int64_t
{
    return 0_i64;
}


template<>
inline auto TypeSafeZero<ts::uint64_t>() -> ts::uint64_t
{
    return 0_u64;
}
}

// @file
// @brief   Library for serializing and deserializing types
//
// End users should only directly call Serialize() and Deserialize(). These functions use the
// low-level SerializeTo() and DeserializeFrom() which actually define how the (de-)serialization is
// done. The low-level functions together with the variable template serialSize<> must be overloaded
// for user-defined types to be (de-)serializable. For TriviallySerializable types this is already
// done in this header. User-defined types should build on that.
//
// An example for (de-)serializing a simple user-defined type can be found in the unit test
// Serial.test.cpp.

#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <span>
#include <type_traits>


// TODO: Add template parameter for endianness (Flash needs big endian)
// TODO: Enforce endianness with std::endian::native, std::endian::little, std::byteswap, etc.
namespace sts1cobcsw
{
template<typename T>
concept TriviallySerializable = std::is_arithmetic_v<T> or std::is_enum_v<T>;

template<typename T>
concept HasEndianness = std::is_integral_v<T> or std::is_enum_v<T>;


// Must be specialized for user-defined types to be serializable
template<typename T>
inline constexpr std::size_t serialSize = 0U;

template<TriviallySerializable T>
inline constexpr std::size_t serialSize<T> = sizeof(T);

template<typename... Ts>
inline constexpr std::size_t totalSerialSize = (serialSize<Ts> + ...);


template<typename T>
    requires(serialSize<T> != 0U)
using Buffer = std::array<Byte, serialSize<T>>;

template<typename T>
    requires(serialSize<T> != 0U)
using BufferView = std::span<Byte const, serialSize<T>>;


// --- Function declarations ---

// Must be overloaded for user-defined types to be serializable
template<TriviallySerializable T>
auto SerializeTo(void * destination, T const & t) -> void *;

// Must be overloaded for user-defined types to be deserializable
template<TriviallySerializable T>
auto DeserializeFrom(void const * source, T * t) -> void const *;

template<typename T>
[[nodiscard]] auto Serialize(T const & t) -> Buffer<T>;

template<std::default_initializable T>
[[nodiscard]] auto Deserialize(BufferView<T> bufferView) -> T;

template<HasEndianness T>
[[nodiscard]] constexpr auto ReverseBytes(T t) -> T;


// --- Function template definitions ---

template<TriviallySerializable T>
inline auto SerializeTo(void * destination, T const & t) -> void *
{
    std::memcpy(destination, &t, serialSize<T>);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<Byte *>(destination) + serialSize<T>;
}


template<TriviallySerializable T>
inline auto DeserializeFrom(void const * source, T * t) -> void const *
{
    std::memcpy(t, source, serialSize<T>);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<Byte const *>(source) + serialSize<T>;
}


template<typename T>
[[nodiscard]] auto Serialize(T const & t) -> Buffer<T>
{
    auto buffer = Buffer<T>{};
    SerializeTo(buffer.data(), t);
    return buffer;
}


template<std::default_initializable T>
[[nodiscard]] auto Deserialize(BufferView<T> bufferView) -> T
{
    auto t = T{};
    DeserializeFrom(bufferView.data(), &t);
    return t;
}


template<HasEndianness T>
constexpr inline auto ReverseBytes(T t) -> T
{
    if constexpr(std::integral<T>)
    {
        return std::byteswap(t);
    }
    else if constexpr(std::is_enum_v<T>)
    {
        return static_cast<T>(std::byteswap(std::to_underlying(t)));
    }
}
}

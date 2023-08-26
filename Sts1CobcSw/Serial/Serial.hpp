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
#include <Sts1CobcSw/Utility/TypeSafe.hpp>

#include <type_safe/boolean.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <span>
#include <type_traits>


// TODO: Enforce endianness with std::endian::native, std::endian::little, std::byteswap, etc.
namespace sts1cobcsw
{
// T::integer_type is for the type_safe fixed-width integers
template<typename T>
concept TriviallySerializable =
    std::is_arithmetic_v<T> or std::is_enum_v<T> or std::is_arithmetic_v<typename T::integer_type>
    or std::is_enum_v<typename T::integer_type> or std::is_same_v<T, type_safe::boolean>;


// Must be specialized for user-defined types to be serializable
template<typename T>
inline constexpr std::size_t serialSize = 0U;

template<TriviallySerializable T>
inline constexpr std::size_t serialSize<T> = sizeof(T);

template<typename... Ts>
inline constexpr std::size_t totalSerialSize = (serialSize<Ts> + ...);


template<typename T>
    requires(serialSize<T> != 0U)
using SerialBuffer = std::array<Byte, serialSize<T>>;


// --- Function declarations ---

// TODO: Rename data -> t or variable
// Must be overloaded for user-defined types to be serializable
template<TriviallySerializable T>
auto SerializeTo(void * destination, T const & data) -> void *;

// TODO: Make DeserializeFrom const correct (Byte const * source, -> Byte const *)
// Must be overloaded for user-defined types to be deserializable
template<TriviallySerializable T>
auto DeserializeFrom(void const * source, T * data) -> void const *;

template<typename T>
[[nodiscard]] auto Serialize(T const & data) -> SerialBuffer<T>;

template<std::default_initializable T>
[[nodiscard]] auto Deserialize(std::span<const Byte, serialSize<T>> source) -> T;

template<utility::TypeSafeInteger T>
[[nodiscard]] auto Deserialize(std::span<const Byte, serialSize<T>> source) -> T;

template<typename T>
    requires std::is_same_v<T, type_safe::boolean>
[[nodiscard]] auto Deserialize(std::span<const Byte, serialSize<T>> source) -> T;


// --- Function template definitions ---

template<TriviallySerializable T>
inline auto SerializeTo(void * destination, T const & data) -> void *
{
    std::memcpy(destination, &data, serialSize<T>);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<Byte *>(destination) + serialSize<T>;
}


// TODO: Add template parameter for endianness (Flash needs big endian)
template<TriviallySerializable T>
inline auto DeserializeFrom(void const * source, T * data) -> void const *
{
    std::memcpy(data, source, serialSize<T>);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<Byte const *>(source) + serialSize<T>;
}


template<typename T>
auto Serialize(T const & data) -> SerialBuffer<T>
{
    auto buffer = SerialBuffer<T>{};
    SerializeTo(buffer.data(), data);
    return buffer;
}


template<std::default_initializable T>
auto Deserialize(std::span<const Byte, serialSize<T>> source) -> T
{
    auto t = T{};
    DeserializeFrom(source.data(), &t);
    return t;
}


template<utility::TypeSafeInteger T>
auto Deserialize(std::span<const Byte, serialSize<T>> source) -> T
{
    auto t = utility::TypeSafeZero<T>();
    DeserializeFrom(source.data(), &t);
    return t;
}


template<typename T>
    requires std::is_same_v<T, type_safe::boolean>
auto Deserialize(std::span<const Byte, serialSize<T>> source) -> T
{
    auto t = T{false};  // NOLINT(bugprone-argument-comment)
    DeserializeFrom(source.data(), &t);
    return t;
}
}

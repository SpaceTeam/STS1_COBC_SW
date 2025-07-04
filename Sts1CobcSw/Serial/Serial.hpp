//! @file
//! @brief  Library for serializing and deserializing types
//!
//! End users should only directly call Serialize() and Deserialize(). These functions use the
//! low-level SerializeTo() and DeserializeFrom() which actually define how the (de-)serialization
//! is done. The low-level functions together with the variable template serialSize<> must be
//! overloaded for user-defined types to be (de-)serializable. For TriviallySerializable types this
//! is already done in this header. User-defined types should build on that.
//!
//! An example for (de-)serializing a simple user-defined type can be found in the unit test
//! Serial.test.cpp.

#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <strong_type/type.hpp>


// We need std::byteswap which is C++23 but for some reason clang-tidy crashes when using C++23, so
// we use the ETL version
#include <etl/bit.h>
#include <etl/string.h>
#include <etl/vector.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <span>
#include <type_traits>


namespace sts1cobcsw
{
template<typename T>
concept TriviallySerializable = std::is_arithmetic_v<T> or std::is_enum_v<T>;

// HasEndianness = TriviallySerializable - floats because is_integral_v = is_arithmetic_v - floats
template<typename T>
concept HasEndianness = std::is_integral_v<T> or std::is_enum_v<T>;


// Must be specialized for user-defined types to be serializable
template<typename T>
inline constexpr std::size_t serialSize = 0;

template<TriviallySerializable T>
inline constexpr std::size_t serialSize<T> = sizeof(T);

template<AUInt T>
inline constexpr std::size_t serialSize<T> = serialSize<SmallestUnsignedType<T::size>>;

template<typename T>
    requires(strong::is_strong_type<T>::value and serialSize<strong::underlying_type_t<T>> > 0)
inline constexpr std::size_t serialSize<T> = serialSize<strong::underlying_type_t<T>>;

template<typename T, std::size_t size>
    requires(serialSize<T> > 0)
inline constexpr std::size_t serialSize<std::array<T, size>> = serialSize<T> * size;

template<std::size_t capacity>
inline constexpr std::size_t serialSize<etl::string<capacity>> = capacity;

namespace internal
{
template<typename... Ts>
    requires((serialSize<Ts> != 0) and ...)
inline constexpr std::size_t totalSerialSizeHelper = (serialSize<Ts> + ...);

template<std::size_t... nBits>
inline constexpr std::size_t totalSerialSizeHelper<UInt<nBits>...> =
    ((nBits + ...) + CHAR_BIT - 1) / CHAR_BIT;  // Round up to the next byte
}

// Prefer using totalSerialSize<> over serialSize<> whenever possible since it ensures that the size
// is not 0
template<typename... Ts>
inline constexpr auto totalSerialSize = internal::totalSerialSizeHelper<std::remove_cvref_t<Ts>...>;

inline constexpr auto defaultEndianness = std::endian::little;


template<typename T>
    requires(serialSize<T> != 0)
using SerialBuffer = std::array<Byte, serialSize<T>>;

template<typename T>
    requires(serialSize<T> != 0)
using SerialBufferView = std::span<Byte const, serialSize<T>>;


template<typename T>
[[nodiscard]] auto Serialize(T const & t) -> SerialBuffer<T>;

template<std::endian endianness, typename T>
[[nodiscard]] auto Serialize(T const & t) -> SerialBuffer<T>;

template<std::default_initializable T>
[[nodiscard]] auto Deserialize(SerialBufferView<T> bufferView) -> T;

template<std::endian endianness, std::default_initializable T>
[[nodiscard]] auto Deserialize(SerialBufferView<T> bufferView) -> T;

// Must be overloaded for user-defined types to be serializable
template<std::endian endianness, TriviallySerializable T>
[[nodiscard]] auto SerializeTo(void * destination, T const & t) -> void *;

// Serializing UInt<>s is only implemented for big endian with MSB first
template<std::endian endianness, std::size_t... nBits>
    requires(endianness == std::endian::big and ((nBits + ...) % CHAR_BIT) == 0)
[[nodiscard]] auto SerializeTo(void * destination, UInt<nBits>... uInts) -> void *;

template<std::endian endianness, typename T>
    requires(strong::is_strong_type<T>::value)
[[nodiscard]] auto SerializeTo(void * destination, T const & t) -> void *;

template<std::endian endianness, typename T, std::size_t size>
[[nodiscard]] auto SerializeTo(void * destination, std::array<T, size> const & array) -> void *;

template<std::endian endianness, typename T>
[[nodiscard]] auto SerializeTo(void * destination, etl::ivector<T> const & vector) -> void *;

template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, etl::istring const & string) -> void *;

// Must be overloaded for user-defined types to be deserializable
template<std::endian endianness, TriviallySerializable T>
[[nodiscard]] auto DeserializeFrom(void const * source, T * t) -> void const *;

// Deserializing UInt<>s is only implemented for big endian with MSB first
template<std::endian endianness, std::size_t... nBits>
    requires(endianness == std::endian::big and ((nBits + ...) % CHAR_BIT) == 0)
[[nodiscard]] auto DeserializeFrom(void const * source, UInt<nBits> *... uInts) -> void const *;

template<std::endian endianness, typename T>
    requires(strong::is_strong_type<T>::value)
[[nodiscard]] auto DeserializeFrom(void const * source, T * t) -> void const *;

template<std::endian endianness, typename T, std::size_t size>
[[nodiscard]] auto DeserializeFrom(void const * source, std::array<T, size> * array)
    -> void const *;

template<std::endian endianness, typename T>
[[nodiscard]] auto DeserializeFrom(void const * source, etl::ivector<T> * vector) -> void const *;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, etl::istring * string) -> void const *;

template<HasEndianness T>
[[nodiscard]] constexpr auto ReverseBytes(T t) -> T;
}


#include <Sts1CobcSw/Serial/Serial.ipp>

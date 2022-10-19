#include <type_safe/types.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <type_traits>


namespace sts1cobcsw::serialize
{
// TODO: Tell clang-format to make this less ugly somehow
// The T::integer_type is for the type_safe fixed-width integers
template<typename T>
concept TriviallySerializable = std::is_arithmetic_v<T> or std::is_enum_v<T> or std::
    is_arithmetic_v<typename T::integer_type> or std::is_enum_v<
        typename T::integer_type> or std::is_same_v<T, type_safe::boolean>;


template<typename T>
inline constexpr std::size_t serialSize = 0U;

template<TriviallySerializable T>
inline constexpr std::size_t serialSize<T> = sizeof(T);



template<std::size_t size>
using ByteArray = std::array<std::byte, size>;

// SerialBuffer is only defined for types that specialize serialSize<>
template<typename T> requires (serialSize<T> != 0U)
using SerialBuffer = ByteArray<serialSize<T>>;


template<TriviallySerializable T>
constexpr auto SerializeTo(std::byte * destination, T t)
{
    std::memcpy(destination, &t, serialSize<T>);
    return destination + serialSize<T>;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}


template<TriviallySerializable T>
constexpr auto Serialize(T t)
{
    auto buffer = SerialBuffer<T>{};
    SerializeTo(buffer.data(), t);
    return buffer;
}
}
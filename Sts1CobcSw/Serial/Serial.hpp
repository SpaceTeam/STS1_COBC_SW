#include <type_safe/boolean.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <limits>
#include <type_traits>


// TODO: Enforce endianness with std::endian::native, std::endian::little, std::byteswap, etc.
namespace sts1cobcsw::serial
{
// The T::integer_type is for the type_safe fixed-width integers. The parenthesis are for nicer
// formatting.
template<typename T>
concept TriviallySerializable =
    (std::is_arithmetic_v<T> or std::is_enum_v<T>)
    or (std::is_arithmetic_v<typename T::integer_type> or std::is_enum_v<typename T::integer_type>)
    or std::is_same_v<T, type_safe::boolean>;


// Must be specialized for user-defined types to be serializable
template<typename T>
inline constexpr std::size_t serialSize = 0U;

template<TriviallySerializable T>
inline constexpr std::size_t serialSize<T> = sizeof(T);

template<typename... Ts>
constexpr std::size_t totalSerialSize = (serialSize<Ts> + ...);


// Allegedly std::byte is quite heavyweight. This type alias allows us to easily replace std::byte
// with, e.g., std::uint8_t to check that.
using Byte = std::byte;

template<typename T>
    requires(serialSize<T> != 0U)
using SerialBuffer = std::array<Byte, serialSize<T>>;


constexpr auto operator"" _B(unsigned long long number)  // NOLINT(google-runtime-int)
{
    return Byte(number);
}


// Must be specialized for user-defined types to be serializable
template<typename T>
constexpr auto SerializeTo(Byte * destination, T const & data) -> Byte *;


template<TriviallySerializable T>
constexpr auto SerializeTo(Byte * destination, T const & data) -> Byte *
{
    std::memcpy(destination, &data, serialSize<T>);
    return destination + serialSize<T>;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}


template<typename T>
constexpr auto Serialize(T const & data)
{
    auto buffer = SerialBuffer<T>{};
    SerializeTo(buffer.data(), data);
    return buffer;
}
}
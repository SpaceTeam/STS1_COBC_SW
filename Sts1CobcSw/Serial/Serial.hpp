#include <Sts1CobcSw/Utility/TypeSafe.hpp>

#include <type_safe/boolean.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <span>
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
inline constexpr std::size_t totalSerialSize = (serialSize<Ts> + ...);


// Allegedly std::byte is quite heavyweight. This type alias allows us to easily replace std::byte
// with, e.g., std::uint8_t to check that.
using Byte = std::byte;

template<typename T>
    requires(serialSize<T> != 0U)
using SerialBuffer = std::array<Byte, serialSize<T>>;


// Function declarations
// ---------------------

constexpr auto operator"" _B(unsigned long long number);  // NOLINT(google-runtime-int)

// Must be overloaded for user-defined types to be serializable
template<TriviallySerializable T>
constexpr auto SerializeTo(Byte * destination, T const & data) -> Byte *;

// Must be overloaded for user-defined types to be deserializable
template<TriviallySerializable T>
constexpr auto DeserializeFrom(Byte * source, T * data) -> Byte *;

template<typename T>
constexpr auto Serialize(T const & data);

template<std::default_initializable T>
constexpr auto Deserialize(std::span<Byte, serialSize<T>> source);

template<utility::TypeSafeInteger T>
constexpr auto Deserialize(std::span<Byte, serialSize<T>> source);

template<typename T>
    requires std::is_same_v<T, type_safe::boolean>
constexpr auto Deserialize(std::span<Byte, serialSize<T>> source);


// Function template definitions
// -----------------------------

inline constexpr auto operator"" _B(unsigned long long number)  // NOLINT(google-runtime-int)
{
    return Byte(number);
}


template<TriviallySerializable T>
inline constexpr auto SerializeTo(Byte * destination, T const & data) -> Byte *
{
    std::memcpy(destination, &data, serialSize<T>);
    return destination + serialSize<T>;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}


template<TriviallySerializable T>
inline constexpr auto DeserializeFrom(Byte * source, T * data) -> Byte *
{
    // The cast to void * suppresses the -Wclass-memaccess warning
    // https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Dialect-Options.html#index-Wclass-memaccess
    std::memcpy(static_cast<void *>(data), source, serialSize<T>);
    return source + serialSize<T>;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}


template<typename T>
constexpr auto Serialize(T const & data)
{
    auto buffer = SerialBuffer<T>{};
    SerializeTo(buffer.data(), data);
    return buffer;
}


template<std::default_initializable T>
constexpr auto Deserialize(std::span<Byte, serialSize<T>> source)
{
    auto t = T{};
    DeserializeFrom(source.data(), &t);
    return t;
}


template<utility::TypeSafeInteger T>
constexpr auto Deserialize(std::span<Byte, serialSize<T>> source)
{
    auto t = utility::TypeSafeZero<T>();
    DeserializeFrom(source.data(), &t);
    return t;
}


template<typename T>
    requires std::is_same_v<T, type_safe::boolean>
constexpr auto Deserialize(std::span<Byte, serialSize<T>> source)
{
    auto t = T{false};  // NOLINT(bugprone-argument-comment)
    DeserializeFrom(source.data(), &t);
    return t;
}
}

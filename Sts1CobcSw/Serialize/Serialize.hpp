#include <type_safe/boolean.hpp>

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


constexpr auto SerialSize(TriviallySerializable auto data)
{
    return sizeof(data);
}


template<std::size_t size>
using ByteArray = std::array<std::byte, size>;

template<typename T>
using SerialBuffer = ByteArray<SerialSize(T())>;


template<TriviallySerializable T>
constexpr auto SerializeTo(std::byte * destination, T t)
{
    std::memcpy(destination, &t, SerialSize(t));
    return destination + SerialSize(t);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}


template<TriviallySerializable T>
constexpr auto Serialize(T t)
{
    auto buffer = SerialBuffer<T>{};
    SerializeTo(buffer.data(), t);
    return buffer;
}
}
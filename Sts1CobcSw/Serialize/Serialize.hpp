#include <array>
#include <concepts>
#include <cstring>
#include <type_traits>


namespace sts1cobcsw::serialize
{
template<typename T>
concept TriviallySerializable = std::is_arithmetic<T>::value or std::is_enum<T>::value;


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
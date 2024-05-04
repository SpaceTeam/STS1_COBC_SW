#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>


namespace sts1cobcsw
{
template<typename T>
inline auto Serialize(T const & t) -> Buffer<T>
{
    return Serialize<defaultEndianness>(t);
}


template<std::endian endianness, typename T>
inline auto Serialize(T const & t) -> Buffer<T>
{
    auto buffer = Buffer<T>{};
    (void)SerializeTo<endianness>(buffer.data(), t);
    return buffer;
}


template<std::default_initializable T>
inline auto Deserialize(BufferView<T> bufferView) -> T
{
    return Deserialize<defaultEndianness, T>(bufferView);
}


template<std::endian endianness, std::default_initializable T>
inline auto Deserialize(BufferView<T> bufferView) -> T
{
    auto t = T{};
    (void)DeserializeFrom<endianness>(bufferView.data(), &t);
    return t;
}


template<std::endian endianness, TriviallySerializable T>
inline auto SerializeTo(void * destination, T const & t) -> void *
{
    if constexpr(HasEndianness<T> and endianness != std::endian::native)
    {
        auto data = ReverseBytes(t);
        std::memcpy(destination, &data, serialSize<T>);
    }
    else
    {
        std::memcpy(destination, &t, serialSize<T>);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<Byte *>(destination) + serialSize<T>;
}


template<std::endian endianness, typename T, std::size_t size>
auto SerializeTo(void * destination, std::array<T, size> const & array) -> void *
{
    for(auto const & element : array)
    {
        destination = SerializeTo<endianness>(destination, element);
    }
    return destination;
}


template<std::endian endianness, TriviallySerializable T>
inline auto DeserializeFrom(void const * source, T * t) -> void const *
{
    std::memcpy(t, source, serialSize<T>);
    if constexpr(HasEndianness<T> and endianness != std::endian::native)
    {
        *t = ReverseBytes(*t);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<Byte const *>(source) + serialSize<T>;
}


template<std::endian endianness, typename T, std::size_t size>
auto DeserializeFrom(void const * source, std::array<T, size> * array) -> void const *
{
    for(auto & element : *array)
    {
        source = DeserializeFrom<endianness>(source, &element);
    }
    return source;
}


template<HasEndianness T>
inline constexpr auto ReverseBytes(T t) -> T
{
    if constexpr(sizeof(T) == 1)
    {
        return t;
    }
    else if constexpr(std::integral<T>)
    {
        return etl::byteswap(t);
    }
    else if constexpr(std::is_enum_v<T>)
    {
        return static_cast<T>(etl::byteswap(static_cast<std::underlying_type<T>>(t)));
    }
}
}

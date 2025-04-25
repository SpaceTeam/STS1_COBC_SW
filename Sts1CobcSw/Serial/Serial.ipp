#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>


namespace sts1cobcsw
{
template<typename T>
inline auto Serialize(T const & t) -> SerialBuffer<T>
{
    return Serialize<defaultEndianness>(t);
}


template<std::endian endianness, typename T>
inline auto Serialize(T const & t) -> SerialBuffer<T>
{
    auto buffer = SerialBuffer<T>{};
    (void)SerializeTo<endianness>(buffer.data(), t);
    return buffer;
}


template<std::default_initializable T>
inline auto Deserialize(SerialBufferView<T> bufferView) -> T
{
    return Deserialize<defaultEndianness, T>(bufferView);
}


template<std::endian endianness, std::default_initializable T>
inline auto Deserialize(SerialBufferView<T> bufferView) -> T
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
        std::memcpy(destination, &data, totalSerialSize<T>);
    }
    else
    {
        std::memcpy(destination, &t, totalSerialSize<T>);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<Byte *>(destination) + totalSerialSize<T>;
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


template<std::endian endianness, std::size_t... nBits>
    requires(endianness == std::endian::big and ((nBits + ...) % CHAR_BIT) == 0)
[[nodiscard]] auto SerializeTo(void * destination, UInt<nBits>... uInts) -> void *
{
    static constexpr auto totalNBits = (nBits + ...);
    using Buffer = SmallestUnsignedType<totalNBits>;
    auto buffer = Buffer{};
    auto insertIntoBuffer = [&buffer](auto uInt)
    {
        buffer <<= uInt.size;
        buffer |= uInt.ToUnderlying();
    };
    (insertIntoBuffer(uInts), ...);
    buffer <<= (std::numeric_limits<Buffer>::digits - totalNBits);
    static constexpr auto totalNBytes = totalNBits / CHAR_BIT;
    // We cannot directly SerializeTo() because totalNBytes <= serialSize<Buffer>
    std::memcpy(destination, Serialize<std::endian::big>(buffer).data(), totalNBytes);
    return static_cast<Byte *>(destination) + totalNBytes;  // NOLINT(*pointer-arithmetic)
}


template<std::endian endianness, TriviallySerializable T>
inline auto DeserializeFrom(void const * source, T * t) -> void const *
{
    std::memcpy(t, source, totalSerialSize<T>);
    if constexpr(HasEndianness<T> and endianness != std::endian::native)
    {
        *t = ReverseBytes(*t);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<Byte const *>(source) + totalSerialSize<T>;
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


template<std::endian endianness, std::size_t... nBits>
    requires(endianness == std::endian::big and ((nBits + ...) % CHAR_BIT) == 0)
[[nodiscard]] auto DeserializeFrom(void const * source, UInt<nBits> *... uInts) -> void const *
{
    static constexpr auto totalNBits = (nBits + ...);
    static constexpr auto totalNBytes = totalNBits / CHAR_BIT;
    using Buffer = SmallestUnsignedType<totalNBits>;
    auto serializedBuffer = SerialBuffer<Buffer>{};
    std::memcpy(serializedBuffer.data(), source, totalNBytes);
    auto buffer = Deserialize<endianness, Buffer>(serializedBuffer);
    auto extractFromBuffer = [&buffer](auto * uInt)
    {
        *uInt = static_cast<typename std::remove_pointer_t<decltype(uInt)>::UnderlyingType>(
            buffer >> (std::numeric_limits<Buffer>::digits - uInt->size));
        buffer <<= uInt->size;
    };
    (extractFromBuffer(uInts), ...);
    return static_cast<Byte const *>(source) + totalNBytes;  // NOLINT(*pointer-arithmetic)
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

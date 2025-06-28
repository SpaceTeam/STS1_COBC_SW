#pragma once


#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
struct Parameter
{
    enum class Id : std::uint8_t
    {
        rxDataRate = 1,
        txDataRate,
        realTimeOffsetCorrection,
        eduStartDelayLimit,
        newEduResultIsAvailable,
    };
    using Value = std::uint32_t;

    Id id;
    Value value;
};


template<>
inline constexpr std::size_t serialSize<Parameter> =
    totalSerialSize<Parameter::Id, Parameter::Value>;


struct FileSystemObject
{
    enum class Type : std::uint8_t
    {
        file = 0,
        directory = 1,
    };

    Type type;
    fs::Path name;
};


template<>
inline constexpr std::size_t serialSize<FileSystemObject> =
    totalSerialSize<decltype(FileSystemObject::type), decltype(FileSystemObject::name)>;


constexpr auto IsValid(Parameter::Id parameterId) -> bool;

template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, Parameter const & parameter) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, Parameter * parameter) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, FileSystemObject const & object) -> void *;
}


#include <Sts1CobcSw/RfProtocols/Vocabulary.ipp>  // IWYU pragma: keep

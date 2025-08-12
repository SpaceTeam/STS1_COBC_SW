#pragma once

#include <Sts1CobcSw/Bootloader/PersistentVariables.hpp>

#include <Sts1CobcSw/Serial/Serial.hpp>


namespace sts1cobcsw
{
namespace internal
{
namespace
{
constexpr std::uint32_t section1StartAddress = 0;
constexpr std::uint32_t sectionSize = 100;
constexpr std::uint32_t section2StartAddress = section1StartAddress + sectionSize;
constexpr std::uint32_t section3StartAddress = section2StartAddress + sectionSize;


template<typename T>
[[nodiscard]] constexpr auto ComputeMajorityVote(T const & value0,
                                                 T const & value1,
                                                 T const & value2) -> T;
}
}


template<typename T>
auto Load(PersistentVariable<T> variable) -> T
{
    auto address1 = internal::section1StartAddress + variable.offset;
    auto address2 = internal::section2StartAddress + variable.offset;
    auto address3 = internal::section3StartAddress + variable.offset;
    auto value1 = Deserialize<T>(fram::Read<totalSerialSize<T>>(address1));
    auto value2 = Deserialize<T>(fram::Read<totalSerialSize<T>>(address2));
    auto value3 = Deserialize<T>(fram::Read<totalSerialSize<T>>(address3));
    auto value = internal::ComputeMajorityVote(value1, value2, value3);
    if(not(value1 == value2 && value2 == value3))
    {
        Store(variable, value);
    }
    return value;
}


template<typename T>
auto Store(PersistentVariable<T> variable, T value) -> void
{
    auto address1 = internal::section1StartAddress + variable.offset;
    auto address2 = internal::section2StartAddress + variable.offset;
    auto address3 = internal::section3StartAddress + variable.offset;
    fram::Write(address1, std::span<Byte const>(Serialize(value)));
    fram::Write(address2, std::span<Byte const>(Serialize(value)));
    fram::Write(address3, std::span<Byte const>(Serialize(value)));
}


namespace internal
{
namespace
{
template<typename T>
constexpr auto ComputeMajorityVote(T const & value0, T const & value1, T const & value2) -> T
{
    if(value0 == value1 || value0 == value2)
    {
        return value0;
    }
    if(value1 == value2)
    {
        return value1;
    }
    return value0;  // If all values are different, returning the first one is as good as any
}
}
}
}

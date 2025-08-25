#pragma once

#include <Sts1CobcSw/Bootloader/PersistentVariables.hpp>

#include <Sts1CobcSw/ErrorDetectionAndCorrection/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
namespace internal
{
constexpr std::uint32_t section0StartAddress = 0;
constexpr std::uint32_t sectionSize = 100;
constexpr std::uint32_t section1StartAddress = section0StartAddress + sectionSize;
constexpr std::uint32_t section2StartAddress = section1StartAddress + sectionSize;
}


template<typename T>
auto Load(PersistentVariable<T> variable) -> T
{
    auto address0 = internal::section0StartAddress + variable.offset;
    auto address1 = internal::section1StartAddress + variable.offset;
    auto address2 = internal::section2StartAddress + variable.offset;
    auto data0 = fram::Read<totalSerialSize<T>>(address0);
    auto data1 = fram::Read<totalSerialSize<T>>(address1);
    auto data2 = fram::Read<totalSerialSize<T>>(address2);
    auto value = Deserialize<T>(ComputeBitwiseMajorityVote(Span(data0), Span(data1), Span(data2)));
    Store(variable, value);
    return value;
}


template<typename T>
auto Store(PersistentVariable<T> variable, T value) -> void
{
    auto address0 = internal::section0StartAddress + variable.offset;
    auto address1 = internal::section1StartAddress + variable.offset;
    auto address2 = internal::section2StartAddress + variable.offset;
    fram::Write(address0, std::span<Byte const>(Serialize(value)));
    fram::Write(address1, std::span<Byte const>(Serialize(value)));
    fram::Write(address2, std::span<Byte const>(Serialize(value)));
}
}

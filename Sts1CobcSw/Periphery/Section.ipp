#pragma once


#include <Sts1CobcSw/Periphery/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>


namespace sts1cobcsw::fram
{
template<Size size>
inline constexpr auto FirstSection() -> Section<Address(0), size>
{
    static_assert(size <= memorySize, "Section does not fit in memory");
    return {};
}


template<Size newSize, Address begin, Size size>
inline constexpr auto NextSection(Section<begin, size> previousSection)
    -> Section<decltype(previousSection)::end, newSize>
{
    static_assert(newSize <= memorySize, "Section does not fit in memory");
    static_assert(value_of(previousSection.end) <= value_of(memorySize) - value_of(newSize), "Section does not fit in memory");
    return {};
}


template<Address begin, Size size>
inline constexpr auto LastSection(Section<begin, size> previousSection)
    -> Section<decltype(previousSection)::end, memorySize - decltype(previousSection)::end>
{
    static_assert(value_of(previousSection.end) < value_of(memorySize), "No space left for last section");
    return {};
}
}

#pragma once


#include <Sts1CobcSw/Periphery/Section.hpp>


namespace sts1cobcsw::fram
{
template<Size size>
inline constexpr auto CreateFirstSection() -> Section<0, size>
{
    static_assert(size <= memorySize, "Section does not fit in memory");
    return {};
}


template<Size newSize, Address begin, Size size>
inline constexpr auto CreateNextSection(Section<begin, size> previousSection)
    -> Section<previousSection.end, newSize>
{
    static_assert(newSize <= memorySize, "Section does not fit in memory");
    static_assert(previousSection.end <= memorySize - newSize, "Section does not fit in memory");
    return {};
}


template<Address begin, Size size>
inline constexpr auto CreateLastSection(Section<begin, size> previousSection)
    -> Section<previousSection.end, memorySize - previousSection.end>
{
    static_assert(previousSection.end < memorySize, "No space left for last section");
    return {};
}
}

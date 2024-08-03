#pragma once


#include <Sts1CobcSw/Periphery/Section.hpp>


namespace sts1cobcsw::fram
{
template<Size size>
constexpr auto FirstSection() -> Section<memoryBegin, size>
{
    static_assert(size <= memorySize, "Section does not fit in memory");
    return {};
}


template<Size newSize, Address begin, Size size>
constexpr auto NextSection(Section<begin, size> previousSection)
    -> Section<decltype(previousSection)::end, newSize>
{
    static_assert(newSize <= memorySize, "Section does not fit in memory");
    static_assert(previousSection.end <= memoryEnd - newSize, "Section does not fit in memory");
    return {};
}


template<Address begin, Size size>
constexpr auto LastSection(Section<begin, size> previousSection)
    -> Section<decltype(previousSection)::end, memoryEnd - decltype(previousSection)::end>
{
    static_assert(previousSection.end < memoryEnd, "No space left for last section");
    return {};
}
}

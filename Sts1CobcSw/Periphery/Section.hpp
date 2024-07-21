#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>


namespace sts1cobcsw::fram
{
template<Address sectionBegin, Size sectionSize>
struct Section
{
    static constexpr auto begin = sectionBegin;
    static constexpr auto size = sectionSize;
    static constexpr auto end = begin + size;
};


template<Size size>
inline constexpr auto FirstSection() -> Section<memoryBegin, size>;

template<Size newSize, Address begin, Size size>
inline constexpr auto NextSection(Section<begin, size> previousSection)
    -> Section<decltype(previousSection)::end, newSize>;

template<Address begin, Size size>
inline constexpr auto LastSection(Section<begin, size> previousSection)
    -> Section<decltype(previousSection)::end, memoryEnd - decltype(previousSection)::end>;
}


#include <Sts1CobcSw/Periphery/Section.ipp>  // IWYU pragma: keep

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
inline constexpr auto CreateFirstSection() -> Section<0, size>;

template<Size newSize, Address begin, Size size>
inline constexpr auto CreateNextSection(Section<begin, size> previousSection)
    -> Section<previousSection.end, newSize>;

template<Address begin, Size size>
inline constexpr auto CreateLastSection(Section<begin, size> previousSection)
    -> Section<previousSection.end, memorySize - previousSection.end>;
}


#include <Sts1CobcSw/Periphery/Section.ipp>  // IWYU pragma: keep

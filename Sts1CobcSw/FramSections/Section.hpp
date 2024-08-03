#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>


namespace sts1cobcsw::fram
{
// TODO: Consider renaming Section to AddressRange, Subsection to AddressSubrange, ...
template<Address sectionBegin, Size sectionSize>
    requires(sectionSize > 0)
struct Section
{
    static constexpr auto begin = sectionBegin;
    static constexpr auto size = sectionSize;
    static constexpr auto end = begin + size;
};


namespace internal
{
template<typename T>
inline constexpr bool isASectionHelper = false;

template<Address sectionBegin, Size sectionSize>
inline constexpr bool isASectionHelper<Section<sectionBegin, sectionSize>> = true;
}


template<typename T>
inline constexpr bool isASection = internal::isASectionHelper<std::remove_cvref_t<T>>;


template<Size size>
[[nodiscard]] constexpr auto FirstSection() -> Section<memoryBegin, size>;

template<Size newSize, Address begin, Size size>
[[nodiscard]] constexpr auto NextSection(Section<begin, size> previousSection)
    -> Section<decltype(previousSection)::end, newSize>;

template<Address begin, Size size>
[[nodiscard]] constexpr auto LastSection(Section<begin, size> previousSection)
    -> Section<decltype(previousSection)::end, memoryEnd - decltype(previousSection)::end>;
}


#include <Sts1CobcSw/FramSections/Section.ipp>  // IWYU pragma: keep

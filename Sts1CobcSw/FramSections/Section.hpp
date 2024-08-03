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
}

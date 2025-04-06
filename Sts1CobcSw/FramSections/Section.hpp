#pragma once


#include <Sts1CobcSw/Fram/Fram.hpp>


namespace sts1cobcsw
{
// TODO: Consider renaming Section to AddressRange, Subsection to AddressSubrange, ...
template<fram::Address sectionBegin, fram::Size sectionSize>
    requires(sectionSize > 0)
struct Section
{
    static constexpr auto begin = sectionBegin;
    static constexpr auto size = sectionSize;
    static constexpr auto end = begin + size;
};
}

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
}

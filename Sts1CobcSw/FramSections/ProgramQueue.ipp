#pragma once

#include <Sts1CobcSw/FramSections/ProgramQueue.hpp>

namespace sts1cobcsw
{
using fram::framIsWorking;

template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, ringArraySection, nCachedElements>::Size() -> SizeType
{
    return 0u;
}

template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, ringArraySection, nCachedElements>::Full() -> bool
{
    return false;
}

template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, ringArraySection, nCachedElements>::Empty() -> bool
{
    return true;
}

template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, ringArraySection, nCachedElements>::PushBack(T const & t) -> bool
{
    // TODO: Scope protector, make all this thread safe
    auto address = ringArraySection.begin + iBegin * elementSize;
    fram::WriteTo(address, Span(Serialize(t)), value_of(spiTimeout));
    size++;

    return true;
}




}

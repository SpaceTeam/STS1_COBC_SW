#pragma once


#include <Sts1CobcSw/FramSections/RingArray.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
using fram::framIsWorking;


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
inline constexpr auto RingArray<T, ringArraySection, nCachedElements>::FramCapacity() -> std::size_t
{
    return framCapacity;
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
inline constexpr auto RingArray<T, ringArraySection, nCachedElements>::CacheCapacity()
    -> std::size_t
{
    return nCachedElements;
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::Size() -> std::size_t
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        return cache.size();
    }
    LoadIndexes();
    return ComputeSize();
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::Get(std::size_t index) -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto size = []()
    {
        if(framIsWorking.Load())
        {
            LoadIndexes();
            return ComputeSize();
        }
        return cache.size();
    }();
    if(index >= size)
    {
        DEBUG_PRINT("Index out of bounds in RingArray::Get: %u >= %u\n", index, size);
        index = size - 1;
    }
    if(not framIsWorking.Load())
    {
        return Deserialize<T>(cache[index]);
    }
    auto i = iBegin;
    i.advance(static_cast<int>(index));
    return ReadElement(i);
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::Front() -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        return Deserialize<T>(cache.front());
    }
    LoadIndexes();
    return ReadElement(iBegin);
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::Back() -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        return Deserialize<T>(cache.back());
    }
    LoadIndexes();
    auto i = iEnd;
    i--;
    return ReadElement(i);
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::Set(std::size_t index, T const & t) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto size = []()
    {
        if(framIsWorking.Load())
        {
            LoadIndexes();
            return ComputeSize();
        }
        return cache.size();
    }();
    if(index >= size)
    {
        DEBUG_PRINT("Index out of bounds in RingArray::Set: %u >= %u\n", index, size);
        return;
    }
    if(not framIsWorking.Load())
    {
        cache[index] = Serialize(t);
    }
    auto i = iBegin;
    i.advance(static_cast<int>(index));
    WriteElement(i, t);
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::PushBack(T const & t) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        cache.push(Serialize(t));
        return;
    }
    LoadIndexes();
    WriteElement(iEnd, t);
    // We reduce the capacity by one to distinguish between an empty and a full ring: iEnd == iBegin
    // means empty, iEnd == iBegin - 1 means full
    iEnd++;
    if(iEnd == iBegin)
    {
        iBegin++;
    }
    StoreIndexes();
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
typename RingArray<T, ringArraySection, nCachedElements>::RingIndex
    RingArray<T, ringArraySection, nCachedElements>::iEnd = {};


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
typename RingArray<T, ringArraySection, nCachedElements>::RingIndex
    RingArray<T, ringArraySection, nCachedElements>::iBegin = {};


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
etl::circular_buffer<SerialBuffer<T>,
                     nCachedElements> RingArray<T, ringArraySection, nCachedElements>::cache = {};


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
RODOS::Semaphore RingArray<T, ringArraySection, nCachedElements>::semaphore = {};


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::LoadIndexes() -> void
{
    iBegin.set(persistentIndexes.template Load<"iBegin">());
    iEnd.set(persistentIndexes.template Load<"iEnd">());
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::StoreIndexes() -> void
{
    persistentIndexes.template Store<"iBegin">(iBegin.get());
    persistentIndexes.template Store<"iEnd">(iEnd.get());
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::ComputeSize() -> std::size_t
{
    if(iEnd.get() >= iBegin.get())
    {
        return iEnd.get() - iBegin.get();
    }
    return framCapacity + 1 + iEnd.get() - iBegin.get();
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::ReadElement(RingIndex index) -> T
{
    auto address = subsections.template Get<"array">().begin + index.get() * elementSize;
    return Deserialize<T>(fram::ReadFrom<serialSize<T>>(address, value_of(spiTimeout)));
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::WriteElement(RingIndex index, T const & t)
    -> void
{
    auto address = subsections.template Get<"array">().begin + index.get() * elementSize;
    fram::WriteTo(address, Span(Serialize(t)), value_of(spiTimeout));
}
}

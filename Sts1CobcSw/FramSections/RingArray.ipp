#pragma once


#include <Sts1CobcSw/FramSections/RingArray.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
using fram::framIsWorking;


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
inline constexpr auto RingArray<T, ringArraySection, nCachedElements>::FramCapacity() -> SizeType
{
    return framCapacity;
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
inline constexpr auto RingArray<T, ringArraySection, nCachedElements>::CacheCapacity() -> SizeType
{
    return nCachedElements;
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::Size() -> SizeType
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    return DoSize();
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::Get(IndexType index) -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    return DoGet(index);
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
auto RingArray<T, ringArraySection, nCachedElements>::Set(IndexType index, T const & t) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    DoSet(index, t);
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::PushBack(T const & t) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    // We always write to the cache
    cache.push(Serialize(t));
    if(not framIsWorking.Load())
    {
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
auto RingArray<T, ringArraySection, nCachedElements>::FindAndReplace(
    std::predicate<T> auto predicate, T const & newData) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto size = DoSize();
    for(IndexType index = 0; index < size; ++index)
    {
        if(predicate(DoGet(index)))
        {
            DoSet(index, newData);
        }
    }
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::DoSize() -> SizeType
{
    if(framIsWorking.Load())
    {
        LoadIndexes();
        return FramSize();
    }
    return cache.size();
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::DoGet(IndexType index) -> T
{
    auto size = DoSize();
    if(index >= size)
    {
        DEBUG_PRINT("Index out of bounds in RingArray::Get(): %u >= %u\n",
                    static_cast<unsigned>(index),
                    static_cast<unsigned>(size));
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
auto RingArray<T, ringArraySection, nCachedElements>::DoSet(IndexType index, T const & t) -> void
{
    if(not framIsWorking.Load())
    {
        if(index >= cache.size())
        {
            DEBUG_PRINT("Index out of bounds for cache in RingArray::Set(): %d >= %d\n",
                        static_cast<int>(index),
                        static_cast<int>(cache.size()));
            return;
        }
        cache[index] = Serialize(t);
        return;
    }
    LoadIndexes();
    auto framSize = FramSize();
    if(index >= framSize)
    {
        DEBUG_PRINT("Index out of bounds in RingArray::Set(): %d >= %d\n",
                    static_cast<int>(index),
                    static_cast<int>(framSize));
        return;
    }
    auto i = iBegin;
    i.advance(static_cast<int>(index));
    WriteElement(i, t);
    // The cache should hold the latest elements, not the oldest ones, so we need to shift the index
    auto cacheIndex =
        static_cast<int>(index) - static_cast<int>(FramSize()) + static_cast<int>(cache.size());
    if(cacheIndex < 0)
    {
        DEBUG_PRINT("Index out of bounds for cache in RingArray::Set(): %d < 0\n",
                    static_cast<int>(cacheIndex));
        return;
    }
    // If the index is not out of bounds, we always write to the cache
    cache[static_cast<unsigned>(cacheIndex)] = Serialize(t);
}


// Load the begin and end indexes from the FRAM
template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::LoadIndexes() -> void
{
    iBegin.set(persistentIndexes.template Load<"iBegin">());
    iEnd.set(persistentIndexes.template Load<"iEnd">());
}


// Store the begin and end indexes on the FRAM
template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::StoreIndexes() -> void
{
    persistentIndexes.template Store<"iBegin">(iBegin.get());
    persistentIndexes.template Store<"iEnd">(iEnd.get());
}


// Compute the size of the FRAM ring buffer
template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::FramSize() -> SizeType
{
    if(iEnd.get() >= iBegin.get())
    {
        return iEnd.get() - iBegin.get();
    }
    return framCapacity + 1 + iEnd.get() - iBegin.get();
}


// Read an element from the FRAM
template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::ReadElement(RingIndex index) -> T
{
    auto address = subsections.template Get<"array">().begin + index.get() * elementSize;
    return Deserialize<T>(fram::ReadFrom<serialSize<T>>(address, value_of(spiTimeout)));
}


// Write an element to the FRAM
template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection, nCachedElements>::WriteElement(RingIndex index, T const & t)
    -> void
{
    auto address = subsections.template Get<"array">().begin + index.get() * elementSize;
    fram::WriteTo(address, Span(Serialize(t)), value_of(spiTimeout));
}
}

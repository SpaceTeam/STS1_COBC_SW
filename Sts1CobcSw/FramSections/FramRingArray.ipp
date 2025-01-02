#pragma once


#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
using fram::framIsWorking;


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
inline constexpr auto FramRingArray<T, framRingArraySection, nCachedElements>::FramCapacity()
    -> SizeType
{
    return framCapacity;
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
inline constexpr auto FramRingArray<T, framRingArraySection, nCachedElements>::CacheCapacity()
    -> SizeType
{
    return nCachedElements;
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::Size() -> SizeType
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(framIsWorking.Load())
    {
        return FramArraySize(LoadIndexes());
    }
    return static_cast<SizeType>(cache.size());
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::Get(IndexType index) -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(framIsWorking.Load())
    {
        return GetFromFram(index, LoadIndexes());
    }
    return GetFromCache(index);
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::Front() -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        return Deserialize<T>(cache.front());
    }
    auto indexes = LoadIndexes();
    return LoadElement(indexes.iBegin);
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::Back() -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        return Deserialize<T>(cache.back());
    }
    auto indexes = LoadIndexes();
    auto i = indexes.iEnd;
    i--;
    return LoadElement(i);
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::Set(IndexType index, T const & t)
    -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(framIsWorking.Load())
    {
        SetInFramAndCache(index, t, LoadIndexes());
    }
    else
    {
        SetInCache(index, t);
    }
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::PushBack(T const & t) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    // We always write to the cache
    cache.push(Serialize(t));
    if(not framIsWorking.Load())
    {
        return;
    }
    auto indexes = LoadIndexes();
    StoreElement(indexes.iEnd, t);
    // We reduce the capacity by one to distinguish between an empty and a full ring: iEnd == iBegin
    // means empty, iEnd == iBegin - 1 means full
    indexes.iEnd++;
    if(indexes.iEnd == indexes.iBegin)
    {
        indexes.iBegin++;
    }
    StoreIndexes(indexes);
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::FindAndReplace(
    std::predicate<T> auto predicate, T const & newData) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        for(auto & element : cache)
        {
            if(predicate(Deserialize<T>(element)))
            {
                element = Serialize(newData);
            }
        }
        return;
    }
    auto indexes = LoadIndexes();
    auto framArraySize = FramArraySize(indexes);
    for(IndexType index = 0; index < framArraySize; ++index)
    {
        if(predicate(GetFromFram(index, indexes)))
        {
            SetInFramAndCache(index, newData, indexes);
        }
    }
}


// Compute the size of the ring array on the FRAM
template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::FramArraySize(Indexes const & indexes)
    -> SizeType
{
    if(indexes.iEnd.get() >= indexes.iBegin.get())
    {
        return indexes.iEnd.get() - indexes.iBegin.get();
    }
    return framCapacity + 1 + indexes.iEnd.get() - indexes.iBegin.get();
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::GetFromCache(IndexType index) -> T
{
    auto size = cache.size();
    if(index >= size)
    {
        DEBUG_PRINT("Index out of bounds in FramRingArray::GetFromCache(): %d >= %d\n",
                    static_cast<int>(index),
                    static_cast<int>(size));
        index = size - 1;
    }
    return Deserialize<T>(cache[index]);
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::GetFromFram(IndexType index,
                                                                          Indexes const & indexes)
    -> T
{
    auto framArraySize = FramArraySize(indexes);
    if(index >= framArraySize)
    {
        DEBUG_PRINT("Index out of bounds in FramRingArray::GetFromFram(): %d >= %d\n",
                    static_cast<int>(index),
                    static_cast<int>(framArraySize));
        index = framArraySize - 1;
    }
    auto i = indexes.iBegin;
    i.advance(static_cast<int>(index));
    return LoadElement(i);
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::SetInCache(IndexType index,
                                                                         T const & t) -> void
{
    if(index >= cache.size())
    {
        DEBUG_PRINT("Index out of bounds in FramRingArray::SetInCache(): %d >= %d\n",
                    static_cast<int>(index),
                    static_cast<int>(cache.size()));
        return;
    }
    cache[index] = Serialize(t);
}


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::SetInFramAndCache(
    IndexType index, T const & t, Indexes const & indexes) -> void
{
    auto framArraySize = FramArraySize(indexes);
    if(index >= framArraySize)
    {
        DEBUG_PRINT("Index out of bounds in FramRingArray::SetInFramAndCache(): %d >= %d\n",
                    static_cast<int>(index),
                    static_cast<int>(framArraySize));
        return;
    }
    auto i = indexes.iBegin;
    i.advance(static_cast<int>(index));
    StoreElement(i, t);
    // The cache should hold the latest elements, not the oldest ones, so we need to shift the index
    auto cacheIndex =
        static_cast<int>(index) - static_cast<int>(framArraySize) + static_cast<int>(cache.size());
    if(cacheIndex < 0)
    {
        DEBUG_PRINT("Index out of bounds for cache in FramRingArray::SetInFramAndCache(): %d < 0\n",
                    static_cast<int>(cacheIndex));
        return;
    }
    // If the index is not out of bounds, we always write to the cache
    cache[static_cast<unsigned>(cacheIndex)] = Serialize(t);
}


// Load the begin and end indexes from the FRAM
template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::LoadIndexes() -> Indexes
{
    auto indexes = Indexes{};
    indexes.iBegin.set(persistentIndexes.template Load<"iBegin">());
    indexes.iEnd.set(persistentIndexes.template Load<"iEnd">());
    return indexes;
}


// Load an array element from the FRAM
template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::LoadElement(RingIndex index) -> T
{
    auto address = subsections.template Get<"array">().begin + index.get() * elementSize;
    return Deserialize<T>(fram::ReadFrom<serialSize<T>>(address, spiTimeout));
}


// Store the begin and end indexes on the FRAM
template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::StoreIndexes(Indexes const & indexes)
    -> void
{
    persistentIndexes.template Store<"iBegin">(indexes.iBegin.get());
    persistentIndexes.template Store<"iEnd">(indexes.iEnd.get());
}


// Store an array element on the FRAM
template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramRingArray<T, framRingArraySection, nCachedElements>::StoreElement(RingIndex index,
                                                                           T const & t) -> void
{
    auto address = subsections.template Get<"array">().begin + index.get() * elementSize;
    fram::WriteTo(address, Span(Serialize(t)), spiTimeout);
}
}

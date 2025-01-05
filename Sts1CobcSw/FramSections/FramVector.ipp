#pragma once


#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>

#include <rodos-semaphore.h>


namespace sts1cobcsw
{
using fram::framIsWorking;


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
constexpr auto FramVector<T, framVectorSection, nCachedElements>::FramCapacity() -> SizeType
{
    return framCapacity;
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
constexpr auto FramVector<T, framVectorSection, nCachedElements>::CacheCapacity() -> SizeType
{
    return nCachedElements;
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::Size() -> SizeType
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    return DoSize();
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::IsFull() -> bool
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(framIsWorking.Load())
    {
        return LoadSize() >= FramCapacity();
    }
    return cache.full();
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::IsEmpty() -> bool
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(framIsWorking.Load())
    {
        return LoadSize() == 0;
    }
    return cache.empty();
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::PushBack(T const & t) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    // We always write to the cache
    if(not cache.full())
    {
        cache.push_back(Serialize(t));
    }
    if(not framIsWorking.Load())
    {
        return;
    }
    auto size = LoadSize();
    if(size >= FramCapacity())
    {
        DEBUG_PRINT("FramVector is full. Cannot push back new element.\n");
        return;
    }
    StoreElement(size, t);
    size++;
    StoreSize(size);
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::Clear() -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    cache.clear();
    if(framIsWorking.Load())
    {
        StoreSize(0);
    }
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::Get(IndexType index) -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto size = DoSize();
    if(size == 0)
    {
        DEBUG_PRINT("Trying to get element from empty FramVector\n");
        return T{};
    }
    if(index >= size)
    {
        DEBUG_PRINT("Index out of bounds in FramVector::Get(): %d >= %d\n",
                    static_cast<int>(index),
                    static_cast<int>(size));
        index = size - 1;
    }
    if(framIsWorking.Load())
    {
        return LoadElement(index);
    }
    return Deserialize<T>(cache[index]);
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::DoSize() -> SizeType
{
    if(framIsWorking.Load())
    {
        return LoadSize();
    }
    return static_cast<SizeType>(cache.size());
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::LoadSize() -> SizeType
{
    return persistentMetadata.template Load<"size">();
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::LoadElement(IndexType index) -> T
{
    auto address = subsections.template Get<"array">().begin + index * elementSize;
    return Deserialize<T>(fram::ReadFrom<serialSize<T>>(address, spiTimeout));
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::StoreSize(SizeType size) -> void
{
    persistentMetadata.template Store<"size">(size);
}


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::StoreElement(IndexType index, T const & t)
    -> void
{
    auto address = subsections.template Get<"array">().begin + index * elementSize;
    fram::WriteTo(address, Span(Serialize(t)), spiTimeout);
}
}

#pragma once


#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>

#include <rodos-semaphore.h>


namespace sts1cobcsw
{
using fram::framIsWorking;


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
constexpr auto FramVector<T, framVectorSection, nCachedElements>::FramCapacity() -> SizeType
{
    return framCapacity;
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
constexpr auto FramVector<T, framVectorSection, nCachedElements>::CacheCapacity() -> SizeType
{
    return nCachedElements;
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::Size() -> SizeType
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        size = cache.size();
    }
    else
    {
        LoadSize();
    }
    return size;
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::Full() -> bool
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    return Size() >= FramCapacity();
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::Empty() -> bool
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    return Size() == 0;
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::PushBack(T const & t) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not cache.full())
    {
        cache.push_back(t);
    }
    if(framIsWorking.Load())
    {
        LoadSize();
        if(size >= FramCapacity())
        {
            DEBUG_PRINT("FramVector is full. Cannot push back new element.\n");
            return;
        }
        WriteElement(size, t);
        size++;
        StoreSize();
    }
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::Clear() -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(not framIsWorking.Load())
    {
        cache.clear();
    }
    else
    {
        size = 0;
        StoreSize();
    }
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::Get(IndexType index) -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto size = Size();
    if(index >= size)
    {
        DEBUG_PRINT("Index out of bounds in FramVector::Get(): %d >= %d\n",
                    static_cast<int>(index),
                    static_cast<int>(size));
        return T{};
    }
    if(not framIsWorking.Load())
    {
        return cache[index];
    }
    return ReadElement(index);
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::LoadSize() -> void
{
    size = persistentIndexes.template Load<"size">();
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::StoreSize() -> void
{
    persistentIndexes.template Store<"size">(size);
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::WriteElement(IndexType index, T const & t)
    -> void
{
    auto address = subsections.template Get<"array">().begin + index * elementSize;
    fram::WriteTo(address, Span(Serialize(t)), spiTimeout);
}


template<typename T, Section framVectorSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto FramVector<T, framVectorSection, nCachedElements>::ReadElement(IndexType index) -> T
{
    auto address = subsections.template Get<"array">().begin + index * elementSize;
    return Deserialize<T>(fram::ReadFrom<serialSize<T>>(address, spiTimeout));
}
}

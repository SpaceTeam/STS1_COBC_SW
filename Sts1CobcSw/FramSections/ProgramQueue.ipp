#pragma once


#include <Sts1CobcSw/FramSections/ProgramQueue.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>

#include <rodos-semaphore.h>


namespace sts1cobcsw
{
using fram::framIsWorking;


template<typename T, Section queueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
constexpr auto ProgramQueue<T, queueSection, nCachedElements>::FramCapacity() -> SizeType
{
    return framCapacity;
}


template<typename T, Section queueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
constexpr auto ProgramQueue<T, queueSection, nCachedElements>::CacheCapacity() -> SizeType
{
    return nCachedElements;
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, ringArraySection, nCachedElements>::Size() -> SizeType
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


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, ringArraySection, nCachedElements>::Full() -> bool
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    return Size() >= FramCapacity();
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, ringArraySection, nCachedElements>::Empty() -> bool
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    return Size() == 0;
}


template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, ringArraySection, nCachedElements>::PushBack(T const & t) -> void
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
            DEBUG_PRINT("[ProgramQueue] FRAM queue is full. Cannot push back new element.\n");
            return;
        }
        WriteElement(size, t);
        size++;
        StoreSize();
    }
}


template<typename T, Section queueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, queueSection, nCachedElements>::Clear() -> void
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


template<typename T, Section queueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, queueSection, nCachedElements>::Get(IndexType index) -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(index >= Size())
    {
        DEBUG_PRINT("[ProgramQueue] Index out of bounds.\n");
        return T{};
    }
    if(not framIsWorking.Load())
    {
        return cache[index];
    }
    return ReadElement(index);
}


template<typename T, Section queueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, queueSection, nCachedElements>::LoadSize() -> void
{
    size = persistentIndexes.template Load<"size">();
}


template<typename T, Section queueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, queueSection, nCachedElements>::StoreSize() -> void
{
    persistentIndexes.template Store<"size">(size);
}


template<typename T, Section queueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, queueSection, nCachedElements>::WriteElement(IndexType index, T const & t)
    -> void
{
    auto address = subsections.template Get<"array">().begin + index * elementSize;
    fram::WriteTo(address, Span(Serialize(t)), spiTimeout);
}


template<typename T, Section queueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
auto ProgramQueue<T, queueSection, nCachedElements>::ReadElement(IndexType index) -> T
{
    auto address = subsections.template Get<"array">().begin + index * elementSize;
    return Deserialize<T>(fram::ReadFrom<serialSize<T>>(address, spiTimeout));
}
}

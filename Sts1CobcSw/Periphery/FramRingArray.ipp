#pragma once


#include <Sts1CobcSw/Periphery/FramRingArray.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
typename RingArray<T, ringArraySection>::RingIndex RingArray<T, ringArraySection>::iEnd =
    RingIndex{};


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
typename RingArray<T, ringArraySection>::RingIndex RingArray<T, ringArraySection>::iBegin =
    RingIndex{};


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
inline constexpr auto RingArray<T, ringArraySection>::Capacity() -> std::size_t
{
    return capacity;
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::Size() -> std::size_t
{
    LoadIndexes();
    return ComputeSize();
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::Get(std::size_t index) -> T
{
    LoadIndexes();
    auto size = ComputeSize();
    if(index >= size)
    {
        DEBUG_PRINT("Index out of bounds in RingArray::Get: %u >= %u\n", index, size);
        index = size - 1;
    }
    auto i = iBegin;
    i.advance(static_cast<int>(index));
    return ReadElement(i);
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::Front() -> T
{
    LoadIndexes();
    return ReadElement(iBegin);
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::Back() -> T
{
    LoadIndexes();
    auto i = iEnd;
    i--;
    return ReadElement(i);
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::PushBack(T const & t) -> void
{
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


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::LoadIndexes() -> void
{
    iBegin.set(persistentIndexes.template Load<"iBegin">());
    iEnd.set(persistentIndexes.template Load<"iEnd">());
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::StoreIndexes() -> void
{
    persistentIndexes.template Store<"iBegin">(iBegin.get());
    persistentIndexes.template Store<"iEnd">(iEnd.get());
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::ComputeSize() -> std::size_t
{
    if(iEnd.get() >= iBegin.get())
    {
        return iEnd.get() - iBegin.get();
    }
    return capacity + 1 + iEnd.get() - iBegin.get();
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::ReadElement(RingIndex index) -> T
{
    auto address = subsections.template Get<"array">().begin + index.get() * elementSize;
    return Deserialize<T>(fram::ReadFrom<serialSize<T>>(address, value_of(spiTimeout)));
}


template<typename T, Section ringArraySection>
    requires(serialSize<T> > 0)
auto RingArray<T, ringArraySection>::WriteElement(RingIndex index, T const & t) -> void
{
    auto address = subsections.template Get<"array">().begin + index.get() * elementSize;
    fram::WriteTo(address, Span(Serialize(t)), value_of(spiTimeout));
}
}

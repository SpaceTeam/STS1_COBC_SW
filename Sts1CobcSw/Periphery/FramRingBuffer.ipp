#pragma once


#include <Sts1CobcSw/Periphery/FramRingBuffer.hpp>


namespace sts1cobcsw::fram
{
template<typename T, std::size_t size, Address startAddress>
void RingBuffer<T, size, startAddress>::Push(T const & newData)
{
    auto const rawAddress = offset_ + value_of(startAddress) + (iEnd_ * serialSize<T>);
    fram::WriteTo(fram::Address(rawAddress), Span(Serialize(newData)), 0);

    ++iEnd_;
    if(iEnd_ == iBegin_)
    {
        iBegin_++;
    }

    WriteIndices();
}


template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Front() -> T
{
    auto const rawAddress = offset_ + value_of(startAddress) + (iBegin_ * serialSize<T>);
    auto readData = fram::ReadFrom<serialSize<T>>(fram::Address(rawAddress), 0);
    auto fromRing = Deserialize<T>(std::span(readData));

    return fromRing;
}


template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Back() -> T
{
    std::uint32_t readIndex = 0;
    if(iEnd_ == 0)
    {
        readIndex = bufferSize_ - 1;
    }
    else
    {
        readIndex = iEnd_.get() - 1;
    }

    auto const rawAddress = offset_ + value_of(startAddress) + readIndex * serialSize<T>;
    auto readData = fram::ReadFrom<serialSize<T>>(fram::Address(rawAddress), 0);
    auto fromRing = Deserialize<T>(std::span(readData));

    return fromRing;
}


template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::operator[](std::size_t index) -> T
{
    auto const rawAddress =
        offset_ + value_of(startAddress) + ((iBegin_ + index) % bufferSize_) * serialSize<T>;
    auto readData = fram::ReadFrom<serialSize<T>>(fram::Address(rawAddress), 0);
    auto fromRing = Deserialize<T>(std::span(readData));

    return fromRing;
}


template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Size() -> std::size_t
{
    if(iEnd_ >= iBegin_)
    {
        return (iEnd_ - iBegin_);
    }
    return (bufferSize_ - (iBegin_ - iEnd_));
}

template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Capacity() -> std::size_t
{
    return (bufferSize_ - 1U);
}

template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Initialize() -> void
{
    ReadIndices();
}

template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::WriteIndices() -> void
{
    auto beginAddress = value_of(startAddress);
    auto endAddress = beginAddress + sizeof(std::size_t);

    fram::WriteTo(fram::Address(beginAddress), Span(Serialize(iBegin_.get())), 0);
    fram::WriteTo(fram::Address(endAddress), Span(Serialize(iEnd_.get())), 0);
}

template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::ReadIndices() -> void
{
    auto beginAddress = value_of(startAddress);
    auto endAddress = beginAddress + sizeof(std::size_t);

    auto beginData = fram::ReadFrom<sizeof(std::size_t)>(fram::Address(beginAddress), 0);
    auto endData = fram::ReadFrom<sizeof(std::size_t)>(fram::Address(endAddress), 0);

    iBegin_.set(Deserialize<std::size_t>(std::span(beginData)));
    iEnd_.set(Deserialize<std::size_t>(std::span(endData)));
}
}

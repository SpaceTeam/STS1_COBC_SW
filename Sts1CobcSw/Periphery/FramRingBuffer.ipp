#pragma once


#include <Sts1CobcSw/Periphery/FramRingBuffer.hpp>


namespace sts1cobcsw::fram
{
template<typename T, std::size_t size, Address startAddress>
void RingBuffer<T, size, startAddress>::Push(T const & newData)
{
    auto const rawaddress = value_of(startAddress) + (nextWriteIndex_ * serialSize<T>);
    fram::WriteTo(fram::Address(rawaddress), Span(Serialize(newData)), 0);

    ++nextWriteIndex_;
    if(nextWriteIndex_ == bufferSize_)
    {
        nextWriteIndex_ = 0;
    }

    if(nextWriteIndex_ == nextReadIndex_)
    {
        ++nextReadIndex_;
        if(nextReadIndex_ == bufferSize_)
        {
            nextReadIndex_ = 0;
        }
    }
}


template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Front() -> T
{
    auto const rawAddress = value_of(startAddress) + (nextReadIndex_ * serialSize<T>);
    auto readData = fram::ReadFrom<serialSize<T>>(fram::Address(rawAddress), 0);
    auto fromRing = Deserialize<T>(std::span(readData));

    return fromRing;
}


template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Back() -> T
{
    std::uint32_t readIndex = 0;
    if(nextWriteIndex_ == 0)
    {
        readIndex = bufferSize_ - 1;
    }
    else
    {
        readIndex = nextWriteIndex_ - 1;
    }

    auto const rawaddress = value_of(startAddress) + readIndex * serialSize<T>;
    auto readData = fram::ReadFrom<serialSize<T>>(fram::Address(rawaddress), 0);
    auto fromRing = Deserialize<T>(std::span(readData));

    return fromRing;
}


template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::operator[](std::size_t index) -> T
{
    auto const rawaddress =
        value_of(startAddress) + ((nextReadIndex_ + index) % bufferSize_) * serialSize<T>;
    auto readData = fram::ReadFrom<serialSize<T>>(fram::Address(rawaddress), 0);
    return Deserialize<T>(std::span(readData));
}


template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Size() -> std::size_t
{
    if(nextWriteIndex_ >= nextReadIndex_)
    {
        return (nextWriteIndex_ - nextReadIndex_);
    }
    return (bufferSize_ - (nextReadIndex_ - nextWriteIndex_));
}

template<typename T, std::size_t size, Address startAddress>
auto RingBuffer<T, size, startAddress>::Capacity() -> std::size_t
{
    return (bufferSize_ - 1U);
}
}

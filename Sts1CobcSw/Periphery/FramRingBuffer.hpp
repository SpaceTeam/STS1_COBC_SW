#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos-debug.h>

#include <etl/cyclic_value.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::fram
{
template<typename T, std::size_t size, Address startAddress>
class RingBuffer
{
public:
    RingBuffer() : bufferSize_(size + 1U)
    {
        nextWriteIndex_.set(0);
        nextReadIndex_.set(0);
    };

    auto Push(T const & newData) -> void;

    auto operator[](std::size_t index) -> T;

    auto Front() -> T;
    auto Back() -> T;

    //! @brief Returns the current size of the ringbuffer
    auto Size() -> std::size_t;

    //! @brief Returns the capacity of the ringbuffer
    auto Capacity() -> std::size_t;


private:
    std::size_t bufferSize_;
    etl::cyclic_value<std::size_t, 0, size> nextWriteIndex_;
    etl::cyclic_value<std::size_t, 0, size> nextReadIndex_;
};
}


#include <Sts1CobcSw/Periphery/FramRingBuffer.ipp>  // IWYU pragma: keep

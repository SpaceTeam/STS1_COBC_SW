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
class RingArray
{
public:
    RingArray() : bufferSize_(size + 1U), offset_(sizeof(std::size_t) * 2)
    {
        Initialize();
    };

    auto Push(T const & newData) -> void;

    auto operator[](std::size_t index) -> T;

    auto Front() -> T;
    auto Back() -> T;

    //! @brief Returns the current size of the ring array
    auto Size() -> std::size_t;

    //! @brief Returns the capacity of the ring array
    auto Capacity() -> std::size_t;

    // @brief Initializes the ring array by reading indices from FRAM
    auto Initialize() -> void;

private:
    std::size_t bufferSize_;
    std::size_t offset_;
    etl::cyclic_value<std::size_t, 0, size> iEnd_;
    etl::cyclic_value<std::size_t, 0, size> iBegin_;

    auto WriteIndices() -> void;
    auto ReadIndices() -> void;
};
}


#include <Sts1CobcSw/Periphery/FramRingArray.ipp>  // IWYU pragma: keep

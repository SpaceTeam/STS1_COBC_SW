#pragma once


#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <cstddef>
#include <cstdint>


namespace sts1cobcsw::hal
{
namespace empty
{
auto DoInitialize(std::uint32_t baudRate) -> void;
auto DoRead(void * data, std::size_t nBytes, Duration timeout) -> void;
auto DoWrite(void const * data, std::size_t nBytes, Duration timeout) -> void;
auto DoTransferEnd() -> RodosTime;
auto DoBaudRate() -> std::int32_t;
}


class Spi::Impl
{
public:
    void (*doInitialize)(std::uint32_t baudRate) = empty::DoInitialize;
    void (*doRead)(void * data, std::size_t nBytes, Duration timeout) = empty::DoRead;
    void (*doWrite)(void const * data, std::size_t nBytes, Duration timeout) = empty::DoWrite;
    RodosTime (*doTransferEnd)() = empty::DoTransferEnd;
    std::int32_t (*doBaudRate)() = empty::DoBaudRate;
};
}

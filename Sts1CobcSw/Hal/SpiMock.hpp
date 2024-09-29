#pragma once


#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::hal
{
class Spi
{
public:
    Spi() = delete;
    Spi(RODOS::SPI_IDX spiIndex,
        RODOS::GPIO_PIN sckPin,
        RODOS::GPIO_PIN misoPin,
        RODOS::GPIO_PIN mosiPin);

    // The following functions are friends and not members to keep the interface coherent with the
    // rest of the low-level code.
    friend auto Initialize(Spi * spi, std::uint32_t baudRate) -> void;
    template<typename T, std::size_t extent>
    friend auto ReadFrom(Spi * spi, std::span<T, extent> data, Duration timeout) -> void;
    template<typename T, std::size_t extent>
    friend auto WriteTo(Spi * spi, std::span<T const, extent> data, Duration timeout) -> void;

    [[nodiscard]] auto TransferEnd() const -> RodosTime;
    [[nodiscard]] auto BaudRate() const -> std::int32_t;


private:
    class Impl;
    Impl * pimpl_;

    auto Read(void * data, std::size_t nBytes, Duration timeout) -> void;
    auto Write(void const * data, std::size_t nBytes, Duration timeout) -> void;
};

auto SetDoInitialize(void (*doInitializeFunction)()) -> void;
auto SetDoRead(void (*doReadFunction)()) -> void;
auto SetDoWrite(void (*doWriteFunction)()) -> void;
auto SetDoTransferEnd(RodosTime (*doTransferEndFunction)()) -> void;
auto SetDoBaudRate(std::int32_t (*doBaudRateFunction)()) -> void;

namespace empty
{
auto SetAllDoFunctions() -> void;

auto DoInitialize(std::uint32_t baudRate) -> void;
auto DoRead(void * data, std::size_t nBytes, Duration timeout) -> void;
auto DoWrite(void const * data, std::size_t nBytes, Duration timeout) -> void;
auto DoTransferEnd() -> RodosTime;
auto DoBaudRate() -> std::int32_t;
}
}


#include <Sts1CobcSw/Hal/Spi.ipp>  // IWYU pragma: keep

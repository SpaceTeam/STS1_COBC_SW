#pragma once


#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::hal
{
// TODO: I used pImpl wrong. I used it to hide the private members of Spi to be able to include
// Spi.hpp in tests that should run on Linux (there is no RODOS::HAL_SPI there). What I really
// wanted to do though is to separate the interface from the implementation in a way that allows me
// to have a "real" implementation for the COBC target and a mock that I can use for tests. Since I
// want to test the SPI supervisor thread which requires the global SPI variables I cannot use
// compile-time polymorphism but really need runtime polymorphism. Therefore, I should have turned
// Spi into an abstract base class and implemented a "HwSpi" and an "SpiMock". Obviously we'll use
// the NVI idiom for the polymorphic SPI class.
class Spi
{
public:
    Spi() = default;
    Spi(Spi const &) = delete;
    Spi(Spi &&) = default;
    auto operator=(Spi const &) -> Spi & = delete;
    auto operator=(Spi &&) -> Spi & = default;
    virtual ~Spi() = default;

    // The following functions are friends and not members to keep the interface coherent with the
    // rest of the low-level code.
    friend auto Initialize(Spi * spi, std::uint32_t baudRate, bool useOpenDrainOutputs) -> void;
    template<typename T, std::size_t extent>
    friend auto ReadFrom(Spi * spi, std::span<T, extent> data, Duration timeout) -> void;
    template<typename T, std::size_t extent>
    friend auto WriteTo(Spi * spi, std::span<T const, extent> data, Duration timeout) -> void;

    [[nodiscard]] auto TransferEnd() const -> RodosTime;
    [[nodiscard]] auto BaudRate() const -> std::int32_t;


private:
    virtual auto DoInitialize(std::uint32_t baudRate, bool useOpenDrainOutputs) -> void = 0;
    virtual auto Read(void * data, std::size_t nBytes, Duration timeout) -> void = 0;
    virtual auto Write(void const * data, std::size_t nBytes, Duration timeout) -> void = 0;
    [[nodiscard]] virtual auto DoTransferEnd() const -> RodosTime = 0;
    [[nodiscard]] virtual auto DoBaudRate() const -> std::int32_t = 0;
};


auto Initialize(Spi * spi, std::uint32_t baudRate, bool useOpenDrainOutputs = false) -> void;
}


#include <Sts1CobcSw/Hal/Spi.ipp>  // IWYU pragma: keep

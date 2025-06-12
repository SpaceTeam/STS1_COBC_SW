#pragma once


#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>
#include <cstdint>
#include <utility>


namespace sts1cobcsw::hal
{
class HardwareSpi : public Spi
{
public:
    HardwareSpi(RODOS::SPI_IDX spiIndex,
                RODOS::GPIO_PIN sckPin,
                RODOS::GPIO_PIN misoPin,
                RODOS::GPIO_PIN mosiPin);
    HardwareSpi(HardwareSpi const &) = delete;
    HardwareSpi(HardwareSpi &&) = delete;
    auto operator=(HardwareSpi const &) -> HardwareSpi & = delete;
    auto operator=(HardwareSpi &&) -> HardwareSpi & = delete;
    ~HardwareSpi() override = default;


private:
    // Do not call this in the init() function of a thread. The semaphore doesn't work correctly
    // there, making other SPIs silently fail to get initialized afterwards.
    auto DoInitialize(std::uint32_t baudRate, bool useOpenDrainOutputs) -> void override;
    auto Read(void * data, std::size_t nBytes, Duration timeout) -> void override;
    auto Write(void const * data, std::size_t nBytes, Duration timeout) -> void override;
    [[nodiscard]] auto DoTransferEnd() const -> RodosTime override;
    [[nodiscard]] auto DoBaudRate() const -> std::int32_t override;

    mutable RODOS::HAL_SPI spi_;
    // This should actually be an atomic variable. Since they don't exist in Rodos, we protect it
    // with a semaphore instead.
    mutable RodosTime transferEnd_ = endOfTime;
    mutable RODOS::Semaphore transferEndSemaphore_;
};
}

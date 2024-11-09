#include <Sts1CobcSw/Hal/HardwareSpi.hpp>

#include <strong_type/affine_point.hpp>


namespace sts1cobcsw::hal
{
HardwareSpi::HardwareSpi(RODOS::SPI_IDX spiIndex,
                         RODOS::GPIO_PIN sckPin,
                         RODOS::GPIO_PIN misoPin,
                         RODOS::GPIO_PIN mosiPin)
    : spi_(spiIndex, sckPin, misoPin, mosiPin)
{
}


auto HardwareSpi::DoInitialize(std::uint32_t baudRate, bool useOpenDrainOutputs) -> void
{
    // spi.init() only returns -1 if the SPI_IDX is out of range. Since we can check that statically
    // we do not need to report that error at runtime.
    spi_.init(baudRate, /*slave=*/false, /*tiMode=*/false, useOpenDrainOutputs);
    auto protector = RODOS::ScopeProtector(&transferEndSemaphore_);
    transferEnd_ = endOfTime;
}


auto HardwareSpi::Read(void * data, std::size_t nBytes, Duration timeout) -> void
{
    // spi.read() only returns -1 or the given buffer length. It only returns -1 if the SPI is not
    // initialized, which we can check/ensure statically. Therefore, we do not need to check the
    // return value at runtime.
    {
        auto protector = RODOS::ScopeProtector(&transferEndSemaphore_);
        transferEnd_ = CurrentRodosTime() + timeout;
    }
    spi_.read(data, nBytes);
    {
        auto protector = RODOS::ScopeProtector(&transferEndSemaphore_);
        transferEnd_ = endOfTime;
    }
}


auto HardwareSpi::Write(void const * data, std::size_t nBytes, Duration timeout) -> void
{
    // spi.write() only returns -1 or the given buffer length. It only returns -1 if the SPI is not
    // initialized, which we can check/ensure statically. Therefore, we do not need to check the
    // return value at runtime.
    {
        auto protector = RODOS::ScopeProtector(&transferEndSemaphore_);
        transferEnd_ = CurrentRodosTime() + timeout;
    }
    spi_.write(data, nBytes);
    {
        auto protector = RODOS::ScopeProtector(&transferEndSemaphore_);
        transferEnd_ = endOfTime;
    }
}


auto HardwareSpi::DoTransferEnd() const -> RodosTime
{
    auto protector = RODOS::ScopeProtector(&transferEndSemaphore_);
    return transferEnd_;
}


auto HardwareSpi::DoBaudRate() const -> std::int32_t
{
    return spi_.status(RODOS::SPI_STATUS_BAUDRATE);
}
}

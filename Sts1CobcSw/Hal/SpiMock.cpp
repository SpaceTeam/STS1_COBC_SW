#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/SpiMock.hpp>

#include <strong_type/affine_point.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>


namespace sts1cobcsw::hal
{
class Spi::Impl
{
public:
    static constexpr auto maxNInstances = 3;

    // Since we cannot use dynamic memory allocation we use a static ETL vector to provide the
    // storage for a fixed number of instances. This limitation is acceptable since we only need one
    // global instance for each of the three SPIs that we use.
    [[nodiscard]] static auto GetInstances() -> etl::vector<Spi::Impl, maxNInstances> &;

    Impl(RODOS::SPI_IDX spiIndex,
         RODOS::GPIO_PIN sckPin,
         RODOS::GPIO_PIN misoPin,
         RODOS::GPIO_PIN mosiPin);

    auto Initialize(std::uint32_t baudRate) -> void;
    auto Read(void * data, std::size_t nBytes, Duration timeout) -> void;
    auto Write(void const * data, std::size_t nBytes, Duration timeout) -> void;
    [[nodiscard]] auto TransferEnd() const -> RodosTime;
    [[nodiscard]] auto BaudRate() const -> std::int32_t;


private:
    mutable RODOS::CommBuffer transferEnd_;
};

auto doInitialize = empty::DoInitialize;
auto doRead = empty::DoRead;
auto doWrite = empty::DoWrite;
auto doTransferEnd = empty::DoTransferEnd;
auto doBaudRate = empty::DoBaudRate;

auto Spi::Impl::GetInstances() -> etl::vector<Spi::Impl, maxNInstances> &
{
    static etl::vector<Spi::Impl, maxNInstances> instances;
    return instances;
}


Spi::Impl::Impl(RODOS::SPI_IDX spiIndex,
                RODOS::GPIO_PIN sckPin,
                RODOS::GPIO_PIN misoPin,
                RODOS::GPIO_PIN mosiPin)
    : spi_(spiIndex, sckPin, misoPin, mosiPin, spiNssDummyPin)
{
    transferEnd_ = RodosTime(RODOS::END_OF_TIME);
}


auto Spi::Impl::Initialize(std::uint32_t baudRate) -> void
{
    return doInitialize();
}


auto Spi::Impl::Read(void * data, std::size_t nBytes, Duration timeout) -> void
{
    return doRead();
}


auto Spi::Impl::Write(void const * data, std::size_t nBytes, Duration timeout) -> void
{
    return doWrite();
}


auto Spi::Impl::TransferEnd() const -> RodosTime
{
    return doTransferEnd();
}


auto Spi::Impl::BaudRate() const -> std::int32_t
{
    return doBaudRate();
}


// --- Set functions ---

auto SetDoInitialize(void (*doInitializeFunction)()) -> void
{
    doInitialize = doInitializeFunction;
}


auto SetDoRead(void (*doReadFunction)()) -> void
{
    doRead = doReadFunction;
}


auto SetDoWtite(void (*doWriteFunction)()) -> void
{
    doWrite = doWriteFunction;
}


auto SetDoTransferEnd(RodosTime (*doTransferEndFunction)()) -> void
{
    doTransferEnd = doTransferEndFunction;
}


auto SetDoBaudRate(std::int32_t (*doBaudRateFunction)()) -> void
{
    doBaudRate = doBaudRateFunction;
}


// --- Predefined do functions ---

namespace empty
{
auto SetAllDoFunctions() -> void
{
    SetDoInitialize(DoInitialize);
    SetDoRead(DoRead);
    SetDoWrite(DoWrite);
    SetDoTransferEnd(DoTransferEnd);
    SetDoBaudRate(DoBaudRate);
}


auto DoInitialize([[maybe_unused]] std::uint32_t baudRate) -> void
{
}


auto DoRead([[maybe_unused]] void * data,
            [[maybe_unused]] std::size_t nBytes,
            [[maybe_unused]] Duration timeout) -> void
{
}


auto DoWrite([[maybe_unused]] void const * data,
             [[maybe_unused]] std::size_t nBytes,
             [[maybe_unused]] Duration timeout) -> void
{
}


auto DoTransferEnd() -> RodosTime
{
    return endOfTime;
}


auto DoBaudRate() -> std::int32_t
{
    return 0;
}
}


Spi::Spi(RODOS::SPI_IDX spiIndex,
         RODOS::GPIO_PIN sckPin,
         RODOS::GPIO_PIN misoPin,
         RODOS::GPIO_PIN mosiPin)
{
    // TODO: Check what happens if we exceed maxNInstances
    Impl::GetInstances().emplace_back(spiIndex, sckPin, misoPin, mosiPin);
    pimpl_ = &Impl::GetInstances().back();  // NOLINT(*prefer-member-initializer)
}


auto Spi::TransferEnd() const -> RodosTime
{
    return pimpl_->TransferEnd();
}


auto Spi::BaudRate() const -> std::int32_t
{
    return pimpl_->BaudRate();
}


auto Initialize(Spi * spi, std::uint32_t baudRate) -> void
{
    spi->pimpl_->Initialize(baudRate);
}


auto Spi::Read(void * data, std::size_t nBytes, Duration timeout) -> void
{
    pimpl_->Read(data, nBytes, timeout);
}


auto Spi::Write(void const * data, std::size_t nBytes, Duration timeout) -> void
{
    pimpl_->Write(data, nBytes, timeout);
}
}

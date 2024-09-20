#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>

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

    auto Initialize(std::uint32_t baudRate, bool useOpenDrainOutputs) -> void;
    auto Read(void * data, std::size_t nBytes, Duration timeout) -> void;
    auto Write(void const * data, std::size_t nBytes, Duration timeout) -> void;
    [[nodiscard]] auto TransferEnd() const -> RodosTime;
    [[nodiscard]] auto BaudRate() const -> std::int32_t;


private:
    mutable RODOS::HAL_SPI spi_;
    mutable RODOS::CommBuffer<RodosTime> transferEnd_;
};


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
    transferEnd_.put(RodosTime(RODOS::END_OF_TIME));
}


auto Spi::Impl::Initialize(std::uint32_t baudRate, bool useOpenDrainOutputs) -> void
{
    // spi.init() only returns -1 if the SPI_IDX is out of range. Since we can check that statically
    // we do not need to report that error at runtime.
    spi_.init(baudRate, /*slave=*/false, /*tiMode=*/false, useOpenDrainOutputs);
    transferEnd_.put(RodosTime(RODOS::END_OF_TIME));
}


auto Spi::Impl::Read(void * data, std::size_t nBytes, Duration timeout) -> void
{
    // spi.read() only returns -1 or the given buffer length. It only returns -1 if the SPI is not
    // initialized, which we can check/ensure statically. Therefore, we do not need to check the
    // return value at runtime.
    transferEnd_.put(RodosTime(RODOS::NOW()) + timeout);
    spi_.read(data, nBytes);
    transferEnd_.put(RodosTime(RODOS::END_OF_TIME));
}


auto Spi::Impl::Write(void const * data, std::size_t nBytes, Duration timeout) -> void
{
    // spi.write() only returns -1 or the given buffer length. It only returns -1 if the SPI is not
    // initialized, which we can check/ensure statically. Therefore, we do not need to check the
    // return value at runtime.
    transferEnd_.put(RodosTime(RODOS::NOW()) + timeout);
    spi_.write(data, nBytes);
    transferEnd_.put(RodosTime(RODOS::END_OF_TIME));
}


auto Spi::Impl::TransferEnd() const -> RodosTime
{
    auto transferEnd = RodosTime(0);
    transferEnd_.get(transferEnd);
    return transferEnd;
}


auto Spi::Impl::BaudRate() const -> std::int32_t
{
    return spi_.status(RODOS::SPI_STATUS_BAUDRATE);
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


auto Initialize(Spi * spi, std::uint32_t baudRate, bool useOpenDrainOutputs) -> void
{
    spi->pimpl_->Initialize(baudRate, useOpenDrainOutputs);
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

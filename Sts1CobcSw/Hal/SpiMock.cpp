#include <Sts1CobcSw/Hal/SpiMock.hpp>


namespace sts1cobcsw::hal
{
auto SpiMock::SetInitialize(void (*initialize)(std::uint32_t, bool)) -> void
{
    initialize_ = initialize;
}


auto SpiMock::SetRead(void (*read)(void *, std::size_t, Duration)) -> void
{
    read_ = read;
}


auto SpiMock::SetWrite(void (*write)(void const *, std::size_t, Duration)) -> void
{
    write_ = write;
}


auto SpiMock::SetTransferEnd(RodosTime (*transferEnd)()) -> void
{
    transferEnd_ = transferEnd;
}


auto SpiMock::SetBaudRate(std::int32_t (*baudRate)()) -> void
{
    baudRate_ = baudRate;
}


auto SpiMock::DoInitialize(std::uint32_t baudRate, bool useOpenDrainOutputs) -> void
{
    initialize_(baudRate, useOpenDrainOutputs);
}


auto SpiMock::Read(void * data, std::size_t nBytes, Duration timeout) -> void
{
    read_(data, nBytes, timeout);
}


auto SpiMock::Write(void const * data, std::size_t nBytes, Duration timeout) -> void
{
    write_(data, nBytes, timeout);
}


auto SpiMock::DoTransferEnd() const -> RodosTime
{
    return transferEnd_();
}


auto SpiMock::DoBaudRate() const -> std::int32_t
{
    return baudRate_();
}
}

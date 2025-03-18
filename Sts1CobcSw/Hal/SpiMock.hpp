#pragma once


#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Vocabulary/TimeTypes.hpp>

#include <cstddef>
#include <cstdint>


namespace sts1cobcsw::hal
{
class SpiMock : public Spi
{
public:
    SpiMock() = default;
    SpiMock(SpiMock const &) = delete;
    SpiMock(SpiMock &&) = default;
    auto operator=(SpiMock const &) -> SpiMock & = delete;
    auto operator=(SpiMock &&) -> SpiMock & = default;
    ~SpiMock() override = default;

    auto SetInitialize(void (*initialize)(std::uint32_t, bool)) -> void;
    auto SetRead(void (*read)(void *, std::size_t, Duration)) -> void;
    auto SetWrite(void (*write)(void const *, std::size_t, Duration)) -> void;
    auto SetTransferEnd(RodosTime (*transferEnd)()) -> void;
    auto SetBaudRate(std::int32_t (*baudRate)()) -> void;


private:
    auto DoInitialize(std::uint32_t baudRate, bool useOpenDrainOutputs) -> void override;
    auto Read(void * data, std::size_t nBytes, Duration timeout) -> void override;
    auto Write(void const * data, std::size_t nBytes, Duration timeout) -> void override;
    [[nodiscard]] auto DoTransferEnd() const -> RodosTime override;
    [[nodiscard]] auto DoBaudRate() const -> std::int32_t override;

    void (*initialize_)(std::uint32_t, bool) = [](auto...) {};
    void (*read_)(void *, std::size_t, Duration) = [](auto...) {};
    void (*write_)(void const *, std::size_t, Duration) = [](auto...) {};
    RodosTime (*transferEnd_)() = []() { return RodosTime{}; };
    std::int32_t (*baudRate_)() = []() { return 0; };
};
}

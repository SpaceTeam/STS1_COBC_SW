#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::hal
{
enum class PinDirection
{
    in,
    out
};


enum class PinOutputType
{
    pushPull,
    openDrain
};


enum class PinState
{
    set,
    reset
};


enum class InterruptSensitivity
{
    bothEdges = 0,
    risingEdge,
    fallingEdge,
};


class GpioPin
{
public:
    // Implicit conversion from GPIO_PIN is very convenient (see Gpio.test.cpp)
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    GpioPin(RODOS::GPIO_PIN pinIndex);

    // TODO: Rename Direction(), ... to SetDirection(), ...
    auto Direction(PinDirection pinDirection) -> void;
    auto OutputType(PinOutputType pinOutputType) -> void;
    auto Set() -> void;
    auto Reset() -> void;
    auto SetInterruptSensitivity(InterruptSensitivity interruptSensitivity) -> void;
    auto EnableInterrupts() -> void;
    auto DisableInterrupts() -> void;
    auto ResetInterruptStatus() -> void;
    auto SuspendUntilInterrupt(Duration timeout) -> Result<void>;
    auto SetInterruptHandler(void (*handler)()) -> void;
    [[nodiscard]] auto Read() const -> PinState;
    [[nodiscard]] auto InterruptOccurred() const -> bool;


private:
    class GpioEventReceiver : public RODOS::IOEventReceiver
    {
    public:
        auto SetInterruptHandler(void (*handler)()) -> void;
        auto onDataReady() -> void override;

    private:
        void (*interruptHandler_)() = nullptr;
    };

    mutable RODOS::HAL_GPIO pin_;
    GpioEventReceiver eventReceiver_;
};
}


#include <Sts1CobcSw/Hal/GpioPin.ipp>

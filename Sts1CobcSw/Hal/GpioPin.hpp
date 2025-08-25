#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::hal
{
enum class PinDirection : std::uint8_t
{
    in,
    out
};


enum class PinOutputType : std::uint8_t
{
    pushPull,
    openDrain
};


enum class PinState : std::uint8_t
{
    set,
    reset
};


enum class InterruptSensitivity : std::uint8_t
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

    auto SetDirection(PinDirection pinDirection) -> void;
    auto SetOutputType(PinOutputType pinOutputType) -> void;
    auto ActivatePullUp() -> void;
    auto ActivatePullDown() -> void;
    auto DeactivatePullResistors() -> void;
    auto Set() -> void;
    auto Reset() -> void;

    auto SetInterruptSensitivity(InterruptSensitivity interruptSensitivity) -> void;
    auto EnableInterrupts() -> void;
    auto DisableInterrupts() -> void;
    auto ResetInterruptStatus() -> void;
    auto SuspendUntilInterrupt(Duration timeout) -> Result<void>;
    auto SuspendUntilInterrupt(RodosTime reactivationTime) -> Result<void>;
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


#include <Sts1CobcSw/Hal/GpioPin.ipp>  // IWYU pragma: keep

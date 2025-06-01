#include <Sts1CobcSw/Hal/GpioPin.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <compare>
#include <utility>


namespace sts1cobcsw::hal
{
auto GpioPin::SetInterruptSensitivity(InterruptSensitivity interruptSensitivity) -> void
{
    // NOLINTBEGIN(*avoid-nested-conditional-operator)
    auto sensitivity =
        interruptSensitivity == InterruptSensitivity::bothEdges    ? RODOS::GPIO_IRQ_SENS_BOTH
        : interruptSensitivity == InterruptSensitivity::risingEdge ? RODOS::GPIO_IRQ_SENS_RISING
                                                                   : RODOS::GPIO_IRQ_SENS_FALLING;
    // NOLINTEND(*avoid-nested-conditional-operator)
    pin_.config(RODOS::GPIO_CFG_IRQ_SENSITIVITY, sensitivity);
}


auto GpioPin::SuspendUntilInterrupt(Duration timeout) -> Result<void>
{
    auto reactivationTime = CurrentRodosTime() + timeout;
    pin_.suspendUntilDataReady(value_of(reactivationTime));
    if(CurrentRodosTime() >= reactivationTime)
    {
        return ErrorCode::timeout;
    }
    return outcome_v2::success();
}


auto GpioPin::SuspendUntilInterrupt(RodosTime reactivationTime) -> Result<void>
{
    pin_.suspendUntilDataReady(value_of(reactivationTime));
    if(CurrentRodosTime() >= reactivationTime)
    {
        return ErrorCode::timeout;
    }
    return outcome_v2::success();
}


auto GpioPin::SetInterruptHandler(void (*handler)()) -> void
{
    if(handler == nullptr)
    {
        pin_.setIoEventReceiver(nullptr);
    }
    else
    {
        eventReceiver_.SetInterruptHandler(handler);
        pin_.setIoEventReceiver(&eventReceiver_);
    }
}


auto GpioPin::GpioEventReceiver::onDataReady() -> void
{
    if(interruptHandler_ != nullptr)
    {
        interruptHandler_();
    }
}
}

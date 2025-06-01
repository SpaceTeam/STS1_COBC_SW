#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
constexpr auto ledBlinkThreadPriority = 100;
constexpr auto interruptTestPriority = 200;

auto nInterrupts = 0;
auto ledPins = std::to_array<hal::GpioPin>({hal::led1Pin, hal::led2Pin});
auto uciUartRxGpioPin = hal::GpioPin(hal::uciUartRxPin);


class LedBlinkThread : public RODOS::StaticThread<>
{
public:
    LedBlinkThread() : StaticThread("LedBlinkThread", ledBlinkThreadPriority)
    {}


private:
    void init() override
    {
        for(auto & pin : ledPins)
        {
            pin.SetDirection(hal::PinDirection::out);
        }
    }


    void run() override
    {
        auto toggle = true;
        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            for(auto & pin : ledPins)
            {
                toggle ? pin.Set() : pin.Reset();
                RODOS::PRINTF("Pin was %s and reads %s\n",
                              toggle ? "  set" : "reset",                             // NOLINT
                              pin.Read() == hal::PinState::set ? "  set" : "reset");  // NOLINT
            }
            toggle = not toggle;
        }
    }
} ledBlinkThread;


class InterruptTest : public RODOS::StaticThread<>
{
public:
    InterruptTest() : StaticThread("InterruptTest", interruptTestPriority)
    {}


private:
    void init() override
    {
        uciUartRxGpioPin.SetDirection(hal::PinDirection::in);
        uciUartRxGpioPin.SetInterruptSensitivity(hal::InterruptSensitivity::fallingEdge);
    }


    void run() override
    {
        static constexpr auto timeout = 5 * s;
        RODOS::PRINTF("\n");
        while(true)
        {
            RODOS::PRINTF("Waiting %i s for falling edge interrupts on UCI UART RX pin\n\n",
                          static_cast<int>(timeout / s));
            nInterrupts = 0;
            uciUartRxGpioPin.ResetInterruptStatus();
            uciUartRxGpioPin.SetInterruptHandler([]() { nInterrupts++; });
            uciUartRxGpioPin.EnableInterrupts();
            auto result = uciUartRxGpioPin.SuspendUntilInterrupt(timeout);
            if(not result.has_error())
            {
                sts1cobcsw::SuspendFor(1 * ms);
            }
            uciUartRxGpioPin.DisableInterrupts();
            uciUartRxGpioPin.SetInterruptHandler(nullptr);
            RODOS::PRINTF("\n");
            RODOS::PRINTF("%i interrupts occurred\n", nInterrupts);
        }
    }
} interruptTest;
}

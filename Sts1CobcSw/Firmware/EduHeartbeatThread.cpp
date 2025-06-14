#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <cinttypes>  // IWYU pragma: keep
#include <utility>


namespace sts1cobcsw
{
namespace
{
// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2000U;
constexpr auto edgeCounterThreshold = 4;
constexpr auto interruptMargin = 5 * ms;

auto ledGpioPin = hal::GpioPin(hal::led1Pin);
auto eduHeartbeatGpioPin = hal::GpioPin(hal::eduHeartbeatPin);


// TODO: Use an external interrupt for detecting the edges of the heartbeat
class EduHeartbeatThread : public RODOS::StaticThread<stackSize>
{
public:
    EduHeartbeatThread() : StaticThread("EduHeartbeatThread", eduHeartbeatThreadPriority)
    {}


private:
    void init() override
    {
        eduHeartbeatGpioPin.SetDirection(hal::PinDirection::in);
        ledGpioPin.SetDirection(hal::PinDirection::out);
        ledGpioPin.Reset();

        eduHeartbeatGpioPin.SetInterruptSensitivity(hal::InterruptSensitivity::bothEdges);
        eduHeartbeatGpioPin.EnableInterrupts();
    }


    void run() override
    {
        //        TIME_LOOP(0, 1*RODOS::SECONDS){
        //
        //            auto type = EduIsAlive();
        //            if(type) {
        //                DEBUG_PRINT("Edu is alive\n");
        //            } else {
        //                DEBUG_PRINT("Edu is dead\n");
        //            }
        //            eduIsAliveTopic.publish(type);
        //
        //        }

        // RODOS::AT(RODOS::END_OF_TIME);

        auto const heartbeatFrequency = 10;  // Hz
        auto const heartbeatPeriod = 1 * s / heartbeatFrequency;

        auto edgeCounter = 0;

        TIME_LOOP(0, value_of(heartbeatPeriod))
        {
            auto result =
                eduHeartbeatGpioPin.SuspendUntilInterrupt(heartbeatPeriod - interruptMargin);

            if(result.has_error())
            {
                // timeout, no edge during a whole heartbeat period
                edgeCounter = 0;
                eduIsAliveTopic.publish(false);
            }
            else
            {
                // edge detected during heartbeat period
                edgeCounter++;
                if(edgeCounter >= edgeCounterThreshold)
                {
                    eduIsAliveTopic.publish(true);
                    edgeCounter = 0;
                }
            }
        }
    }
} eduHeartbeatThread;
}
}

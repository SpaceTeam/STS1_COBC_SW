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

auto ledGpioPin = hal::GpioPin(hal::led1Pin);
auto epsChargingGpioPin = hal::GpioPin(hal::epsChargingPin);
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
        epsChargingGpioPin.SetDirection(hal::PinDirection::out);
        ledGpioPin.Reset();
        epsChargingGpioPin.Reset();

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

        auto const heartbeatFrequency = 10;                     // Hz
        auto const samplingFrequency = 5 * heartbeatFrequency;  // Hz
        auto const samplingPeriod = 1 * s / samplingFrequency;

        auto edgeCounter = 0;

        DEBUG_PRINT("Sampling period : %" PRIi64 " ms\n", samplingPeriod / ms);
        auto toggle = true;
        TIME_LOOP(0, value_of(samplingPeriod) * 5)
        {
            // Read current heartbeat value

            if(toggle)
            {
                epsChargingGpioPin.Set();
            }
            else
            {
                epsChargingGpioPin.Reset();
            }
            toggle = not toggle;

 
            auto result = eduHeartbeatGpioPin.SuspendUntilInterrupt(samplingPeriod * 5);

            if(result.has_error())
            {
                // timeout
                edgeCounter = 0;
                // DEBUG_PRINT("Edu is alive published to false\n");
                eduIsAliveTopic.publish(false);
            } else {
                // edge detected during sampling period
                edgeCounter++;
                if(edgeCounter >= edgeCounterThreshold)
                {
                    // DEBUG_PRINT("Edu is alive published to true\n");
                    eduIsAliveTopic.publish(true);
                    edgeCounter = 0;
                }
            }

        }
    }
} eduHeartbeatThread;
}
}

#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cinttypes>  // IWYU pragma: keep
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 2000U;
constexpr auto heartbeatFrequency = 10;
constexpr auto heartbeatPeriod = 1 * s / heartbeatFrequency;
// Due to integer arithmetic, we cannot store the safety margin of 12 / 10 in a separate variable
constexpr auto interruptTimeout = heartbeatPeriod / 2 * 12 / 10;
constexpr auto edgeCounterThreshold = 3;

auto eduHeartbeatGpioPin = hal::GpioPin(hal::eduHeartbeatPin);


class EduHeartbeatThread : public RODOS::StaticThread<stackSize>
{
public:
    EduHeartbeatThread() : StaticThread("EduHeartbeatThread", eduHeartbeatThreadPriority)
    {}


private:
    void init() override
    {
        eduHeartbeatGpioPin.SetDirection(hal::PinDirection::in);
        eduHeartbeatGpioPin.SetInterruptSensitivity(hal::InterruptSensitivity::bothEdges);
        eduHeartbeatGpioPin.EnableInterrupts();
    }


    void run() override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        auto edgeCounter = 0;
        while(true)
        {
            auto result = eduHeartbeatGpioPin.SuspendUntilInterrupt(interruptTimeout);
            if(result.has_error())
            {
                edgeCounter = 0;
                eduIsAliveTopic.publish(false);
            }
            else
            {
                ++edgeCounter;
                edgeCounter = std::min(edgeCounter, edgeCounterThreshold);
                if(edgeCounter == edgeCounterThreshold)
                {
                    eduIsAliveTopic.publish(true);
                }
            }
        }
    }
} eduHeartbeatThread;
}
}

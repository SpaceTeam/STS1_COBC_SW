#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::MILLISECONDS;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;
constexpr auto threadPriority = 100;

auto ledGpioPin = hal::GpioPin(hal::ledPin);
auto eduHeartBeatGpioPin = hal::GpioPin(hal::eduHeartbeatPin);


class EduHeartbeatThread : public RODOS::StaticThread<stackSize>
{
public:
    // TODO: Add this to all the other threads as well
    EduHeartbeatThread() : StaticThread("EduHeartbeatThread", threadPriority)
    {
    }

private:
    void init() override
    {
        eduHeartBeatGpioPin.Direction(hal::PinDirection::in);
        ledGpioPin.Direction(hal::PinDirection::out);
        ledGpioPin.Reset();
    }


    void run() override
    {
        namespace ts = type_safe;
        using ts::operator""_i;
        using ts::operator""_isize;

        constexpr auto heartbeatFrequency = 10_isize;                     // Hz
        constexpr auto samplingFrequency = 5_isize * heartbeatFrequency;  // Hz
        constexpr auto samplingPeriode = 1'000_isize * MILLISECONDS / samplingFrequency;

        auto samplingCount = 0_i;
        ts::bool_t heartbeatIsConstant = true;
        auto oldHeartbeat = eduHeartBeatGpioPin.Read();
        TIME_LOOP(0, samplingPeriode.get())
        {
            auto heartbeat = eduHeartBeatGpioPin.Read();
            ++samplingCount;

            if(heartbeatIsConstant and (heartbeat != oldHeartbeat))
            {
                heartbeatIsConstant = false;
                ledGpioPin.Set();
                eduIsAliveTopic.publish(true);
            }

            oldHeartbeat = heartbeat;

            // Check if heartbeat is constant over a whole heartbeat period
            if(samplingCount == samplingFrequency / heartbeatFrequency)
            {
                if(heartbeatIsConstant)
                {
                    ledGpioPin.Reset();
                    eduIsAliveTopic.publish(false);
                }
                heartbeatIsConstant = true;
                samplingCount = 0_i;
            }
        }
    }
} eduHeartbeatThread;
// TODO: Get back to the inline thread variable definition (like above) for all threads
}

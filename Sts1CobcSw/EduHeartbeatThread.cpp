#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::MILLISECONDS;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;


auto eduHeartbeatGpioPin = hal::GpioPin(hal::eduHeartbeatPin);


class EduHeartbeatThread : public RODOS::StaticThread<stackSize>
{
public:
    EduHeartbeatThread() : StaticThread("EduHeartbeatThread", eduHeartbeatThreadPriority)
    {
    }


private:
    void init() override
    {
        eduHeartbeatGpioPin.Direction(hal::PinDirection::in);
    }


    // TODO: It must be possible to simplify the logic for heartbeat detection
    void run() override
    {
        // Hack for HAF
        eduIsAliveTopic.publish(false);
        RODOS::AT(RODOS::NOW() + 20 * RODOS::SECONDS);
        eduIsAliveTopic.publish(true);
        
        // namespace ts = type_safe;
        // using ts::operator""_i;
        // using ts::operator""_isize;

        // constexpr auto heartbeatFrequency = 10_isize;                     // Hz
        // constexpr auto samplingFrequency = 5_isize * heartbeatFrequency;  // Hz
        // constexpr auto samplingPeriod = 1000_isize * MILLISECONDS / samplingFrequency;
        // constexpr auto edgeCounterThreshold = 4;

        // auto edgeCounter = 0_i;
        // auto samplingCount = 0_i;
        // ts::bool_t heartbeatIsConstant = true;
        // auto oldHeartbeat = eduHeartbeatGpioPin.Read();

        // TIME_LOOP(0, samplingPeriod.get())
        // {
        //     auto heartbeat = eduHeartbeatGpioPin.Read();
        //     ++samplingCount;

        //     if(heartbeatIsConstant and (heartbeat != oldHeartbeat))
        //     {
        //         heartbeatIsConstant = false;
        //         edgeCounter++;
        //     }
        //     if(edgeCounter == edgeCounterThreshold)
        //     {
        //         eduIsAliveTopic.publish(true);
        //         edgeCounter = 0;
        //     }

        //     // Check if heartbeat is constant over a whole heartbeat period
        //     if(samplingCount == samplingFrequency / heartbeatFrequency)
        //     {
        //         if(heartbeatIsConstant)
        //         {
        //             edgeCounter = 0;
        //             eduIsAliveTopic.publish(false);
        //         }
        //         heartbeatIsConstant = true;
        //         samplingCount = 0_i;
        //     }
        //     oldHeartbeat = heartbeat;
        // }
    }
} eduHeartbeatThread;
}

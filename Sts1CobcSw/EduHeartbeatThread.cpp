#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::MILLISECONDS;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;
// Must be the highest of all, otherwise educommunicationerror will never resume from busy wait.
constexpr auto threadPriority = 600;

auto ledGpioPin = hal::GpioPin(hal::ledPin);
auto epsChargingGpioPin = hal::GpioPin(hal::epsChargingPin);
auto eduHeartBeatGpioPin = hal::GpioPin(hal::eduHeartbeatPin);

// periphery::Edu edu = periphery::Edu();

auto constexpr edgeCounterThreshold = 4;

auto EduIsAlive()
{
    auto begin = RODOS::NOW();

    auto refHeartbeat = eduHeartBeatGpioPin.Read();
    auto heartbeat = eduHeartBeatGpioPin.Read();
    auto edgeCounter = 0;
    for(int i = 0; i < 1'000'000; ++i)
    {
        heartbeat = eduHeartBeatGpioPin.Read();
        if(heartbeat != refHeartbeat)
        {
            edgeCounter++;
            refHeartbeat = heartbeat;
            if(edgeCounter >= 4)
            {
                return true;
            }
        }
    }
    auto executionTime = RODOS::NOW() - begin;
    RODOS::PRINTF("Execution Time of EduIsAlive (ns) : %lld\n", executionTime);
    RODOS::PRINTF("Execution Time of EduIsAlive (ms) : %lld\n",
                  executionTime / RODOS::MILLISECONDS);
    return false;
}

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
        epsChargingGpioPin.Direction(hal::PinDirection::out);
        ledGpioPin.Reset();
        epsChargingGpioPin.Reset();
        // edu.Initialize();
    }


    void run() override
    {
        //        TIME_LOOP(0, 1*RODOS::SECONDS){
        //
        //            auto type = EduIsAlive();
        //            if(type) {
        //                RODOS::PRINTF("Edu is alive\n");
        //            } else {
        //                RODOS::PRINTF("Edu is dead\n");
        //            }
        //            eduIsAliveTopic.publish(type);
        //
        //        }


        // RODOS::AT(RODOS::END_OF_TIME);
        namespace ts = type_safe;
        using ts::operator""_i;
        using ts::operator""_isize;

        constexpr auto heartbeatFrequency = 10_isize;                     // Hz
        constexpr auto samplingFrequency = 5_isize * heartbeatFrequency;  // Hz
        constexpr auto samplingPeriode = 1'000_isize * MILLISECONDS / samplingFrequency;

        auto samplingCount = 0_i;
        ts::bool_t heartbeatIsConstant = true;
        auto oldHeartbeat = eduHeartBeatGpioPin.Read();
        auto edgeCounter = 0_i;

        RODOS::PRINTF("Sampling period : %lld\n", samplingPeriode.get() / RODOS::MILLISECONDS);
        auto toggle = true;
        TIME_LOOP(0, samplingPeriode.get())
        {
            // Read current heartbneat value

            if(toggle)
            {
                epsChargingGpioPin.Set();
            }
            else
            {
                epsChargingGpioPin.Reset();
            }
            toggle = not toggle;

            auto heartbeat = eduHeartBeatGpioPin.Read();

            ++samplingCount;

            // If hearbeat wass constant bu is not anymore
            if(heartbeatIsConstant and (heartbeat != oldHeartbeat))
            {
                heartbeatIsConstant = false;
                // RODOS::PRINTF("Detected an edge \n");
                edgeCounter++;
            }


            if(edgeCounter == 4)
            {
                // RODOS::PRINTF("Edu is alive published to true\n");
                eduIsAliveTopic.publish(true);
                edgeCounter = 0;
            }
            // if(oldHeartbeat == heartbeat) {
            //    heartbeatIsConstant = true;
            //}

            oldHeartbeat = heartbeat;

            // Check if heartbeat is constant over a whole heartbeat period
            if(samplingCount == samplingFrequency / heartbeatFrequency)
            {
                if(heartbeatIsConstant)
                {
                    edgeCounter = 0;
                    // RODOS::PRINTF("Edu is alive published to false\n");
                    eduIsAliveTopic.publish(false);
                }
                heartbeatIsConstant = true;
                samplingCount = 0_i;
            }
        }
    }
} eduHeartbeatThread;
// TODO: Get back to the inline thread variable definition (like above) for all threads
/*
constexpr auto dummyThreadPriority = 500;
class DummyThread : public RODOS::StaticThread<>
{
public:
    // TODO: Add this to all the other threads as well
    DummyThread() : StaticThread("DummyThread", dummyThreadPriority)
    {
    }

private:

    void init() {
    }

    void run() {
        TIME_LOOP(0, 1 * RODOS::SECONDS) {

            RODOS::PRINTF("Hello From DummyThread Timeloo\n");
        }
    }

} dummyThread;
*/

}
